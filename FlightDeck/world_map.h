#pragma once
#include <esp32s3/rom/miniz.h>
#include <esp_rom_crc.h>
#include <esp_partition.h>

namespace world {
constexpr int detailCount=180*90, hungaryKey=detailCount+36*18, tileCount=hungaryKey+2;
constexpr size_t tileLimit=256*1024, cacheLimit=1024*1024;
// Optional installation on a specifically provisioned board, within its data partition.
constexpr size_t flashBase=0xc0000,flashCapacity=0x5c0000;
static const esp_partition_t *partition(){
  auto p=esp_partition_find_first(ESP_PARTITION_TYPE_DATA,ESP_PARTITION_SUBTYPE_DATA_FAT,"ffat");
  return p && p->size>=flashBase+flashCapacity?p:nullptr;
}
constexpr char filename[]="/WORLD.FDM";
static uint32_t *offsets;
static const esp_partition_t *flash;
static size_t cacheBytes;
static uint32_t tick;
struct Entry {int key=-1; uint32_t use=0; MapHeader *map=nullptr; size_t size=0;};
static Entry cache[40];
static char nearest[96],urban[96];

static void discard(Entry &item){
  free(item.map);cacheBytes-=item.size;item=Entry{};
}
static bool readAt(uint32_t start,void *dest,size_t size){
  if(flash)return esp_partition_read(flash,flashBase+start,dest,size)==ESP_OK;
  FsFile file=card.open(filename,O_RDONLY);
  return file && file.seekSet(start) && file.read(dest,size)==int(size);
}
static bool loadIndex(size_t available,bool exact){
  struct Header {char magic[4];uint32_t length,count,crc;} header;
  if(!readAt(0,&header,sizeof(header)) || memcmp(header.magic,"FDW2",4) || header.count!=tileCount ||
     header.length>available || (exact && header.length!=available))return false;
  size_t bytes=(tileCount+1)*sizeof(uint32_t);
  auto index=static_cast<uint32_t*>(ps_malloc(bytes));
  if(!index)return false;
  bool ok=readAt(sizeof(header),index,bytes) && index[0]==sizeof(header)+bytes &&
          index[tileCount]==header.length && esp_rom_crc32_le(0,reinterpret_cast<uint8_t*>(index),bytes)==header.crc;
  for(int i=0;ok && i<tileCount;i++)ok=index[i+1]>=index[i] && index[i+1]-index[i]<=(i<hungaryKey?tileLimit+1024:2*1024*1024);
  if(ok)offsets=index;else free(index);
  return ok;
}
static void clear(){
  for(auto &item:cache)discard(item);
  free(offsets);offsets=nullptr;flash=nullptr;nearest[0]=urban[0]=0;
}
static Entry &oldest(){
  Entry *old=cache;
  for(auto &item:cache){
    if(item.key<0)return item;
    if(item.use<old->use)old=&item;
  }
  return *old;
}
static bool valid(const MapHeader &h,size_t bytes){
  uint64_t length=sizeof(h)+uint64_t(h.places)*sizeof(MapPlace)+uint64_t(h.paths)*sizeof(MapPath)+
                  uint64_t(h.points)*sizeof(MapPoint)+h.names;
  if(memcmp(h.magic,"FDM1",4) || length!=bytes || !h.names)return false;
  auto towns=reinterpret_cast<const MapPlace*>(&h+1);
  auto lines=reinterpret_cast<const MapPath*>(towns+h.places);
  auto points=reinterpret_cast<const MapPoint*>(lines+h.paths);
  auto names=reinterpret_cast<const char*>(points+h.points);
  if(names[h.names-1])return false;
  for(unsigned i=0;i<h.places;i++)if(towns[i].name>=h.names)return false;
  for(unsigned i=0;i<h.paths;i++){
    const auto &p=lines[i];
    if(p.name>=h.names || p.count<2 || p.first>h.points || p.count>h.points-p.first)return false;
  }
  return true;
}
static bool unpack(void *map,size_t bytes,const uint8_t *source,size_t length){
  // The inflater's Huffman tables exceed the Arduino loop task's stack.
  auto decoder=static_cast<tinfl_decompressor*>(ps_malloc(sizeof(tinfl_decompressor)));
  if(!decoder)return false;
  tinfl_init(decoder);
  size_t in=length,out=bytes;
  auto dest=reinterpret_cast<uint8_t*>(map);
  auto result=tinfl_decompress(decoder,source,&in,dest,dest,&out,
                               TINFL_FLAG_PARSE_ZLIB_HEADER|TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
  free(decoder);
  return result==TINFL_STATUS_DONE && in==length && out==bytes;
}
static MapHeader *decode(const uint8_t *stream,size_t bytes,size_t &allocated){
  MapHeader h;
  if(bytes<sizeof(h))return nullptr;
  memcpy(&h,stream,sizeof(h));
  uint64_t total=sizeof(h)+uint64_t(h.places)*8+uint64_t(h.paths)*20+uint64_t(h.points)*4+h.names;
  if(memcmp(h.magic,"FDM1",4) || total>tileLimit || !h.names || h.names>bytes-sizeof(h))return nullptr;
  auto out=static_cast<MapHeader*>(ps_malloc(total));
  if(!out)return nullptr;
  *out=h;
  auto towns=reinterpret_cast<MapPlace*>(out+1);
  auto lines=reinterpret_cast<MapPath*>(towns+h.places);
  auto pts=reinterpret_cast<MapPoint*>(lines+h.paths);
  auto cur=stream+sizeof(h),end=stream+bytes-h.names;
  bool ok=true;
  auto var=[&]()->unsigned{
    unsigned v=0;
    for(int shift=0;cur<end && shift<=21;shift+=7){
      unsigned b=*cur++;v|=(b&127)<<shift;if(b<128)return v;
    }
    ok=false;return 0;
  };
  for(unsigned i=0;ok && i<h.places;i++){
    if(end-cur<4){ok=false;break;}
    memcpy(&towns[i].at,cur,4);cur+=4;towns[i].name=var();
  }
  unsigned n=0;
  for(unsigned i=0;ok && i<h.paths;i++){
    if(cur==end){ok=false;break;}
    MapPath &p=lines[i];p.kind=*cur++;
    auto count=var();if(count<2 || count>65535 || count>h.points-n){ok=false;break;}
    p.count=count;p.first=n;p.name=p.kind>=6?var():0;
    p.left=p.bottom=32767;p.right=p.top=-32768;
    int x=0,y=0;
    auto delta=[&](){int v=var();return (v&1)?-(v/2)-1:v/2;};
    for(unsigned j=0;ok && j<count;j++){
      x+=delta();y+=delta();
      if(abs(x)>5001 || abs(y)>5001){ok=false;break;}
      auto &q=pts[n++];q.x=x*5;q.y=y*5;
      p.left=std::min(p.left,q.x);p.right=std::max(p.right,q.x);
      p.bottom=std::min(p.bottom,q.y);p.top=std::max(p.top,q.y);
    }
  }
  if(ok && n==h.points && cur==end){
    memcpy(pts+n,end,h.names);
    if(valid(*out,total)){allocated=total;return out;}
  }
  free(out);return nullptr;
}
static MapHeader *readSection(int key,size_t &allocated){
  size_t size=offsets[key+1]-offsets[key];
  if(size<=4)return nullptr;
  auto packed=static_cast<uint8_t*>(ps_malloc(size));
  if(!packed)return nullptr;
  uint32_t bytes=0;
  if(readAt(offsets[key],packed,size))memcpy(&bytes,packed,4);
  MapHeader *map=nullptr;
  if(bytes>=sizeof(MapHeader) && bytes<=(key<hungaryKey?tileLimit:2*1024*1024)){
    auto raw=static_cast<MapHeader*>(ps_malloc(bytes));
    if(raw && unpack(raw,bytes,packed+4,size-4)){
      if(key<hungaryKey)map=decode(reinterpret_cast<uint8_t*>(raw),bytes,allocated);
      else if(valid(*raw,bytes)){map=raw;raw=nullptr;allocated=bytes;}
    }
    free(raw);
  }
  free(packed);return map;
}
static const MapHeader *tile(int key){
  if(!offsets || offsets[key]==offsets[key+1])return nullptr;
  for(auto &item:cache)if(item.key==key){item.use=++tick;return item.map;}
  size_t bytes=0;
  MapHeader *map=readSection(key,bytes);
  if(map){
    while(cacheBytes+bytes>cacheLimit){
      Entry *old=nullptr;
      for(auto &item:cache)if(item.map && (!old || item.use<old->use))old=&item;
      if(!old)break;
      discard(*old);
    }
  }
  Entry &item=oldest();discard(item);
  item.key=key;item.use=++tick;item.map=map;item.size=map?bytes:0;cacheBytes+=item.size;
  return map;
}
template<class Visit> static void visit(float lon,float lat,float dx,float dy,int step,Visit fn){
  if(!offsets || !isfinite(lon) || !isfinite(lat))return;
  const int nx=360/step,ny=180/step;
  dx=std::min(dx,180.f);
  int x0=floorf((lon-dx+180)/step),x1=floorf((lon+dx+180)/step);
  int y0=constrain(int(floorf((lat-dy+90)/step)),0,ny-1);
  int y1=constrain(int(floorf((lat+dy+90)/step)),0,ny-1);
  for(int y=y0;y<=y1;y++)for(int k=x0;k<=std::min(x1,x0+nx-1);k++){
    int x=(k%nx+nx)%nx;
    auto data=tile((step==2?0:detailCount)+y*nx+x);
    if(data)fn(*data,(x+.5f)*step-180,(y+.5f)*step-90);
  }
}
static bool contains(const MapPath &ring,const MapPoint *points,float x,float y){
  if(x<ring.left || x>ring.right || y<ring.bottom || y>ring.top)return false;
  auto p=points+ring.first;bool hit=false;
  for(int i=0,j=ring.count-1;i<ring.count;j=i++)
    if((p[i].y>y)!=(p[j].y>y) && x<(p[j].x-p[i].x)*(y-p[i].y)/(p[j].y-p[i].y)+p[i].x)hit=!hit;
  return hit;
}
static const char *locate(float lat,float lon,float &distance){
  nearest[0]=urban[0]=0;
  constexpr float radius=30.f,dy=radius/111.195f;
  float dx=fabsf(lat)+dy>=90?180.f:degrees(asinf(std::min(1.f,sinf(radians(dy))/cosf(radians(lat)))));
  float best=sq(sinf(radians(dy)*.5f));
  visit(lon,lat,dx,dy,2,[&](const MapHeader &h,float cx,float cy){
    auto towns=reinterpret_cast<const MapPlace*>(&h+1);
    auto lines=reinterpret_cast<const MapPath*>(towns+h.places);
    auto points=reinterpret_cast<const MapPoint*>(lines+h.paths);
    auto names=reinterpret_cast<const char*>(points+h.points);
    float x=remainderf(lon-cx,360.f)*5000,y=(lat-cy)*5000;
    if(fabsf(x)<=5001 && fabsf(y)<=5001){
      for(unsigned i=0;i<h.paths;i++)if(lines[i].kind==6 && contains(lines[i],points,x,y)){
        bool hole=false;
        for(unsigned j=i+1;j<h.paths && lines[j].kind==7;j++)if(contains(lines[j],points,x,y))hole=true;
        if(!hole){strlcpy(urban,names+lines[i].name,sizeof(urban));break;}
      }
    }
    for(unsigned i=0;i<h.places;i++){
      const auto &t=towns[i];
      float dlat=(t.at.y-y)/5000,dlon=(t.at.x-x)/5000;
      if(fabsf(dlat)>dy || fabsf(dlon)>dx)continue;
      float d=sq(sinf(radians(dlat)*.5f))+cosf(radians(lat))*cosf(radians(lat+dlat))*sq(sinf(radians(dlon)*.5f));
      if(d<=best){best=d;strlcpy(nearest,names+t.name,sizeof(nearest));}
    }
  });
  distance=nearest[0]?12742.f*asinf(sqrtf(std::max(0.f,best))):INFINITY;
  return nearest[0]?nearest:nullptr;
}
}
