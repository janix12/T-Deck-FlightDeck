#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include "board.h"

// Use the ESP32 FatFs driver with its own mount and sector transfers.
const char *nativeMap(const char *path,const uint8_t *seed,size_t size,uint8_t *data,
                      size_t &verified,bool &created,int &diff,int &actual){
  if(!SD.begin(BOARD_SDCARD_CS,SPI,4000000,"/fdcard",2,false)) return "SD csatolas";
  auto check=[&](size_t limit)->const char* {
    verified=0;
    fs::File f=SD.open(path,FILE_READ);
    if(!f) return "SD megnyitas";
    if(f.size()!=limit) return "SD fajlmeret";
    uint8_t block[512];
    while(verified<limit){
      size_t n=std::min(sizeof(block),limit-verified);
      if(f.read(block,n)!=n) return "SD olvasas";
      if(memcmp(block,seed+verified,n)){
        size_t i=0;while(block[i]==seed[verified+i]) ++i;
        diff=verified+i;actual=block[i];
        return "SD adatelteres";
      }
      memcpy(data+verified,block,n);verified+=n;
      if(!(verified%8192)) delay(1);
    }
    return nullptr;
  };
  const char *error=check(size);
  if(error){
    auto write=[&](size_t begin,size_t end)->const char* {
      fs::File f=SD.open(path,begin?FILE_APPEND:FILE_WRITE);
      if(!f) return "SD letrehozas";
      uint8_t block[512];
      for(size_t at=begin;at<end;){
        size_t n=std::min(sizeof(block),end-at);memcpy(block,seed+at,n);
        if(f.write(block,n)!=n) return "SD iras";
        at+=n;if(!(at%8192)) delay(1);
      }
      f.flush();f.close();return nullptr;
    };
    size_t first=std::min(size,size_t(512));
    error=write(0,first);
    if(!error) error=check(first);
    if(!error) error=write(first,size);
    if(!error){created=true;error=check(size);}
  }
  SD.end();return error;
}
