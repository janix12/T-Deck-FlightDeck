#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <StreamUtils.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <algorithm>
#include "roots.h"

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
};
static SemaphoreHandle_t networkGate;
static QueueHandle_t requests, results;

class ExternalRam : public ArduinoJson::Allocator {
 public:
  void *allocate(size_t n) override { return heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
  void deallocate(void *p) override { heap_caps_free(p); }
  void *reallocate(void *p, size_t n) override { return heap_caps_realloc(p, n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
};

static void collectPlanes(JsonArrayConst list, Traffic &out) {
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
    if (p.km > out.area.radius * 1.852) continue;
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

static void fetchArea(const Area &area, Traffic &out) {
  out = {};
  out.area = area;
  WiFiClientSecure tls;
  tls.setCACert(adsbRoot);
  tls.setHandshakeTimeout(8);
  HTTPClient http;
  String url = "https://api.adsb.lol/v2/point/" + String(area.lat, 4) + "/" + String(area.lon, 4) + "/" + String(area.radius);
  http.setReuse(false);
  http.setConnectTimeout(7000);
  http.setTimeout(8000);
  if (!http.begin(tls, url)) { out.code = -1; return; }
  const char *headers[]={"Transfer-Encoding"};
  http.collectHeaders(headers,1);
  out.code = http.GET();
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
      collectPlanes(data["ac"], out);
    }
  }
  http.end();
}

static void trafficWorker(void *) {
  Area area;
  static Traffic reply;
  for (;;) {
    if (xQueueReceive(requests, &area, portMAX_DELAY) == pdTRUE) {
      xSemaphoreTake(networkGate,portMAX_DELAY);
      fetchArea(area, reply);
      xSemaphoreGive(networkGate);
      xQueueOverwrite(results, &reply);
      reply.planes=nullptr; // Ownership passes through the queue to the display task.
    }
  }
}
