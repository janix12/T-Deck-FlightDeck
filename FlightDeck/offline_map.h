#pragma once

extern "C" const uint8_t flightdeck_map_start[],flightdeck_map_end[];
struct MapHeader {char magic[4]; uint32_t places,paths,points,names;};
struct MapPoint {int16_t x,y;};
struct MapPlace {MapPoint at; uint32_t name;};
struct MapPath {int16_t left,bottom,right,top; uint32_t first; uint16_t count,kind; uint32_t name;};
static_assert(sizeof(MapHeader)==20 && sizeof(MapPath)==20 && sizeof(MapPlace)==8,"map encoding");
static const MapHeader *atlas;
static uint8_t *mapBuffer;
static const MapPlace *places;
static const MapPath *paths;
static const MapPoint *vertices;
static const char *mapNames;
static const char *mapStatus="SD hianyzik";
static char mapFilename[48];

static size_t mapProgress;
static uint32_t storedLength, writtenLength;
static int mapReadCount, writeProtect, sameHandle;
static uint8_t ioCode,ioData;
static bool mapFromSD, installedMap;
static int diffByte,actualByte;
static const char *nativeStatus,*probeStatus;
static bool sdWriteFailed;
const char *nativeMap(const char*,const uint8_t*,size_t,uint8_t*,size_t&,bool&,int&,int&);

static bool readMapFile(uint8_t *data,size_t size){
  mapProgress=0; mapStatus="SD megnyitas";
  FsFile f=card.open(mapFilename,O_RDONLY);
  storedLength=0;mapReadCount=-1;
  if(!f) return false;
  storedLength=f.fileSize();
  bool ok=storedLength==size;
  mapStatus=ok?"SD olvasas":"SD fajlmeret";
  uint8_t buffer[512];
  while(ok && mapProgress<size){
    size_t n=std::min(sizeof(buffer),size-mapProgress);
    mapReadCount=f.read(buffer,n);
    if(mapReadCount!=int(n)){ok=false;break;}
    if(memcmp(buffer,flightdeck_map_start+mapProgress,n)){
      size_t i=0;while(buffer[i]==flightdeck_map_start[mapProgress+i]) ++i;
      diffByte=mapProgress+i;actualByte=buffer[i];mapStatus="SD adatelteres";ok=false;break;
    }
    memcpy(data+mapProgress,buffer,n);mapProgress+=n;
    if(!(mapProgress%8192)) delay(1);
  }
  f.close();
  return ok;
}
static bool writeMapRange(size_t begin,size_t end){
  mapProgress=begin;mapStatus="SD letrehozas";
  FsFile f=card.open(mapFilename,begin?O_RDWR|O_APPEND:O_RDWR|O_CREAT|O_TRUNC);
  if(!f) return false;
  uint8_t buffer[512];
  bool ok=f.fileSize()==begin;mapStatus="SD iras";
  while(ok && mapProgress<end){
    size_t n=std::min(sizeof(buffer),end-mapProgress);
    memcpy(buffer,flightdeck_map_start+mapProgress,n);
    if(f.write(buffer,n)!=n){ok=false;break;}
    mapProgress+=n;
    if(!(mapProgress%8192)) delay(1);
  }
  writtenLength=f.fileSize();
  if(ok){mapStatus="SD szinkron";ok=f.sync();}
  if(!f.close() && ok){mapStatus="SD lezaras";ok=false;}
  return ok;
}
static bool installMap(uint8_t *data,size_t size){
  // Remount before reading so success cannot come from the filesystem cache.
  const size_t prefix=37;
  if(!writeMapRange(0,prefix)) return false;
  mountCard();
  if(!sdOK){mapStatus="SD csatolas";return false;}
  if(!readMapFile(data,prefix)) return false;
  sameHandle=prefix;
  if(!writeMapRange(prefix,size)) return false;
  mountCard();
  if(!sdOK){mapStatus="SD csatolas";return false;}
  return readMapFile(data,size);
}
static void loadMap(bool retry){
  if(mapBuffer){free(mapBuffer);mapBuffer=nullptr;}
  const size_t size=uintptr_t(flightdeck_map_end)-uintptr_t(flightdeck_map_start);
  uint32_t hash=2166136261u;
  for(size_t i=0;i<size;i++) hash=(hash^flightdeck_map_start[i])*16777619u;
  snprintf(mapFilename,sizeof(mapFilename),"/%08lX.FDM",(unsigned long)hash);
  mapProgress=storedLength=writtenLength=0;mapReadCount=sameHandle=writeProtect=-1;
  mapFromSD=installedMap=false;ioCode=ioData=0;mapStatus=cardMiB?"SD kotet hiba":"Nincs SD";
  nativeStatus=probeStatus="--";sdWriteFailed=false;diffByte=actualByte=-1;
  if(sdOK){
    csd_t csd;
    if(card.card()->readCSD(&csd)) writeProtect=csd.permWriteProtect()*2+csd.tempWriteProtect();
    mapBuffer=(uint8_t*)ps_malloc(size);
    mapStatus="Keves memoria";
    if(mapBuffer){
      mapFromSD=readMapFile(mapBuffer,size);
      cid_t cid;
      bool hasId=card.card()->readCID(&cid);
      uint32_t cardKey=hasId?(cid.psn()^hash):0;
      Preferences cache;
      bool havePrefs=cache.begin("fdmap",false);
      bool previousError=cardKey && havePrefs && cache.getUInt("failed",0)==cardKey;
      bool attempt=!mapFromSD && writeProtect<=0 && (retry || !previousError);
      if(writeProtect>0 && !mapFromSD) mapStatus="SD irasvedett";
      if(previousError && !retry && !mapFromSD) mapStatus="SD visszaolvasas";
      if(attempt){
        mapFromSD=installMap(mapBuffer,size);installedMap=mapFromSD;probeStatus=mapFromSD?"OK":mapStatus;
      }
      if(attempt && !mapFromSD){
        ioCode=card.sdErrorCode();ioData=card.sdErrorData();
        if(writeProtect<=0){
          card.end();
          SPI.begin(BOARD_SPI_SCK,BOARD_SPI_MISO,BOARD_SPI_MOSI);
          bool created=false;
          const char *error=nativeMap(mapFilename,flightdeck_map_start,size,mapBuffer,mapProgress,created,diffByte,actualByte);
          nativeStatus=error?error:"OK";
          mountCard();mapFromSD=!error;installedMap=created;
          if(error) mapStatus=error;
        }
      }
      sdWriteFailed=!mapFromSD && (attempt || previousError || writeProtect>0);
      if(havePrefs){
        if(mapFromSD && previousError) cache.remove("failed");
        if(!mapFromSD && attempt && cardKey && !previousError) cache.putUInt("failed",cardKey);
        cache.end();
      }
      if(!mapFromSD){free(mapBuffer);mapBuffer=nullptr;}
    }
  }
  atlas=reinterpret_cast<const MapHeader*>(mapFromSD?mapBuffer:flightdeck_map_start);
  places=reinterpret_cast<const MapPlace*>(atlas+1);
  paths=reinterpret_cast<const MapPath*>(places+atlas->places);
  vertices=reinterpret_cast<const MapPoint*>(paths+atlas->paths);
  mapNames=reinterpret_cast<const char*>(vertices+atlas->points);
  if(mapFromSD) mapStatus="SD OK";
}

static bool inRing(const MapPath &p,float x,float y){
  if(x<p.left || x>p.right || y<p.bottom || y>p.top) return false;
  bool inside=false;
  const MapPoint *v=vertices+p.first;
  for(int i=0,j=p.count-1;i<p.count;j=i++){
    if((v[i].y>y)!=(v[j].y>y) && x<(v[j].x-v[i].x)*(y-v[i].y)/(v[j].y-v[i].y)+v[i].x) inside=!inside;
  }
  return inside;
}
static const char *placeBelow(const Plane &p,bool &within,float &distance){
  within=false; distance=INFINITY;
  if(!atlas || p.lon<15.5f || p.lon>23.25f || p.lat<45.25f || p.lat>49) return nullptr;
  const float x=(p.lon-19)*5000,y=(p.lat-47)*5000;
  const char *name=nullptr; long smallest=LONG_MAX;
  for(unsigned i=0;i<atlas->paths;i++){
    const MapPath &s=paths[i];
    if(s.kind!=4 || !inRing(s,x,y)) continue;
    bool hole=false;
    for(unsigned j=i+1;j<atlas->paths && paths[j].kind==5;j++) if(inRing(paths[j],x,y)) hole=true;
    long bounds=long(s.right-s.left)*(s.top-s.bottom);
    if(!hole && bounds<smallest){smallest=bounds;name=mapNames+s.name;}
  }
  if(name){within=true;distance=0;return name;}
  const float lonScale=cosf(radians(p.lat));
  for(unsigned i=0;i<atlas->places;i++){
    const MapPlace &s=places[i];
    float dx=(s.at.x-x)*lonScale,dy=s.at.y-y,d=dx*dx+dy*dy;
    if(d<distance){distance=d;name=mapNames+s.name;}
  }
  distance=sqrtf(distance)*111.195f/5000;
  return name;
}
