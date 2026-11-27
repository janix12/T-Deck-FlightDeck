#pragma once
namespace mapUpdate {
constexpr char base[]="https://janix12.github.io/T-Deck-FlightDeck/maps/";
enum Event {prepare,chunk,finish,done};
struct Message {Event type;MapRecord record;const uint8_t *data;size_t length;int error;TaskHandle_t sender;};
static QueueHandle_t queue;
static bool later,showProgress,flashTarget,sdFailed;
static FsFile stage;
static size_t written,erased;
static MapRecord offer;
static unsigned failures;
static int lastError;
static const esp_partition_t *copySource;
static FsFile sdSource;
static cid_t transferCid;
static bool transferHasCid;
static constexpr char temp[]="/WORLD.PRT",backup[]="/WORLD.BAK";

static bool notify(Event event,const MapRecord &r,const uint8_t *p=nullptr,size_t n=0,int err=0){
  Message msg{event,r,p,n,err,xTaskGetCurrentTaskHandle()};
  xQueueSend(queue,&msg,portMAX_DELAY);
  uint32_t ok=0;xTaskNotifyWait(0,UINT32_MAX,&ok,portMAX_DELAY);return ok==1;
}
static esp_http_client_handle_t openUrl(const String &url,int64_t &length){
  esp_http_client_config_t cfg{};cfg.url=url.c_str();cfg.cert_pem=weatherRoots;
  cfg.timeout_ms=15000;cfg.buffer_size=8192;cfg.disable_auto_redirect=true;
  auto http=esp_http_client_init(&cfg);if(!http)return nullptr;
  esp_http_client_set_header(http,"Accept-Encoding","identity");
  if(esp_http_client_open(http,0)==ESP_OK){
    length=esp_http_client_fetch_headers(http);
    if(esp_http_client_get_status_code(http)==200)return http;
  }
  esp_http_client_cleanup(http);return nullptr;
}
static int fetch(MapRecord &r,const MapRecord &local){
  int64_t len=0;
  auto http=openUrl(String(base)+"world.json",len);if(!http)return 1;
  char json[512];
  int n=esp_http_client_read_response(http,json,sizeof(json)-1);
  bool complete=esp_http_client_is_complete_data_received(http);
  esp_http_client_cleanup(http);
  if(!complete || n<=0 || n>=int(sizeof(json)-1))return 2;
  json[n]=0;JsonDocument doc;
  if(deserializeJson(doc,json) || strcmp(doc["format"]|"","FDW2"))return 2;
  const char *hash=doc["sha256"]|"";
  r.size=doc["size"]|0u;r.revision=doc["revision"]|0u;
  if(strlen(hash)!=64 || strspn(hash,"0123456789abcdef")!=64 || !r.revision ||
     r.size<68000 || r.size>64*1024*1024)return 2;
  strlcpy(r.sha,hash,sizeof(r.sha));
  if(local.revision>r.revision)return 0;
  if(local.revision==r.revision && local.size==r.size && !strcmp(local.sha,r.sha))return 0;
  http=openUrl(String(base)+"WORLD.FDM?sha256="+hash,len);if(!http)return 3;
  int error=0;
  if((len>0 && len!=r.size) || !notify(prepare,r))error=4;
  auto block=static_cast<uint8_t*>(malloc(8192));
  if(!block)error=5;
  size_t received=0;
  while(!error && received<r.size){
    int need=std::min(size_t(8192),size_t(r.size-received));
    int have=0;
    while(have<need){
      int got=esp_http_client_read(http,reinterpret_cast<char*>(block+have),need-have);
      if(got<=0){error=6;break;}
      have+=got;
    }
    if(!error && !notify(chunk,r,block,have))error=7;
    received+=have;
  }
  free(block);esp_http_client_cleanup(http);
  if(!error && !notify(finish,r))error=8;
  return error;
}
static void worker(void *arg){
  MapRecord local=*static_cast<MapRecord*>(arg);delete static_cast<MapRecord*>(arg);
  MapRecord r{};
  xSemaphoreTake(networkGate,portMAX_DELAY);
  int error=fetch(r,local);
  xSemaphoreGive(networkGate);notify(done,r,nullptr,0,error);vTaskDelete(nullptr);
}
static bool sdStage(){
  if(!sdOK || sdFailed)return false;
  // A successful write alone is insufficient on cards that silently lose sectors.
  uint32_t sample[8],copy[8];for(auto &v:sample)v=esp_random();
  stage=card.open(temp,O_RDWR|O_CREAT|O_TRUNC);
  bool ok=stage && stage.write(sample,sizeof(sample))==sizeof(sample) && stage.sync();
  stage.close();
  if(ok){
    mountCard();
    stage=card.open(temp,O_RDONLY);
    ok=sdOK && stage && stage.read(copy,sizeof(copy))==sizeof(copy) && !memcmp(copy,sample,sizeof(copy));
    stage.close();
  }
  if(ok)stage=card.open(temp,O_RDWR|O_TRUNC);
  if(ok && stage){transferHasCid=haveMountedCid;transferCid=mountedCid;}
  sdFailed=!ok || !stage;return !sdFailed;
}
static bool begin(const MapRecord &r){
  offer=r;written=erased=0;
  bool canUseFlash=internalEnabled && world::partition() && r.size<=world::flashCapacity;
  flashTarget=canUseFlash && mapStorage!=MapStorage::sd;
  if(!flashTarget && !sdStage()){
    if(!canUseFlash)return false;
    flashTarget=true;
  }
  if(mapPrefs.putBytes("pending",&r,sizeof(r))!=sizeof(r))return false;
  if(flashTarget && world::flash)closeAtlas();
  mapPrompt=false;showProgress=!atlas;dirty=true;return true;
}
static bool writeBlock(const uint8_t *p,size_t n){
  if(n>offer.size-written)return false;
  if(flashTarget){
    auto part=world::partition();size_t end=(written+n+4095)&~size_t(4095);
    if(end>erased && esp_partition_erase_range(part,world::flashBase(part)+erased,end-erased)!=ESP_OK)return false;
    erased=end;
    if(esp_partition_write(part,world::flashBase(part)+written,p,n)!=ESP_OK)return false;
  }else if(stage.write(p,n)!=n){sdFailed=true;return false;}
  written+=n;return true;
}
static bool reopenStage(FsFile &file){
  bool flushed=stage.sync();stage.close();if(!flushed)return false;
  mountCard();
  if(!sdOK || (transferHasCid && (!haveMountedCid || memcmp(&mountedCid,&transferCid,sizeof(cid_t)))))return false;
  file=card.open(temp,O_RDONLY);
  return file && file.fileSize()==offer.size;
}
static bool commit(bool sdVerified=false){
  if(written!=offer.size)return false;
  auto part=flashTarget?world::partition():nullptr;
  if(!flashTarget){
    if(!sdVerified){
      FsFile f;
      if(!reopenStage(f) || !mapHash(&f,nullptr,offer)){sdFailed=true;return false;}
      f.close();
    }
    if(card.exists(backup) && card.exists(world::filename) && !card.remove(backup))return false;
    if(card.exists(world::filename) && !card.rename(world::filename,backup))return false;
    if(!card.rename(temp,world::filename)){
      if(card.exists(backup))card.rename(backup,world::filename);
      return false;
    }
  }else if(!mapHash(nullptr,part,offer))return false;
  bool keepInternal=!flashTarget && mapStorage==MapStorage::internal && world::flash &&
                    !strcmp(offer.sha,installedRecord.sha);
  if(!keepInternal && !activateMap(offer,part))return false;
  if(flashTarget){
    mapPrefs.putBytes("flashMap",&offer,sizeof(offer));
    checkSdCopy=sdOK && !sdSource && !sdFailed;
  }
  if(mapPrefs.putBytes("record",&offer,sizeof(offer))!=sizeof(offer))return false;
  mapPrefs.remove("pending");
  if(!flashTarget && card.exists(backup))card.remove(backup);
  if(!flashTarget)storageNeedsScan=true;
  return true;
}
static void copyLocalMap(){
  static FsFile readback;
  static mbedtls_sha256_context digest;
  static size_t verified;
  if(copyToInternal && !mapBusy){
    copyToInternal=false;
    sdSource=card.open(world::filename,O_RDONLY);
    if(!sdSource)return;
    offer=installedRecord;written=erased=0;flashTarget=true;
    if(mapPrefs.putBytes("pending",&offer,sizeof(offer))!=sizeof(offer)){sdSource.close();return;}
    mapBusy=true;
  }
  if(checkSdCopy && !mapBusy){
    checkSdCopy=false;sdFailed=false;
    if(!world::flash || !sdStage())return;
    copySource=world::flash;offer=installedRecord;written=0;flashTarget=false;mapBusy=true;
  }
  if(!copySource && !sdSource)return;
  static uint8_t block[4096];
  size_t n=std::min(sizeof(block),size_t(offer.size-written));
  bool ok=true;
  if(written<offer.size){
    ok=copySource?esp_partition_read(copySource,world::flashBase(copySource)+written,block,n)==ESP_OK:
                  sdSource.read(block,n)==int(n);
    ok=ok && writeBlock(block,n);
    if(ok && written<offer.size)return;
    if(ok && copySource){
      ok=reopenStage(readback);
      if(ok){verified=0;mbedtls_sha256_init(&digest);mbedtls_sha256_starts_ret(&digest,0);return;}
    }
  }else if(copySource){
    size_t chunk=std::min(sizeof(block),size_t(offer.size-verified));
    ok=readback.read(block,chunk)==int(chunk) && !mbedtls_sha256_update_ret(&digest,block,chunk);
    if(ok){verified+=chunk;if(verified<offer.size)return;}
    ok=digestMatches(digest,offer) && ok;
  }
  readback.close();
  ok=ok && commit(copySource!=nullptr);
  Serial.printf("WorldCopy %s verified=%d\n",flashTarget?"flash":"SD",ok);
  sdSource.close();stage.close();copySource=nullptr;mapBusy=false;sdFailed=!ok;
  dirty=true;
}
static void pollStorage(){
  static uint32_t nextProbe;
  if(mapBusy)return;
  if(reloadStorage){reloadStorage=false;loadMap(false);dirty=true;}
  if(!due(millis(),nextProbe))return;
  nextProbe=millis()+2500;
  if(sdOK){
    cid_t id;
    if(card.card()->readCID(&id) && (!haveMountedCid || !memcmp(&id,&mountedCid,sizeof(id)))){
      mountedCid=id;haveMountedCid=true;return;
    }
    sdOK=false;haveMountedCid=false;cardMiB=volumeMiB=volumeBytes=0;sdFreeBytes=-1;
    loadMap(false);dirty=true;
    Serial.println("WorldCard removed");
  }else{
    if(world::offsets && !world::flash){loadMap(false);dirty=true;}
    if(!cardResponds())return;
    mountCard();
    if(!sdOK){nextProbe=millis()+10000;return;}
    sdFailed=sdWriteFailed=false;loadMap(false);dirty=true;
    Serial.println("WorldCard inserted");
  }
}
static void tick(){
  pollStorage();copyLocalMap();
  if(!queue)queue=xQueueCreate(1,sizeof(Message));
  Message m;
  while(queue && xQueueReceive(queue,&m,0)==pdTRUE){
    bool ok=true;
    switch(m.type){
      case prepare:ok=begin(m.record);break;
      case chunk:ok=writeBlock(m.data,m.length);break;
      case finish:ok=commit();break;
      case done:
        stage.close();mapBusy=false;showProgress=false;lastError=m.error;
        if(lastError){failures=std::min(5u,failures+1);mapNext=millis()+(30000UL<<(failures-1));if(!atlas)mapPrompt=true;}
        else{failures=0;mapNext=millis()+86400000UL;}
        Serial.printf("WorldSync rev=%u installed=%u bytes=%u error=%d\n",m.record.revision,installedRecord.revision,unsigned(written),lastError);
        dirty=true;break;
    }
    xTaskNotify(m.sender,ok?1:2,eSetValueWithOverwrite);
  }
  if(!queue || mapBusy || (!atlas && !mapConsent) || WiFi.status()!=WL_CONNECTED ||
     time(nullptr)<1700000000 || (!mapRecheck && !due(millis(),mapNext)))return;
  // Revalidate stored bytes daily, including changes made to a removable card.
  if(!mapRecheck && !failures && atlas)loadMap(false);
  auto local=new(std::nothrow) MapRecord(installedRecord);
  if(!local)return;
  mapRecheck=false;mapBusy=true;
  if(xTaskCreatePinnedToCore(worker,"map-update",14336,local,1,nullptr,0)!=pdPASS){
    delete local;mapBusy=false;mapNext=millis()+30000;
  }
}
static void choose(bool accept){
  mapPrompt=false;later=false;dirty=true;
  if(accept){mapConsent=true;mapPrefs.putBool("consent",true);mapRecheck=true;sdFailed=false;lastError=0;}
}
static void dialog(){
  framedBox(17,52,286,136,8,bg,mint);
  centeredLabel(23,60,274,24,tr("Térkép letöltése","Download map"),ink,2);
  if(mapPrompt){
    centeredLabel(24,92,272,19,lastError?tr("Letöltési hiba","Download failed"):tr("Offline világtérkép a GitHubról","Offline world map from GitHub"),muted,1);
    const char *target=internalEnabled && (!sdOK || mapStorage!=MapStorage::sd)?
      tr("Belső tárhely · Wi-Fi","Internal storage · Wi-Fi"):tr("microSD · Wi-Fi","microSD · Wi-Fi");
    centeredLabel(24,112,272,19,lastError?tr("Automatikus újrapróbálás","Retrying automatically"):target,muted,1);
    framedBox(27,146,130,31,5,land,later?grid:mint);
    framedBox(163,146,130,31,5,land,later?mint:grid);
    centeredLabel(27,146,130,31,tr("Letöltés","Download"),ink);
    centeredLabel(163,146,130,31,tr("Később","Later"),ink);
  }else{
    int pc=offer.size?int(uint64_t(written)*100/offer.size):0;
    centeredLabel(28,100,264,22,String(pc)+"%   "+(flashTarget?"Flash":"SD"),mint,2);
    screen.drawRoundRect(40,139,240,11,4,grid);screen.fillRect(43,142,234*pc/100,5,mint);
  }
}
}
