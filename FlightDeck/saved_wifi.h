#pragma once
#include <stdint.h>
#include <string.h>

struct WifiProfile {char ssid[33],password[65];};
struct WifiBook {
  static constexpr int capacity=8;
  uint8_t version=1,count=0,preferred=0;
  WifiProfile entries[capacity]{};
  int find(const char *ssid) const{
    for(int i=0;i<count;i++)if(!strcmp(entries[i].ssid,ssid))return i;
    return -1;
  }
  bool valid() const{
    if(version!=1 || count>capacity || (count?preferred>=count:preferred!=0))return false;
    for(int i=0;i<count;i++){
      const auto &p=entries[i];
      if(!p.ssid[0] || p.ssid[32] || p.password[64] || find(p.ssid)!=i)return false;
    }
    return true;
  }
  bool put(const char *ssid,const char *password){
    if(!*ssid || strlen(ssid)>32 || strlen(password)>64)return false;
    int i=find(ssid);
    if(i<0){if(count>=capacity)return false;i=count++;}
    entries[i]={};
    strcpy(entries[i].ssid,ssid);strcpy(entries[i].password,password);
    preferred=i;return true;
  }
  void erase(int i){
    if(i<0 || i>=count)return;
    for(int j=i+1;j<count;j++)entries[j-1]=entries[j];
    entries[--count]={};
    if(preferred==i)preferred=0;else if(preferred>i)--preferred;
  }
};
