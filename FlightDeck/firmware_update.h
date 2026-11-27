#pragma once
#include <esp_http_client.h>
#include <esp_ota_ops.h>
#include <mbedtls/sha256.h>
#include "weather_roots.h"

// Let setup finish and the first screen render before accepting a new image.
extern "C" bool verifyRollbackLater(){return true;}
enum class UpdateState { idle,checking,current,available,downloading,rebooting,error };
struct FirmwareOffer {char version[20],sha[65];uint32_t size;};
struct UpdateJob {bool install;FirmwareOffer offer;};
struct UpdateReply {UpdateState state;FirmwareOffer offer;int error;};
static UpdateState otaState=UpdateState::idle;
static FirmwareOffer offer{};
static volatile unsigned otaPercent;
static QueueHandle_t otaReplies;
static uint32_t lastOtaTry,otaDelay;
static int otaError;
static bool otaTried;
static constexpr char updateBase[]="https://janix12.github.io/T-Deck-FlightDeck/install/";

static bool versionParts(const char *s,unsigned (&v)[3]){
  if(!s || strlen(s)>18)return false;
  for(int i=0;i<3;i++){
    if(*s<'0' || *s>'9')return false;
    unsigned n=0;
    do{n=n*10+(*s++-'0');if(n>65535)return false;}while(*s>='0' && *s<='9');
    v[i]=n;
    if(i<2 && *s++!='.')return false;
  }
  return !*s;
}
static bool readOffer(const char *json,FirmwareOffer &out,bool &newer){
  JsonDocument doc;
  if(deserializeJson(doc,json) || strcmp(doc["device"]|"","lilygo-t-deck-esp32s3") ||
     !doc["size"].is<uint32_t>())return false;
  const char *version=doc["version"]|"",*sha=doc["sha256"]|"";
  unsigned remote[3],localVersion[3];
  if(!versionParts(version,remote) || !versionParts(FW_VERSION,localVersion) ||
     strlen(sha)!=64 || strspn(sha,"0123456789abcdef")!=64)return false;
  out.size=doc["size"];
  if(out.size<4096 || out.size>0x300000)return false;
  strcpy(out.version,version);strcpy(out.sha,sha);
  newer=std::lexicographical_compare(localVersion,localVersion+3,remote,remote+3);
  return true;
}
static int fetchUpdate(UpdateJob &job,UpdateReply &reply){
  String url=String(updateBase)+(job.install?String("FlightDeck.ino.bin?sha256=")+job.offer.sha:"manifest.json");
  esp_http_client_config_t cfgHttp{};
  cfgHttp.url=url.c_str();cfgHttp.cert_pem=weatherRoots;cfgHttp.timeout_ms=12000;
  cfgHttp.disable_auto_redirect=true;cfgHttp.buffer_size=4096;
  auto http=esp_http_client_init(&cfgHttp);
  if(!http)return -1;
  esp_ota_handle_t handle=0;
  bool writing=false;
  int err=-2;
  mbedtls_sha256_context hash;mbedtls_sha256_init(&hash);
  do{
    esp_http_client_set_header(http,"Accept-Encoding","identity");
    if(esp_http_client_open(http,0)!=ESP_OK)break;
    int64_t size=esp_http_client_fetch_headers(http);
    int status=esp_http_client_get_status_code(http);
    if(status!=200){err=status>0?status:-3;break;}
    if(!job.install){
      char json[768];
      if(size<1 || size>=int(sizeof(json))){err=-4;break;}
      int n=esp_http_client_read_response(http,json,int(size));
      if(n!=size){err=-5;break;}json[n]=0;
      bool newer=false;
      if(!readOffer(json,reply.offer,newer)){err=-6;break;}
      reply.state=newer?UpdateState::available:UpdateState::current;
      err=0;break;
    }
    auto part=esp_ota_get_next_update_partition(nullptr);
    if(size!=job.offer.size || !part || part==esp_ota_get_running_partition() || size>part->size){err=-7;break;}
    if(esp_ota_begin(part,size,&handle)!=ESP_OK){err=-8;break;}
    writing=true;mbedtls_sha256_starts_ret(&hash,0);
    char buf[3072];uint32_t total=0;
    while(total<size){
      int n=esp_http_client_read(http,buf,min(uint32_t(sizeof(buf)),job.offer.size-total));
      if(n<=0 || esp_ota_write(handle,buf,n)!=ESP_OK)break;
      mbedtls_sha256_update_ret(&hash,(const uint8_t*)buf,n);
      total+=n;otaPercent=total*100/job.offer.size;
    }
    if(total!=size){err=-9;break;}
    uint8_t digest[32];char hex[65];
    mbedtls_sha256_finish_ret(&hash,digest);
    for(int i=0;i<32;i++)snprintf(hex+2*i,3,"%02x",digest[i]);
    if(strcmp(hex,job.offer.sha)){err=-10;break;}
    writing=false;
    if(esp_ota_end(handle)!=ESP_OK || esp_ota_set_boot_partition(part)!=ESP_OK){err=-11;break;}
    reply.state=UpdateState::rebooting;err=0;
  }while(false);
  if(writing)esp_ota_abort(handle);
  mbedtls_sha256_free(&hash);esp_http_client_cleanup(http);
  return err;
}
static void updateWorker(void *arg){
  UpdateJob job=*static_cast<UpdateJob*>(arg);delete static_cast<UpdateJob*>(arg);
  UpdateReply reply{};reply.state=UpdateState::error;
  xSemaphoreTake(networkGate,portMAX_DELAY);
  reply.error=fetchUpdate(job,reply);
  xSemaphoreGive(networkGate);
  xQueueSend(otaReplies,&reply,portMAX_DELAY);vTaskDelete(nullptr);
}
static void beginUpdate(bool install){
  if(otaState==UpdateState::checking || otaState==UpdateState::downloading || otaState==UpdateState::rebooting)return;
  if(WiFi.status()!=WL_CONNECTED || time(nullptr)<1700000000)return;
  otaTried=true;lastOtaTry=millis();otaDelay=30UL*60000;
  if(!otaReplies)otaReplies=xQueueCreate(1,sizeof(UpdateReply));
  auto job=new(std::nothrow) UpdateJob{install,offer};
  if(!otaReplies || !job || xTaskCreatePinnedToCore(updateWorker,"fw-update",14336,job,1,nullptr,0)!=pdPASS){
    delete job;otaState=UpdateState::error;otaError=-12;return;
  }
  otaPercent=0;otaError=0;otaState=install?UpdateState::downloading:UpdateState::checking;
}
static void otaTick(){
  UpdateReply reply;
  if(otaReplies && xQueueReceive(otaReplies,&reply,0)==pdTRUE){
    otaState=reply.state;otaError=reply.error;offer=reply.offer;
    lastOtaTry=millis();otaDelay=otaError?30UL*60000:86400000UL;dirty=true;
    Serial.printf("FW check state=%d error=%d version=%s\n",int(otaState),otaError,offer.version);
  }
  if(otaState==UpdateState::rebooting){
    if(millis()-lastOtaTry>1200)ESP.restart();
    return;
  }
  if(!otaTried || millis()-lastOtaTry>=otaDelay)beginUpdate(false);
}
static void updateAction(){beginUpdate(otaState==UpdateState::available);}
static void drawUpdateScreen(){
  label(10,39,tr("Firmware-frissítés","Firmware update"),mint,4);
  label(10,86,String(tr("Telepítve: v","Installed: v"))+FW_VERSION,ink);
  String msg;
  switch(otaState){
    case UpdateState::checking:msg=tr("Verzió ellenőrzése...","Checking version...");break;
    case UpdateState::available:msg=String(tr("Elérhető: v","Available: v"))+offer.version;break;
    case UpdateState::current:msg=tr("A firmware naprakész.","Firmware is up to date.");break;
    case UpdateState::downloading:msg=String(tr("Telepítés: ","Installing: "))+String(otaPercent)+"%";break;
    case UpdateState::rebooting:msg=tr("Ellenőrizve. Újraindítás...","Verified. Restarting...");break;
    case UpdateState::error:msg=String(tr("Frissítési hiba ","Update error "))+String(otaError);break;
    default:break;
  }
  if(WiFi.status()!=WL_CONNECTED)msg=tr("Wi-Fi kapcsolatra vár","Waiting for Wi-Fi");
  else if(time(nullptr)<1700000000)msg=tr("Pontos időre vár","Waiting for clock sync");
  label(10,117,msg,otaState==UpdateState::error?amber:ink);
  label(10,159,tr("Ellenőrzés induláskor és naponta.","Checks at startup and daily."),muted);
  saveButton.draw(otaState==UpdateState::available?tr("Telepítés","Install"):tr("Ellenőrzés","Check"),mint,mint,bg);
  cancelButton.draw(tr("Vissza","Back"),bg,muted,ink);
}
