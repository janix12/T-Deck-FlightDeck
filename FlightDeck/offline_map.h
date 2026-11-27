#pragma once

struct MapHeader {char magic[4]; uint32_t places,paths,points,names;};
struct MapPoint {int16_t x,y;};
struct MapPlace {MapPoint at; uint32_t name;};
struct MapPath {int16_t left,bottom,right,top; uint32_t first; uint16_t count,kind; uint32_t name;};
static_assert(sizeof(MapHeader)==20 && sizeof(MapPath)==20 && sizeof(MapPlace)==8,"map encoding");
#include "world_map.h"
#include <mbedtls/sha256.h>
struct MapRecord {uint32_t size,revision; char sha[65];};
static const MapRecord initialMap={5835336,1,"34c356cd11ecd11c94420544e667e7eec92fdf56cacf239edf5008ba7faa5924"};
static MapRecord installedRecord{};
static MapHeader *atlas,*overviewAtlas;
static const MapPlace *places;
static const MapPath *paths;
static const MapPoint *vertices;
static const char *mapNames;
static bool sdWriteFailed,internalEnabled,mapConsent,mapPrompt,mapBusy,mapRecheck,checkSdCopy;
static uint32_t mapNext;
static Preferences mapPrefs;
static bool mapPrefsOpen;

static void closeAtlas(){
  free(atlas);free(overviewAtlas);atlas=overviewAtlas=nullptr;
  world::clear();installedRecord={};
}
static bool mapHash(FsFile *f,const esp_partition_t *part,const MapRecord &r){
  if(!r.size || (!f && (!part || r.size>world::flashCapacity)) || (f && f->fileSize()!=r.size))return false;
  mbedtls_sha256_context ctx;mbedtls_sha256_init(&ctx);mbedtls_sha256_starts_ret(&ctx,0);
  uint8_t block[1024],sum[32];char hex[65];bool ok=true;
  for(size_t at=0;at<r.size && ok;){
    size_t n=std::min(sizeof(block),size_t(r.size-at));
    ok=f?f->read(block,n)==int(n):esp_partition_read(part,world::flashBase+at,block,n)==ESP_OK;
    if(ok)mbedtls_sha256_update_ret(&ctx,block,n);
    at+=n;if(at%65536==0)delay(1);
  }
  mbedtls_sha256_finish_ret(&ctx,sum);mbedtls_sha256_free(&ctx);
  for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",sum[i]);
  return ok && !strncmp(hex,r.sha,sizeof(hex));
}
static bool activateMap(const MapRecord &r,const esp_partition_t *part){
  closeAtlas();world::flash=part;
  if(!world::loadIndex(r.size,true)){closeAtlas();return false;}
  size_t size=0;
  atlas=world::readSection(world::hungaryKey,size);
  overviewAtlas=world::readSection(world::hungaryKey+1,size);
  if(!atlas || !overviewAtlas){closeAtlas();return false;}
  places=reinterpret_cast<const MapPlace*>(atlas+1);
  paths=reinterpret_cast<const MapPath*>(places+atlas->places);
  vertices=reinterpret_cast<const MapPoint*>(paths+atlas->paths);
  mapNames=reinterpret_cast<const char*>(vertices+atlas->points);
  installedRecord=r;return true;
}
static void loadMap(bool retry){
  if(mapBusy)return;
  if(!mapPrefsOpen)mapPrefsOpen=mapPrefs.begin("fdworld",false);
  internalEnabled=mapPrefs.getBool("reserve",false);
  mapConsent=mapPrefs.getBool("consent",false);
  MapRecord r=initialMap;
  if(mapPrefs.getBytesLength("record")==sizeof(r))mapPrefs.getBytes("record",&r,sizeof(r));
  r.sha[64]=0;
  MapRecord pending{};
  if(mapPrefs.getBytesLength("pending")==sizeof(pending))mapPrefs.getBytes("pending",&pending,sizeof(pending));
  pending.sha[64]=0;
  closeAtlas();
  if(sdOK){
    if(!card.exists(world::filename) && card.exists("/WORLD.BAK"))card.rename("/WORLD.BAK",world::filename);
    for(const MapRecord *record:{&r,&pending}){
      if(!record->size)continue;
      FsFile f=card.open(world::filename,O_RDONLY);
      if(f && mapHash(&f,nullptr,*record)){f.close();if(activateMap(*record,nullptr))break;}
    }
  }
  if(!atlas && internalEnabled){
    auto part=world::partition();
    for(const MapRecord *record:{&r,&pending})
      if(part && record->size && mapHash(nullptr,part,*record) && activateMap(*record,part))break;
  }
  if(atlas){
    if(!mapConsent){mapConsent=true;mapPrefs.putBool("consent",true);}
    if(pending.size && !strcmp(installedRecord.sha,pending.sha) &&
       mapPrefs.putBytes("record",&installedRecord,sizeof(r))==sizeof(r))mapPrefs.remove("pending");
  }
  mapPrompt=!atlas && (!mapConsent || retry);
  checkSdCopy=sdOK && world::flash;
  mapNext=0;mapRecheck=true;
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
  within=false; distance=INFINITY;world::urban[0]=0;
  if(!atlas || p.lon<15.5f || p.lon>23.25f || p.lat<45.25f || p.lat>49)
    return world::locate(p.lat,p.lon,distance);
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
  if(world::offsets)return world::locate(p.lat,p.lon,distance);
  const float lonScale=cosf(radians(p.lat));
  for(unsigned i=0;i<atlas->places;i++){
    const MapPlace &s=places[i];
    float dx=(s.at.x-x)*lonScale,dy=s.at.y-y,d=dx*dx+dy*dy;
    if(d<distance){distance=d;name=mapNames+s.name;}
  }
  distance=sqrtf(distance)*111.195f/5000;
  return name;
}
