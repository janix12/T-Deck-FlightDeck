#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

constexpr unsigned trafficMinPeriod=120, trafficDailyLimit=500;
constexpr uint32_t trafficClockFloor=1700000000;
struct TrafficBudget {
  uint32_t last=0, used=0, retryAt=0, limitedReplies=0;
  uint32_t usedToday(uint32_t now) const {return now/86400==last/86400?used:0;}
  uint32_t nextAllowed(unsigned period) const {
    uint32_t next=last+period;
    if(used>=trafficDailyLimit){
      uint32_t midnight=(last/86400+1)*86400;
      if(midnight>next)next=midnight;
    }
    return retryAt>next?retryAt:next;
  }
  bool reserve(uint32_t now,unsigned period){
    if(now<trafficClockFloor || now<nextAllowed(period))return false;
    used=usedToday(now)+1;
    last=now;
    return true;
  }
  void limited(uint32_t now,uint32_t retry){
    // Missing Retry-After: back off from two minutes to one hour.
    unsigned delay=120u<<(limitedReplies<5?limitedReplies:5);
    if(delay>3600)delay=3600;
    if(limitedReplies<6)++limitedReplies;
    retryAt=retry>now?retry:now+delay;
  }
};

// Retry-After is either a delay in seconds or an IMF-fixdate in GMT.
static uint32_t trafficRetryTime(const char *value,uint32_t now){
  if(*value>='0' && *value<='9'){
    char *end=nullptr;
    unsigned long long delay=strtoull(value,&end,10);
    if(*end)return 0;
    return delay>UINT32_MAX-now?UINT32_MAX:now+(uint32_t)delay;
  }
  char month[4]={};
  int d,y,h,m,s,n=0;
  if(sscanf(value,"%*3[A-Za-z], %2d %3[A-Za-z] %4d %2d:%2d:%2d GMT%n",&d,month,&y,&h,&m,&s,&n)!=6 ||
     !n || value[n] || y<1970 || y>2105 || d<1 || h<0 || h>23 || m<0 || m>59 || s<0 || s>59)return 0;
  const char *months="JanFebMarAprMayJunJulAugSepOctNovDec";
  const char *found=strstr(months,month);
  if(!found || strlen(month)!=3 || (found-months)%3)return 0;
  int mon=(found-months)/3+1;
  const int lengths[]={31,28,31,30,31,30,31,31,30,31,30,31};
  bool leap=y%4==0 && (y%100!=0 || y%400==0);
  if(d>lengths[mon-1]+(mon==2 && leap))return 0;
  y-=mon<=2;
  int era=y/400, yy=y%400;
  int days=era*146097+yy*365+yy/4-yy/100+(153*(mon+(mon>2?-3:9))+2)/5+d-1-719468;
  return uint32_t(days)*86400u+h*3600u+m*60u+s;
}
