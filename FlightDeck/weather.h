#pragma once
#include <PNGdec.h>
#undef local  // zlib's internal macro from PNGdec
#include "weather_roots.h"

struct WxRequest {int layer,z; float lat,lon,halfLat,halfLon;};
struct RainPixel {uint16_t color; uint8_t alpha;};
struct WxFrame {
  WxRequest at; int code,frames; time_t stamp[3]; RainPixel *rain[3]; float temps[9];bool metar;
};
static QueueHandle_t wxJobs,wxReplies;
static WxFrame wxCache[2];
static bool wxBusy,playing=true;
static int rainIndex=-1;
static uint32_t nextWx[2],lastWxAttempt;
static WxRequest lastWxJob{};
static float mercY(float lat){return asinhf(tanf(radians(constrain(lat,-85.f,85.f))));}
static void releaseWx(WxFrame &w){for(auto &p:w.rain){free(p);p=nullptr;}}

// HTTPClient decodes chunked transfers into this bounded PSRAM buffer.
class WeatherBody:public Stream {
 public:
  uint8_t *data; size_t used=0,capacity;
  explicit WeatherBody(size_t n):data((uint8_t*)ps_malloc(n)),capacity(n){}
  ~WeatherBody(){free(data);}
  size_t write(const uint8_t *p,size_t n) override {
    if(!data || n>capacity-used)return 0;
    memcpy(data+used,p,n);used+=n;return n;
  }
  size_t write(uint8_t b) override{return write(&b,1);}
  int available() override{return 0;} int read() override{return -1;} int peek() override{return -1;}
  void flush() override{}
};
static int wxGet(const String &url,WeatherBody &body){
  if(!body.data)return -201;
  WiFiClientSecure tls;tls.setCACert(weatherRoots);tls.setHandshakeTimeout(10);
  HTTPClient http;http.setReuse(false);http.setConnectTimeout(7000);http.setTimeout(9000);
  if(!http.begin(tls,url))return -202;
  http.setUserAgent("FlightDeck-TDeck/1.0");
  int code=http.GET();
  if(code==200 && (http.getSize()>int(body.capacity) || http.writeToStream(&body)<=0))code=-203;
  http.end();return code;
}
// Expand the luminance range of the saturated blues on the T-Deck LCD.
// The weight fades to zero outside the blue part of the RainViewer palette.
static uint16_t rainColor(int r,int g,int b){
  int weight=constrain(min((b-200)*5,(b-r-80)*3),0,255);
  if(weight){
    int light=g<96?g*50/96:g<144?50+(g-96)*174/48:224+(g-144)*31/111;
    int lift=(light-g)*weight/255;
    r=constrain(r+lift,0,255);g+=lift;
    b-=max(0,180-g)*weight/510;
  }
  return ((r&248)<<8)|((g&252)<<3)|(b>>3);
}
static uint16_t rainOverLand(RainPixel pixel,uint16_t base){
  if(pixel.alpha==255)return pixel.color;
  int a=pixel.alpha,inv=255-a;
  int r=((pixel.color>>11)*a+(base>>11)*inv+127)/255;
  int g=(((pixel.color>>5)&63)*a+((base>>5)&63)*inv+127)/255;
  int b=((pixel.color&31)*a+(base&31)*inv+127)/255;
  return (r<<11)|(g<<5)|b;
}
struct TileDecode {PNG *decoder;RainPixel *pixels;int rows;};
static int rainRow(PNGDRAW *row){
  auto &r=*static_cast<TileDecode*>(row->pUser);
  if(row->y!=r.rows || row->y>=256 || row->iWidth!=256)return 0;
  RainPixel *line=r.pixels+256*row->y;
  // PNGdec's S3 vector converter requires aligned buffers. Decoded scanlines
  // and PSRAM allocations do not guarantee that alignment.
  if(row->iPixelType==PNG_PIXEL_TRUECOLOR_ALPHA){
    if(row->iBpp!=8 || row->iPitch<256*4)return 0;
    const uint8_t *p=row->pPixels;
    for(int x=0;x<256;x++,p+=4){
      line[x]={rainColor(p[0],p[1],p[2]),p[3]};
    }
    ++r.rows;return 1;
  }
  uint16_t colors[256];
  r.decoder->getLineAsRGB565(row,colors,PNG_RGB565_LITTLE_ENDIAN,0xffffffff);
  for(int i=0;i<256;i++){
    uint8_t a=255;
    if(row->iPixelType==PNG_PIXEL_INDEXED){
      int bits=row->iBpp;
      if(bits!=1 && bits!=2 && bits!=4 && bits!=8)return 0;
      int bit=i*bits;
      int index=(row->pPixels[bit/8]>>(8-bits-bit%8))&((1<<bits)-1);
      a=row->pPalette[768+index];
    }else if(row->iPixelType==PNG_PIXEL_GRAY_ALPHA){
      if(row->iBpp!=8)return 0;
      a=row->pPixels[i*2+1];
    }
    uint16_t c=colors[i];
    line[i]={rainColor((c>>11)*255/31,((c>>5)&63)*255/63,(c&31)*255/31),a};
  }
  ++r.rows;return 1;
}
static void getRain(WxFrame &out){
  WeatherBody index(16000);out.code=wxGet("https://api.rainviewer.com/public/weather-maps.json",index);
  if(out.code!=200)return;
  ExternalRam ram;JsonDocument doc(&ram);
  if(deserializeJson(doc,index.data,index.used)){out.code=-204;return;}
  JsonArray list=doc["radar"]["past"];String host=doc["host"]|"";
  if(!list.size() || host!="https://tilecache.rainviewer.com"){out.code=-205;return;}
  void *storage=ps_malloc(sizeof(PNG));if(!storage){out.code=-201;return;}
  PNG *png=new(storage) PNG;
  for(int j=max(0,int(list.size())-3);j<int(list.size());j++){
    String path=list[j]["path"]|"";
    if(!path.startsWith("/v2/radar/")){out.code=-205;break;}
    WeatherBody image(280000);
    out.code=wxGet(host+path+"/256/"+String(out.at.z)+"/"+String(out.at.lat,5)+"/"+String(out.at.lon,5)+"/2/1_1.png",image);
    if(out.code!=200)break;
    int slot=out.frames;
    if(image.used<33 || png->openRAM(image.data,image.used,rainRow)!=PNG_SUCCESS){out.code=-206;break;}
    if(png->getWidth()!=256 || png->getHeight()!=256){png->close();out.code=-206;break;}
    out.rain[slot]=(RainPixel*)ps_malloc(256*256*sizeof(RainPixel));
    TileDecode r{png,out.rain[slot],0};
    bool ok=r.pixels && png->decode(&r,0)==PNG_SUCCESS && r.rows==256;png->close();
    if(!ok){out.code=-206;break;}
    out.stamp[slot]=list[j]["time"]|0L;out.frames++;
  }
  png->~PNG();free(png);
}
static void getHeat(WxFrame &out){
  String lats,lons;
  for(int y=0;y<3;y++)for(int x=0;x<3;x++){
    if(lats.length()){lats+=',';lons+=',';}
    lats+=String(out.at.lat+(1-y)*out.at.halfLat,4);
    lons+=String(remainderf(out.at.lon+(x-1)*out.at.halfLon,360),4);
  }
  WeatherBody body(18000);
  out.code=wxGet("https://api.open-meteo.com/v1/forecast?latitude="+lats+"&longitude="+lons+"&current=temperature_2m&timeformat=unixtime",body);
  if(out.code!=200)return;
  ExternalRam ram;JsonDocument doc(&ram);
  if(deserializeJson(doc,body.data,body.used) || doc.size()!=9){out.code=-207;return;}
  for(int i=0;i<9;i++){
    JsonObject current=doc[i]["current"];
    if(!current["temperature_2m"].is<float>() || !current["time"].is<long>()){out.code=-208;return;}
    out.temps[i]=current["temperature_2m"];out.stamp[0]=current["time"];
  }
  out.frames=1;
}
static void getMetar(WxFrame &out){
  const auto &q=out.at;
  const float dy=q.halfLat+.7f,dx=q.halfLon+.9f;
  if(q.lon-dx < -180 || q.lon+dx > 180)return;
  String bbox=String(max(-89.f,q.lat-dy),3)+","+String(q.lon-dx,3)+","+
              String(min(89.f,q.lat+dy),3)+","+String(q.lon+dx,3);
  WeatherBody body(320000);
  out.code=wxGet("https://aviationweather.gov/api/data/metar?format=json&bbox="+bbox,body);
  if(out.code!=200)return;
  ExternalRam ram;JsonDocument doc(&ram),filter;
  for(const char *key:{"lat","lon","temp","obsTime"})filter[0][key]=true;
  if(deserializeJson(doc,body.data,body.used,DeserializationOption::Filter(filter))){out.code=-207;return;}
  int valid=0;time_t now=time(nullptr);out.stamp[0]=now;
  for(int y=0;y<3;y++)for(int x=0;x<3;x++){
    float lat=q.lat+(1-y)*q.halfLat,lon=q.lon+(x-1)*q.halfLon;
    float total=0,sum=0;
    for(JsonObject station:doc.as<JsonArray>()){
      if(!station["temp"].is<float>() || !station["lat"].is<float>() || !station["lon"].is<float>())continue;
      time_t t=station["obsTime"]|0L;
      if(t<now-7200 || t>now+300)continue;
      float dist2=sq(111.2f*(station["lat"].as<float>()-lat))+
          sq(111.2f*cosf(radians(lat))*(station["lon"].as<float>()-lon));
      if(dist2>150*150)continue;
      float weight=1.f/max(1.f,dist2);total+=weight;sum+=weight*station["temp"].as<float>();
      out.stamp[0]=min(out.stamp[0],t);
    }
    out.temps[y*3+x]=total?sum/total:NAN;
    if(total)++valid;
  }
  if(valid<4){out.code=-209;return;}
  out.metar=true;out.frames=1;
}
static void wxWorker(void*){
  uint32_t meteoRetry=0;
  WxRequest job;
  for(;;)if(xQueueReceive(wxJobs,&job,portMAX_DELAY)){
    WxFrame result{};result.at=job;
    xSemaphoreTake(networkGate,portMAX_DELAY);
    if(job.layer==3)getRain(result);
    else{
      if(due(millis(),meteoRetry)){
        getHeat(result);
        if(result.code==429)meteoRetry=millis()+3600000UL;
      }
      if(!result.frames)getMetar(result);
    }
    xSemaphoreGive(networkGate);
    xQueueSend(wxReplies,&result,portMAX_DELAY);
  }
}
static bool weatherPage(){return page==3 || page==4;}
static WxRequest weatherView(){
  Area a=currentArea();WxRequest q{};q.layer=page;
  q.halfLat=min(20.f,viewRadius(weatherZoomStep)*1.852f/111.195f);
  q.lat=constrain((float)a.lat,-85.f+q.halfLat,85.f-q.halfLat);
  q.lon=a.lon;q.halfLon=q.halfLat*320.f/240.f/cosf(radians(q.lat));
  float cy=mercY(q.lat);
  float vertical=max(mercY(q.lat+q.halfLat)-cy,cy-mercY(q.lat-q.halfLat));
  q.z=constrain(int(floorf(log2f(.99f/max(q.halfLon/180.f,vertical/float(PI))))),0,7);
  return q;
}
static bool rainFits(const WxRequest &tile,const WxRequest &view){
  float count=1<<tile.z,cy=mercY(tile.lat);
  return (fabsf(remainderf(view.lon-tile.lon,360))+view.halfLon)*count<180.f &&
    (mercY(view.lat+view.halfLat)-cy)*count<PI && (cy-mercY(view.lat-view.halfLat))*count<PI;
}
static bool sameWx(const WxRequest &a,const WxRequest &b){
  if(a.layer==3 && b.layer==3)return a.z==b.z && rainFits(a,b);
  return a.layer==b.layer && a.z==b.z && fabsf(a.halfLat-b.halfLat)<.0001f &&
      fabsf(a.lat-b.lat)<b.halfLat*.05f && fabsf(remainderf(a.lon-b.lon,360))<b.halfLon*.05f;
}
static void weatherTick(){
  WxFrame reply;
  if(xQueueReceive(wxReplies,&reply,0)){
    wxBusy=false;int slot=reply.at.layer-3;auto &old=wxCache[slot];
    if(reply.code==200 && reply.frames){releaseWx(old);old=reply;rainIndex=-1;}
    else {releaseWx(reply);old.code=reply.code;}
    nextWx[slot]=millis()+(reply.code==200?600000:60000);dirty=true;
    Serial.printf("WEATHER layer=%d code=%d frames=%d epoch=%ld center=%.4f,%.4f\n",reply.at.layer,reply.code,reply.frames,(long)reply.stamp[0],reply.at.lat,reply.at.lon);
    if(reply.at.layer==4 && reply.frames)Serial.printf("HEAT source=%s center_C=%.1f\n",reply.metar?"METAR":"Open-Meteo",reply.temps[4]);
  }
  if(!weatherPage() || wxBusy || WiFi.status()!=WL_CONNECTED || time(nullptr)<1700000000)return;
  auto q=weatherView();auto &old=wxCache[page-3];
  if(!due(millis(),nextWx[page-3]) && (sameWx(old.at,q)||sameWx(lastWxJob,q)))return;
  if(millis()-lastWxAttempt<1500)return;
  if(xQueueSend(wxJobs,&q,0)){wxBusy=true;lastWxJob=q;lastWxAttempt=millis();}
}
static uint16_t heatColor(float fraction){
  const uint32_t stops[]={0x49538c,0x477cb5,0x55a1ad,0x8db898,0xd5c77c,0xe59b65,0xc9675e,0x944b67};
  float p=constrain(fraction*7,0.f,6.999f);int i=int(p);float f=p-i;
  uint32_t col=0;
  for(int shift:{16,8,0})col|=uint32_t(((stops[i]>>shift)&255)*(1-f)+((stops[i+1]>>shift)&255)*f)<<shift;
  return rgb565(col);
}
static void weatherMap(){
  WxRequest q=weatherView();auto &w=wxCache[page-3];
  MapView v{};v.lat=q.lat;v.lon=q.lon;v.nm=viewRadius(weatherZoomStep);v.pixelsPerDeg=120.f/q.halfLat;v.lonFactor=v.pixelsPerDeg*cosf(radians(v.lat));
  screen.fillSprite(land);
  bool show=w.frames && (page==3?rainFits(w.at,q):sameWx(w.at,q));
  float low=INFINITY,high=-INFINITY;
  if(show && page==4){
    for(float t:w.temps)if(isfinite(t)){low=min(low,t);high=max(high,t);}
    low=floorf(low);high=max(low+2,ceilf(high));
  }
  int frame=show && page==3?(rainIndex>=0?min(rainIndex,w.frames-1):playing?(millis()/1400)%w.frames:w.frames-1):0;
  if(show && page==3){
    int xs[320];float world=256.f*(1<<w.at.z);
    for(int x=0;x<320;x++)xs[x]=int(floorf(128+world*(remainderf(q.lon-w.at.lon,360)+(x-160)/v.lonFactor)/360));
    float centerMerc=mercY(w.at.lat);
    for(int y=0;y<240;y++){
      int sy=int(floorf(128+(centerMerc-mercY(q.lat+(120-y)/v.pixelsPerDeg))*world/(2*PI)));
      if(sy<0||sy>=256)continue;
      for(int x=0;x<320;x++)if(xs[x]>=0 && xs[x]<256){
        RainPixel pixel=w.rain[frame][sy*256+xs[x]];
        if(pixel.alpha)screen.drawPixel(x,y,rainOverLand(pixel,land));
      }
    }
  }
  if(show && page==4){
    for(int y=0;y<240;y+=4)for(int x=0;x<320;x+=4){
      float a=constrain(1+(q.lon-w.at.lon+(x-158)/v.lonFactor)/w.at.halfLon,0.f,1.9999f);
      float b=constrain(1+(w.at.lat-q.lat+(y-118)/v.pixelsPerDeg)/w.at.halfLat,0.f,1.9999f);
      int i=int(b)*3+int(a);a-=int(a);b-=int(b);
      float temp=(1-b)*(w.temps[i]*(1-a)+w.temps[i+1]*a)+b*(w.temps[i+3]*(1-a)+w.temps[i+4]*a);
      if(isfinite(temp))screen.fillRect(x,y,4,4,heatColor((temp-low)/(high-low)));
    }
  }
  const bool regional=q.lon-q.halfLon<15.5f || q.lon+q.halfLon>23.25f ||
                      q.lat-q.halfLat<45.25f || q.lat+q.halfLat>49.f;
  if(regional){
    v.units=2000.f;
    drawGeography(v,nullptr,reinterpret_cast<const MapHeader*>(flightdeck_overview_start));
  }else drawGeography(v,nullptr);
  if(show && page==4){
    for(int y=0;y<3;y++)for(int x=0;x<3;x++){
      int px=constrain(v.x(w.at.lon+(x-1)*w.at.halfLon)-23,3,270);
      int py=constrain(v.y(w.at.lat+(1-y)*w.at.halfLat)-8,38,188);
      if(isfinite(w.temps[y*3+x])){pill(px,py,46,17);label(px+4,py+4,String(w.temps[y*3+x],1)+" C",ink,1);}
    }
  }
  String desc=page==3?"RainViewer":show && w.metar?"METAR ~":"Open-Meteo";
  if(show){
    char t[9];tm local;localtime_r(&w.stamp[frame],&local);strftime(t,sizeof(t),"%H:%M",&local);
    desc+="  "+String(t);
    if(time(nullptr)-w.stamp[frame]>3600 || w.code!=200)desc+=tr("  Régi","  Stale");
    if(page==3){desc+="  "+String(frame+1)+"/"+String(w.frames);desc+=playing?"  II":"  >";}
  }else if(WiFi.status()!=WL_CONNECTED)desc+="  Offline";
  else if(wxBusy)desc+=tr("  Letöltés...","  Loading...");
  else if(w.code && w.code!=200)desc+="  "+String(w.code);
  else desc+=tr("  Várakozás","  Waiting");
  pill(4,209,page==4?243:196,27);label(10,218,desc,ink,1);
  if(show && page==4){
    for(int i=0;i<68;i++)screen.drawFastVLine(165+i,214,5,heatColor(i/67.f));
    label(165,222,String(int(low)),ink,1);label(203,222,String(int(high))+" C",ink,1);
  }
  zoomButtons(320);
}
