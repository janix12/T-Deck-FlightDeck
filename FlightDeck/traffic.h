#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <StreamUtils.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include <atomic>
#include "roots.h"
#include "traffic_budget.h"
#include <Preferences.h>

struct Area {
  double lat, lon;
  int radius;
};
struct Plane {
  char name[12];
  char id[10];
  float lat,lon,verticalRate;
  float bearing, km, heading, altitude, speed;
  bool ground, airKnown;
  time_t observed, transitionUntil, turnStart, lastTurn;
  float turnSum, startLat, startLon, startAltitude;
};
struct Traffic {
  Plane *planes;
  int count, total, code;
  time_t stamp;
  Area area;
  TrafficBudget budget;
  uint32_t retryAt;
  char lookupId[sizeof(Plane::id)];
};
struct TrafficRequest {Area area; unsigned period; Area observer; char lookupId[sizeof(Plane::id)];};
static std::atomic<bool> menuOpen{false};
static constexpr int pollPaused=-110;
static TrafficBudget budget;
static Preferences budgetPrefs;
static bool budgetReady;
static void loadBudget(){
  budgetReady=budgetPrefs.begin("fd-adsb",false);
  if(budgetReady && budgetPrefs.isKey("budget"))
    budgetReady=budgetPrefs.getBytesLength("budget")==sizeof(budget) &&
      budgetPrefs.getBytes("budget",&budget,sizeof(budget))==sizeof(budget);
}
static SemaphoreHandle_t networkGate;
static QueueHandle_t requests, results;

class ExternalRam : public ArduinoJson::Allocator {
 public:
  void *allocate(size_t n) override { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
  void deallocate(void *p) override { heap_caps_free(p); }
  void *reallocate(void *p, size_t n) override { return heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
};

static void collectPlanes(JsonArrayConst list, Traffic &out,const TrafficRequest &request) {
  size_t capacity=list.size();
  if(capacity){
    out.planes=static_cast<Plane*>(ps_malloc(capacity*sizeof(Plane)));
    if(!out.planes){out.code=-102;return;}
  }
  for (JsonObjectConst obj : list) {
    if (!obj["lat"].is<double>() || !obj["lon"].is<double>() || (obj["seen_pos"] | 0.f) > 90) continue;
    double a = radians(out.area.lat), b = radians(obj["lat"].as<double>());
    double dl = radians(obj["lon"].as<double>() - out.area.lon);
    double h = sq(sin((b-a)/2)) + cos(a)*cos(b)*sq(sin(dl/2));
    Plane p{};
    strlcpy(p.id,obj["hex"] | "",sizeof(p.id));
    p.lat=obj["lat"]; p.lon=obj["lon"];
    p.observed=out.stamp-time_t(obj["seen_pos"] | 0.f);
    p.verticalRate=obj["baro_rate"].is<float>()?obj["baro_rate"].as<float>():(obj["geom_rate"] | (float)NAN);
    p.km = 12742.0 * asin(sqrt(constrain(h, 0.0, 1.0)));
    if(!request.lookupId[0] && request.area.lat==out.area.lat && request.area.lon==out.area.lon &&
       p.km>request.area.radius*1.852)continue;
    if (out.lookupId[0] && strcmp(out.lookupId,p.id)) continue;
    p.bearing = atan2(sin(dl)*cos(b), cos(a)*sin(b)-sin(a)*cos(b)*cos(dl));
    p.heading = obj["track"] | (float)NAN;
    p.speed = (obj["gs"] | (float)NAN) * 1.852f;
    p.ground = !strcmp(obj["alt_baro"] | "", "ground");
    p.altitude = (obj["alt_baro"].is<float>() ? obj["alt_baro"].as<float>() : (obj["alt_geom"] | (float)NAN)) * 0.3048f;
    p.airKnown=!p.ground && isfinite(p.altitude);
    String name = obj["flight"] | "";
    name.trim();
    if (!name.length()) name = obj["hex"] | "?";
    name.toCharArray(p.name, sizeof(p.name));
    ++out.total;
    out.planes[out.count++]=p;
  }
  if(out.count>1) std::sort(out.planes,out.planes+out.count,[](const Plane &a,const Plane &b){return a.km<b.km;});
}

static void fetchArea(const TrafficRequest &request, Traffic &out) {
  out = {};
  out.area = request.observer;
  strcpy(out.lookupId,request.lookupId);
  WiFiClientSecure tls;
  tls.setCACert(adsbRoot);
  tls.setHandshakeTimeout(8);
  HTTPClient http;
  const Area &area=request.area;
  String url="https://api.adsb.lol/v2/";
  if(request.lookupId[0])url+="hex/"+String(request.lookupId);
  else url+="point/"+String(area.lat,4)+"/"+String(area.lon,4)+"/"+String(area.radius);
  http.setReuse(false);
  http.setConnectTimeout(7000);
  http.setTimeout(8000);
  if (!http.begin(tls, url)) { out.code = -1; return; }
  const char *headers[]={"Transfer-Encoding","Retry-After"};
  http.collectHeaders(headers,2);
  out.code = http.GET();
  if(out.code==429 || out.code==503){
    String retry=http.header("Retry-After");retry.trim();
    out.retryAt=trafficRetryTime(retry.c_str(),time(nullptr));
  }
  if (out.code == 200) {
    JsonDocument filter;
    filter["now"] = true;
    for (auto key : {"lat","lon","hex","flight","alt_baro","alt_geom","gs","track","seen_pos","baro_rate","geom_rate"})
      filter["ac"][0][key] = true;
    ExternalRam ram;
    JsonDocument data(&ram);
    auto &raw=http.getStream();
    ChunkDecodingStream chunks(raw);
    Stream &body=http.header("Transfer-Encoding").equalsIgnoreCase("chunked")?static_cast<Stream&>(chunks):static_cast<Stream&>(raw);
    auto error = deserializeJson(data, body, DeserializationOption::Filter(filter));
    if (error || !data["ac"].is<JsonArray>() || !data["now"].is<double>()) out.code = -101;
    else {
      double epoch = data["now"].as<double>();
      out.stamp = time_t(epoch > 1e11 ? epoch/1000 : epoch);
      collectPlanes(data["ac"], out,request);
    }
  }
  http.end();
}

static void trafficWorker(void *) {
  TrafficRequest request;
  TrafficBudget ledger=budget;
  static Traffic reply;
  for (;;) {
    if (xQueueReceive(requests, &request, portMAX_DELAY) == pdTRUE) {
      xSemaphoreTake(networkGate,portMAX_DELAY);
      reply={};
      TrafficBudget reserved=ledger;
      if(menuOpen)reply.code=pollPaused;
      else if(WiFi.status()!=WL_CONNECTED)reply.code=-1;
      else if(!reserved.reserve(time(nullptr),request.period))reply.code=-103;
      else if(!budgetReady || budgetPrefs.putBytes("budget",&reserved,sizeof(reserved))!=sizeof(reserved))reply.code=-104;
      else{
        ledger=reserved;
        fetchArea(request,reply);
        if(reply.code==429 || (reply.code==503 && reply.retryAt)){
          ledger.limited(time(nullptr),reply.retryAt);
          if(budgetPrefs.putBytes("budget",&ledger,sizeof(ledger))!=sizeof(ledger))
            Serial.println("ADS-B cooldown storage failed");
        }else if(reply.code==200){ledger.retryAt=0;ledger.limitedReplies=0;}
      }
      reply.budget=ledger;
      xSemaphoreGive(networkGate);
      xQueueOverwrite(results, &reply);
      reply.planes=nullptr; // Ownership passes through the queue to the display task.
    }
  }
}
