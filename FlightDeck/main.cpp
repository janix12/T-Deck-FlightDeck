#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <sys/time.h>
#include "hardware.h"
#include "traffic.h"
#include "offline_map.h"
#include "flight_state.h"

enum Field {SSID, PASS, ZONE, LAT, LON, RANGE, PERIOD, LIGHT, BAUD, GPS_PIN, LOGGING, CLOCK, LANG, THEME, SAVER, FIELDS};
static const char *labels[]={"Wi-Fi név","Wi-Fi jelszó","Időzóna","Kézi szélesség","Kézi hosszúság","Sugár (NM)","Frissítés (mp)","Fényerő 1-16","GPS baud","GPS RX GPIO","SD napló 0/1","UTC óra","Nyelv","Téma","Pihenő (perc)"};
static const char *defaults[]={"","","Budapest","47.4979","19.0402","40","120","10","0","44","0","","HU","0","30"};
static String cfg[FIELDS],edit,notice;
static Preferences prefs;
static int page,field,selected;
static bool editing,dirty=true,busy;
static bool choosingNetwork;
static int networkIndex;
static uint32_t nextFetch,lastGpsChange,lastSentence,lastLog;
static const int rates[]={9600,38400,115200,4800,19200,57600};
static int rateIndex,activeRate;
static Traffic traffic,incoming;
static int apiCode;
#include "appearance.h"
static uint32_t lastNetAttempt,lastSignalCheck,weakSince,lastRoamScan;
static volatile uint8_t disconnectReason;
static bool probePending;
static int seenNetworks=-1,matchingChannel,matchingAuth=-1;
static int startScan(){
  if(WiFi.status()!=WL_CONNECTED) WiFi.disconnect(false,false);
  return WiFi.scanNetworks(true,true);
}

static bool due(uint32_t now,uint32_t at){return (int32_t)(now-at)>=0;}
static bool hasFix(){return gps.location.isValid() && gps.location.age()<15000;}
static Area currentArea(){
  return {hasFix()?gps.location.lat():cfg[LAT].toDouble(),hasFix()?gps.location.lng():cfg[LON].toDouble(),cfg[RANGE].toInt()};
}
static void network(){
  if(probePending){WiFi.scanDelete();probePending=false;}
  weakSince=0;
  if(cfg[SSID].length()) WiFi.begin(cfg[SSID].c_str(),cfg[PASS].c_str());
  else WiFi.disconnect(false,false);
  seenNetworks=-1; matchingChannel=0; matchingAuth=-1;
  lastNetAttempt=millis();
}
static void networkProbe(){
  if(choosingNetwork || cfg[SSID].isEmpty()) return;
  const bool connected=WiFi.status()==WL_CONNECTED;
  const uint32_t now=millis();
  if(!probePending){
    if(connected){
      if(now-lastSignalCheck<2000)return;
      lastSignalCheck=now;
      if(WiFi.RSSI()>-75){weakSince=0;return;}
      if(!weakSince)weakSince=now;
      if(now-weakSince<15000 || (lastRoamScan && now-lastRoamScan<90000) || busy)return;
      lastRoamScan=now;
    }else{
      weakSince=0;
      if(seenNetworks!=-1 || now-lastNetAttempt<12000)return;
    }
    probePending=startScan()==WIFI_SCAN_RUNNING;
    if(!probePending) seenNetworks=-2;
    return;
  }
  int count=WiFi.scanComplete();
  if(count==WIFI_SCAN_RUNNING) return;
  seenNetworks=count; probePending=false;
  wifi_ap_record_t current{};
  if(connected && (esp_wifi_sta_get_ap_info(&current)!=ESP_OK || current.rssi>-75)){
    WiFi.scanDelete();return;
  }
  int best=-1;
  for(int i=0;i<count;i++) if(WiFi.SSID(i)==cfg[SSID]){
    auto auth=WiFi.encryptionType(i);
    if((auth==WIFI_AUTH_OPEN)!=cfg[PASS].isEmpty())continue;
    if(connected && (auth!=current.authmode || !memcmp(WiFi.BSSID(i),current.bssid,6) ||
                     WiFi.RSSI(i)<current.rssi+10))continue;
    if(best<0 || WiFi.RSSI(i)>WiFi.RSSI(best)) best=i;
  }
  if(best>=0){
    uint8_t bssid[6]; memcpy(bssid,WiFi.BSSID(best),6);
    matchingChannel=WiFi.channel(best); matchingAuth=WiFi.encryptionType(best);
    if(connected)Serial.printf("WiFi roam RSSI %d -> %d channel %d\n",current.rssi,WiFi.RSSI(best),matchingChannel);
    WiFi.begin(cfg[SSID].c_str(),cfg[PASS].isEmpty()?nullptr:cfg[PASS].c_str(),matchingChannel,bssid);
    lastNetAttempt=millis();
  }
  WiFi.scanDelete();
}
static void zone(){
  String z=cfg[ZONE];
  if(z=="Budapest") z="CET-1CEST,M3.5.0,M10.5.0/3";
  if(z=="UTC") z="UTC0";
  configTzTime(z.c_str(),"pool.ntp.org","time.cloudflare.com");
}
static void startGps(){
  activeRate=cfg[BAUD].toInt();
  if(!activeRate) activeRate=rates[rateIndex];
  Serial1.end();
  Serial1.begin(activeRate,SERIAL_8N1,cfg[GPS_PIN].toInt(),-1);
  lastGpsChange=millis(); lastSentence=0;
}
static time_t utcEpoch(int year,int month,int day,int hour,int minute,int second){
  // Gregorian civil date to UTC seconds, independent of the display time zone.
  year-=month<=2;
  int era=year/400, yy=year-era*400;
  int days=(153*(month+(month>2?-3:9))+2)/5+day-1;
  return time_t(era*146097+yy*365+yy/4-yy/100+days-719468)*86400+hour*3600+minute*60+second;
}
static void readGps(){
  while(Serial1.available()){
    if(gps.encode(Serial1.read())){
      lastSentence=millis();
      if(time(nullptr)<1700000000 && gps.date.isValid() && gps.time.isValid() &&
         gps.time.age()<2000 && gps.date.age()<2000 && gps.date.year()>=2024){
        time_t utc=utcEpoch(gps.date.year(),gps.date.month(),gps.date.day(),gps.time.hour(),gps.time.minute(),gps.time.second());
        timeval tv={utc,0}; settimeofday(&tv,nullptr);
      }
    }
  }
  if(!cfg[BAUD].toInt() && millis()-lastGpsChange>4500 && (!lastSentence || millis()-lastSentence>10000)){
    rateIndex=(rateIndex+1)%(sizeof(rates)/sizeof(rates[0])); startGps();
  }
}
static bool numeric(const String &v,double lo,double hi,bool integer=false){
  char *end; double n=strtod(v.c_str(),&end);
  return end!=v.c_str() && !*end && isfinite(n) && n>=lo && n<=hi && (!integer || floor(n)==n);
}
static bool applyField(){
  if(field!=PASS && field!=SSID) edit.trim();
  switch(field){
    case SSID: if(edit.length()>32) return false; break;
    case PASS: if(edit.length() && (edit.length()<8 || edit.length()>63)) return false; break;
    case LAT: if(!numeric(edit,-90,90)) return false; break;
    case LON: if(!numeric(edit,-180,180)) return false; break;
    case RANGE: if(!numeric(edit,1,250,true)) return false; break;
    case PERIOD: if(!numeric(edit,trafficMinPeriod,300,true)) return false; break;
    case LIGHT: if(!numeric(edit,1,16,true)) return false; break;
    case BAUD: if(edit!="0" && edit!="4800" && edit!="9600" && edit!="19200" && edit!="38400" && edit!="57600" && edit!="115200") return false; break;
    case GPS_PIN: if(edit!="43" && edit!="44") return false; break;
    case LOGGING: if(edit!="0" && edit!="1") return false; break;
    case LANG: if(edit!="HU" && edit!="EN") return false; break;
    case THEME: if(!numeric(edit,0,2,true)) return false; break;
    case SAVER: if(!numeric(edit,0,1440,true)) return false; break;
    case ZONE: if(edit.isEmpty() || edit.length()>64) return false; break;
    case CLOCK:{
      int y,m,d,h,minu;
      if(edit.length()!=16 || sscanf(edit.c_str(),"%4d-%2d-%2d %2d:%2d",&y,&m,&d,&h,&minu)!=5 ||
         y<2024 || y>2037 || m<1 || m>12 || d<1 || d>31 || h<0 || h>23 || minu<0 || minu>59) return false;
      time_t utc=utcEpoch(y,m,d,h,minu,0);
      tm checked; gmtime_r(&utc,&checked); char expected[18];
      strftime(expected,sizeof(expected),"%Y-%m-%d %H:%M",&checked);
      if(edit!=expected) return false;
      timeval tv={utc,0}; return settimeofday(&tv,nullptr)==0;
    }
  }
  String k="v"+String(field);
  if(!prefs.putString(k.c_str(),edit) && edit.length()) return false;
  cfg[field]=edit;
  if(field==THEME) setPalette();
  if(field==SSID || field==PASS) network();
  if(field==ZONE) zone();
  if(field==LIGHT) backlight(edit.toInt());
  if(field==BAUD || field==GPS_PIN) { rateIndex=0; startGps(); }
  if(field==RANGE || field==LAT || field==LON){ free(traffic.planes);traffic={}; selected=0; nextFetch=millis(); }
  return true;
}
static String valueFor(int f){
  if(f==THEME) return themeName();
  if(f==LANG) return english()?"English":"Magyar";
  if(f==PASS) return cfg[f].length()?"********":tr("(üres)","(empty)");
  if(f==CLOCK) {
    time_t now=time(nullptr); if(now<1700000000) return "--";
    struct tm utc; gmtime_r(&now,&utc); char out[20]; strftime(out,sizeof(out),"%Y-%m-%d %H:%M",&utc); return out;
  }
  if(f==BAUD && cfg[f]=="0") return "Auto";
  return cfg[f].length()?cfg[f]:tr("(üres)","(empty)");
}
static String number(float n,int digits=0){return isfinite(n)?String(n,digits):"--";}
#include "display_text.h"
static String clockText(){
  time_t t=time(nullptr); if(t<1700000000) return "--:--";
  tm local; localtime_r(&t,&local); char out[8]; strftime(out,sizeof(out),"%H:%M",&local); return out;
}
static String feedStatus(){
  const uint32_t now=time(nullptr);
  if(WiFi.status()==WL_CONNECTED && now>=trafficClockFloor){
    bool localCap=budget.usedToday(now)>=trafficDailyLimit;
    if(budget.retryAt>now || localCap){
      const time_t next=budget.nextAllowed(cfg[PERIOD].toInt());
      struct tm local;localtime_r(&next,&local);char out[8];strftime(out,sizeof(out),"%H:%M",&local);
      String msg=String(localCap?tr("Napi keret","Daily limit"):tr("API-korlát","API limit"))+" / "+out;
      if(traffic.stamp)msg+=" / "+String(max(0L,long(time(nullptr)-traffic.stamp))/60)+tr(" p"," min");
      return msg;
    }
    if(!budgetReady || apiCode==-104)return tr("Keretmentési hiba","Quota storage error");
    if(!traffic.stamp && !busy && now<budget.nextAllowed(cfg[PERIOD].toInt()))
      return String(tr("Frissítés: ","Refresh in "))+String(budget.nextAllowed(cfg[PERIOD].toInt())-now)+tr(" mp"," s");
  }
  if(traffic.stamp){
    long age=max(0L,long(time(nullptr)-traffic.stamp));
    String old=age>=120?String(age/60)+tr(" perc"," min"):String(age)+tr(" mp"," s");
    if(WiFi.status()!=WL_CONNECTED) return tr("Offline  /  utolso adat: ","Offline  /  last data: ")+old;
    if(apiCode!=200) return tr("Frissitesi hiba ","Update error ")+String(apiCode)+tr("  /  utolso adat: ","  /  last data: ")+old;
    return (age>cfg[PERIOD].toInt()+30?tr("Régi adat  /  ","Stale data  /  "):"adsb.lol  /  ")+old;
  }
  if(!cfg[SSID].length()) return tr("Wi-Fi beallitas szukseges","Set up Wi-Fi");
  if(WiFi.status()==WL_NO_SSID_AVAIL) return tr("Wi-Fi halozat nem lathato","Wi-Fi network not found");
  if(WiFi.status()==WL_CONNECT_FAILED) return tr("Wi-Fi kapcsolodas sikertelen","Wi-Fi connection failed");
  if(WiFi.status()!=WL_CONNECTED) return tr("Wi-Fi kapcsolodas...","Connecting to Wi-Fi...");
  if(time(nullptr)<1700000000) return tr("Varakozas az orara","Waiting for clock");
  if(apiCode && apiCode!=200) return tr("API hiba ","API error ")+String(apiCode);
  return tr("Forgalom lekérése...","Loading flights...");
}
#include "radar_map.h"
#include "weather.h"
static void diagnostics(){
  label(9,33,"I2C  "+busList,muted);
  label(9,55,sensor+(ahtOK?"+AHT2x":shtOK?"+SHT3x":"")+"   "+number(temperature,1)+" C   "+number(pressure,1)+" hPa");
  label(9,79,"GPS RX"+cfg[GPS_PIN]+"  "+String(activeRate)+"  "+(hasFix()?"FIX":(lastSentence?tr("NMEA / fixre vár","NMEA / waiting for fix"):tr("jelet keres","searching"))));
  label(9,102,"SD "+(sdOK?String(volumeMiB)+" MiB":(cardMiB?String(tr("FS hiba","FS error")):String("--")))+tr("    Akku ","    Battery ")+number(voltage,2)+" V");
  label(9,125,String("GT911 ")+(touchOK?"OK":"--")+tr("   Bill. ","   Keys ")+(kbOK?"OK":"--")+"   RH "+number(humidity)+"%");
  label(9,148,String("LoRa RX ")+(radioOK?String(radioPackets)+tr(" csomag"," packets"):"--")+"   Mic "+(micOK?String(micPeak):"--"));
  label(9,174,tr("Hangpróba [B]    SD újra [R]","Speaker [B]      Retry SD [R]"),speakerOK?mint:muted);
  label(9,197,WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():feedStatus(),muted);
  label(9,219,String("ADS-B ")+String(budget.usedToday(time(nullptr)))+"/500 UTC  "+
      (WiFi.status()==WL_CONNECTED?String(WiFi.RSSI())+" dBm":String("Wi-Fi --")),muted,1);
}
struct EditorButton {
  int left;
  static constexpr int top=201,width=148,height=33;
  bool contains(int x,int y) const{
    return x>=left && x<left+width && y>=top && y<top+height;
  }
  void draw(const char *text,uint16_t fill,uint16_t border,uint16_t foreground) const{
    framedBox(left,top,width,height,6,fill,border);
    centeredLabel(left,top,width,height,text,foreground);
  }
};
static constexpr EditorButton saveButton{8},cancelButton{164};
static void settings(){
  if(choosingNetwork){
    label(10,33,tr("Wi-Fi hálózatok","Wi-Fi networks"),mint);
    int count=WiFi.scanComplete();
    if(count<=0) label(10,70,count==WIFI_SCAN_RUNNING?tr("Keresés...","Scanning..."):tr("Nincs találat","No networks"),muted);
    else{
      int top=(networkIndex/5)*5;
      for(int i=top;i<min(top+5,count);i++){
        int y=59+(i-top)*25;
        if(i==networkIndex) screen.fillRoundRect(5,y-2,310,23,4,grid);
        String name=WiFi.SSID(i);
        label(10,y,name.length()?name.substring(0,labelPrefixBytes(name,2,240)):tr("(rejtett)","(hidden)"));
        label(260,y,String(WiFi.RSSI(i)),muted);
      }
    }
    label(10,196,tr("Érintés: választ     Vissza","Tap to select     Back"),muted);return;
  }
  if(editing){
    label(10,45,fieldLabel(field),mint,4);
    String shown=edit;
    if(field==PASS){shown=""; for(size_t i=0;i<edit.length();i++) shown+="*";}
    label(10,93,shown.substring(shown.length()>32?shown.length()-32:0)+"_");
    const char *hint="";
    switch(field){
      case SAVER: hint=tr("0 = ki, 1 - 1440 perc","0 = off, 1 - 1440 minutes");break;
      case ZONE: hint=tr("Budapest, UTC vagy POSIX TZ","Budapest, UTC or POSIX TZ");break;
      case BAUD: hint=tr("0 = automatikus felismerés","0 = automatic");break;
      case CLOCK: hint="UTC: YYYY-MM-DD HH:MM";break;
      case RANGE: hint=tr("1 - 250 tengeri mérföld","1 - 250 nautical miles");break;
      case PERIOD: hint=tr("120 - 300 mp / 16 óra: 120 mp","120 - 300 s / 16 hours: 120 s");break;
      case GPS_PIN: hint=tr("43 vagy 44","43 or 44");break;
      case PASS: hint=tr("Üres = nyílt Wi-Fi","Empty = open Wi-Fi");break;
    }
    label(10,131,field==SSID?tr("[ Hálózatok keresése ]","[ Scan networks ]"):hint,field==SSID?mint:muted); label(10,159,notice,amber);
    saveButton.draw(tr("Mentés","Save"),mint,mint,bg);
    cancelButton.draw(tr("Mégse","Cancel"),bg,muted,ink);
    return;
  }
  int top=field/6*6;
  for(int i=top;i<min(top+6,int(FIELDS));i++){
    int y=35+(i-top)*27;
    if(i==field) screen.fillRoundRect(5,y-2,310,25,4,grid);
    label(11,y,fieldLabel(i),i==field?mint:ink);
    String value=valueFor(i);label(160,y,value.substring(0,labelPrefixBytes(value,2,150)));
  }
  label(10,198,tr("Görgő: le/fel    Nyomás: szerkesztés","Scroll to browse    Press to edit"),muted);
  pill(7,218,46,20);centeredLabel(7,218,46,20,"<",mint);
  pill(267,218,46,20);centeredLabel(267,218,46,20,">",mint);
  centeredLabel(53,218,214,20,String(top/6+1)+" / "+String((FIELDS+5)/6),muted,1);
}
static void drawBatteryIcon(int x,int y,int charge,bool usb){
  constexpr int width=35;
  const uint16_t outline=usb?mint:charge<0?muted:charge<=20?amber:mint;
  framedBox(x,y,width,16,4,bg,outline);
  screen.fillRect(x+width,y+5,2,6,outline);
  if(usb){
    screen.fillTriangle(x+9,y+2,x+4,y+9,x+8,y+9,outline);
    screen.fillTriangle(x+7,y+7,x+11,y+7,x+6,y+13,outline);
    const UiGlyph &usbText=face9.glyphs[0];
    drawGlyph(x+12+(width-14-usbText.width)/2,y+(17-usbText.height)/2,face9,usbText,ink);
  }else{
    const String value=charge<0?String("--%"):String(charge)+"%";
    centeredLabel(x,y,width,16,value,ink,1);
  }
}
#include "self_test.h"
static const char *pageTitle(int p){
  switch(p){
    case 1:return tr("Műszerek","Instruments");
    case 2:return tr("Beállítások","Settings");
    case 3:return tr("Esőradar","Rain radar");
    case 4:return tr("Hőtérkép","Heat map");
    default:return tr("Légtér","Airspace");
  }
}
static const int menuPages[]={0,3,4,1,2,-1};
static const char *menuTitle(int i){return i==5?tr("Képernyőpihenő","Sleep now"):pageTitle(menuPages[i]);}
static void goPage(int p){
  if(choosingNetwork){WiFi.scanDelete();choosingNetwork=false;}
  editing=menuOpen=false;
  if(p==-1)startSaver();else page=p;
  if(page==0 && p==0){cardOpen=false;cardList=false;}
  dirty=true;
}
static void toggleMenu(){
  menuOpen=!menuOpen;
  for(int i=0;i<5;i++)if(menuPages[i]==page)menuIndex=i;
  dirty=true;
}
static void render(){
  if(bench.active){benchScreen();return;}
  if(sleeping){
    screen.fillSprite(TFT_BLACK);int drift=(millis()/60000)%5;
    label(62+drift*7,78+drift*5,clockText(),muted,6);
    screen.pushSprite(0,0);return;
  }
  screen.fillSprite(bg);
  if(page==0)radar();
  else if(weatherPage()){mapWidth=320;midX=160;weatherMap();}
  else if(page==1)diagnostics();else settings();
  pill(4,3,30,26);for(int i=0;i<3;i++)screen.fillRect(12,11+i*4,14,2,ink);
  String title=pageTitle(page);
  if(!page)title+="   "+String(traffic.count);
  if(page==3)title+="   "+number(viewRadius(weatherZoomStep))+" NM";
  int titleW=labelWidth(title,2)+23;pill(38,3,titleW,26);
  screen.fillCircle(47,16,3,WiFi.status()==WL_CONNECTED?mint:amber);
  centeredLabel(55,3,titleW-23,26,title,ink);
  pill(221,3,95,26);
  drawBatteryIcon(224,8,batteryPercent(batteryEstimateV),externalPower);
  centeredLabel(268,3,38,26,clockText(),ink);screen.fillCircle(310,16,2,hasFix()?mint:muted);
  if(page==0 && cardOpen && !menuOpen){
    pill(cardX,3,closeButtonWidth,26);
    for(int i=0;i<8;++i){
      screen.fillRect(cardX+(closeButtonWidth-9)/2+i,12+i,2,1,ink);
      screen.fillRect(cardX+(closeButtonWidth-9)/2+7-i,12+i,2,1,ink);
    }
  }
  if(menuOpen){
    framedBox(4,34,166,201,8,bg,grid);
    for(int i=0;i<6;i++){
      int y=39+i*32;if(i==menuIndex)screen.fillRoundRect(8,y,158,29,5,grid);
      screen.fillCircle(17,y+14,3,i==menuIndex?mint:muted);label(28,y+6,menuTitle(i),i==menuIndex?mint:ink);
    }
  }
  screen.pushSprite(0,0);
}
static void moveBy(int step){
  if(menuOpen){menuIndex=(menuIndex+step+6)%6;dirty=true;return;}
  if(choosingNetwork){
    int count=WiFi.scanComplete(); if(count>0) networkIndex=(networkIndex+step+count)%count;
    dirty=true;return;
  }
  if(page==2 && !editing) field=(field+step+FIELDS)%FIELDS;
  if(page==0 && traffic.count) pickPlane((selected+step+traffic.count)%traffic.count);
  dirty=true;
}
static void activate(){
  if(menuOpen){goPage(menuPages[menuIndex]);return;}
  if(choosingNetwork){
    if(networkIndex>=0 && networkIndex<WiFi.scanComplete()) edit=WiFi.SSID(networkIndex);
    choosingNetwork=false; WiFi.scanDelete(); dirty=true;return;
  }
  if(page==2){
    if(editing){ if(applyField()) editing=false; else notice=tr("Érvénytelen érték","Invalid value"); }
    else if(field==LANG || field==THEME){
      edit=field==LANG?(english()?"HU":"EN"):String((cfg[THEME].toInt()+1)%3);
      applyField();
    }else {editing=true; edit=""; notice="";} // Existing value stays in NVS until Enter.
  } else if(page==1) beep(); else if(page==0){if(cardOpen && cardList)pickPlane(listCursor);else if(!cardOpen)cardMode(true);else cardOpen=false;}
  else if(page==3){playing=!playing;rainIndex=playing?-1:(wxCache[0].frames?int(millis()/1400%wxCache[0].frames):0);}
  dirty=true;
}
static void key(int c){
  if(!c || !interact()) return;
  if(menuOpen){
    if(c=='m'||c=='M'||c==27)toggleMenu();else if(c=='j'||c=='k')moveBy(c=='j'?1:-1);
    else if(c==10||c==13)activate();
    return;
  }
  dirty=true;
  if(choosingNetwork){
    if(c==13 || c==10) activate();
    else if(c==27){choosingNetwork=false;WiFi.scanDelete();}
    else if(c=='j' || c=='k') moveBy(c=='j'?1:-1);
    return;
  }
  if(editing){
    if(c==13 || c==10) activate();
    else if(c==27) editing=false;
    else if(c==8 || c==127){ if(edit.length()) edit.remove(edit.length()-1); }
    else if(c>=32 && c<=126 && edit.length()<64) edit+=char(c);
    return;
  }
  if(c>='1' && c<='5') goPage(c-'1');
  else if(c=='m'||c=='M')toggleMenu();
  else if(page==0 && (c=='f'||c=='F'||c==27)){cardOpen=false;cardList=false;}
  else if(c=='s'||c=='S') startSaver();
  else if(page==0 && (c=='l' || c=='L')) cardMode(!cardList);
  else if(page==0 && (c=='d' || c=='u' || c=='D' || c=='U')) cardBall(tolower(c)=='d'?1:-1);
  else if((page==0||weatherPage()) && (c=='+' || c=='=' || c=='i' || c=='I')) zoomMap(1);
  else if((page==0||weatherPage()) && (c=='-' || c=='o' || c=='O')) zoomMap(-1);
  else if(c=='j' || c=='J' || c=='.') moveBy(1);
  else if(c=='k' || c=='K' || c==',') moveBy(-1);
  else if(c==13 || c==10) activate();
  else if(page==1 && (c=='b'||c=='B')) beep();
  else if(page==1 && (c=='r'||c=='R')) {mountCard();loadMap(true);}
}
static void input(){
  if(kbOK && Wire.requestFrom(0x55,1)==1) key(Wire.read());
  static uint32_t ignoreBallUntil;
  static bool old[5]; static bool init=false;
  for(int i=0;i<5;i++){
    bool v=digitalRead(ballPins[i]);
    if(init && old[i]!=v){
      if(!interact())ignoreBallUntil=millis()+300;
      else if(!due(millis(),ignoreBallUntil)){}
      else if(i==4 && !v) activate();
      else if(!menuOpen && page==0 && cardOpen && (i==1 || i==3)) cardBall(i==1?-1:1);
      else if((choosingNetwork || !editing) && i<4) moveBy(i==1 || i==2?-1:1);
    }
    old[i]=v;
  }
  init=true;
  static bool pressed=false;
  static bool dragCard=false,scrolled=false;
  static int firstY,lastY;
  int16_t x,y;
  bool down=touchOK && touch.isPressed() && touch.getPoint(&x,&y,1);
  if(down && pressed && dragCard){
    if(abs(y-firstY)>4) scrolled=true;
    if(scrolled){scrollCard(lastY-y);lastY=y;}
  }
  if(!down && pressed && dragCard){
    if(!scrolled && cardList) pickPlane((listScroll+firstY-cardBodyY)/listRow);
    dragCard=false;
  }
  if(down && !pressed){
    if(!interact()){pressed=true;return;}
    if(x<38 && y<32)toggleMenu();
    else if(menuOpen){if(x<170 && y>=37 && y<231)goPage(menuPages[min(5,(y-37)/32)]);else menuOpen=false;}
    else if(weatherPage()){
      if(y>=206 && x>=253)zoomMap(x>=285?1:-1);
      else if(page==3 && y>=206 && x<200)activate();
    }
    else if(choosingNetwork){
      if(y>=190){choosingNetwork=false; WiFi.scanDelete();}
      else if(y>=57 && y<182){
        int i=(networkIndex/5)*5+(y-57)/25;
        if(i<WiFi.scanComplete()){networkIndex=i;activate();}
      }
    }
    else if(page==2 && editing && field==SSID && y>=121 && y<160){
      probePending=false; WiFi.scanDelete(); startScan(); networkIndex=0;choosingNetwork=true;
    }
    else if(page==2 && !editing && y>=215 && (x<58 || x>262)){
      int pages=(FIELDS+5)/6;field=((field/6+(x<58?-1:1)+pages)%pages)*6;
    }
    else if(page==2 && !editing && y>=33 && y<195){ int hit=field/6*6+(y-33)/27;if(hit<FIELDS){field=hit;activate();} }
    else if(page==2 && editing){
      if(saveButton.contains(x,y)) activate();
      else if(cancelButton.contains(x,y)) editing=false;
    }
    else if(page==1 && y>=165 && y<196) {if(x<150) beep(); else {mountCard();loadMap(true);}}
    else if(page==0 && cardOpen && x>=cardX && x<cardX+closeButtonWidth && y<cardTop){
      cardOpen=false;cardList=false;
    }
    else if(page==0 && cardOpen && x>=cardX && y>=cardTop && y<cardBodyY){
      cardMode(x>=listTabX);
    }
    else if(page==0 && cardOpen && x>=cardX && y>=cardBodyY && y<cardBodyY+cardBodyH){
      dragCard=true;scrolled=false;firstY=lastY=y;
    }
    else if(page==0 && y>=205 && x>=mapWidth-67 && x<mapWidth) zoomMap(x>=mapWidth-35?1:-1);
    else if(page==0 && y>=204 && x>=69 && x<=103){
      if(cardOpen){cardOpen=false;cardList=false;}else cardMode(true);
    }
    else if(page==0 && traffic.count && y>=mapTop && y<mapTop+mapHeight) {
      if(x<mapWidth){
        float best=225; MapView v=mapView();
        for(int i=0;i<traffic.count;i++){const Plane &p=traffic.planes[i];
          float dist=sq(float(v.x(p.lon)-x))+sq(float(v.y(p.lat)-y));
          if(dist<best){best=dist;pickPlane(i);}
        }
      }
    }
    dirty=true;
  }
  pressed=down;
}
static void handleTraffic(){
  uint32_t now=millis();
  if(xQueueReceive(results,&incoming,0)==pdTRUE){
    busy=false; apiCode=incoming.code;budget=incoming.budget;
    if(apiCode==200){
      int nextSelection=0;
      for(int i=0;i<incoming.count;i++){
        Plane &p=incoming.planes[i];
        if(!p.id[0]) continue;
        for(int j=0;j<traffic.count;j++) if(!strcmp(p.id,traffic.planes[j].id)){
          updateMotion(p,traffic.planes[j]);
          if(j==selected) nextSelection=i;
          break;
        }
      }
      free(traffic.planes); traffic=incoming; incoming.planes=nullptr; selected=nextSelection;
      listCursor=constrain(listCursor,0,max(0,traffic.count-1));
    }
    nextFetch=now+(apiCode==-104?60000UL:1000UL);dirty=true;
  }
  if(nextFetch && due(now,nextFetch))nextFetch=0;
  if(!busy && !nextFetch && !sleeping && !bench.active && page==0 && budgetReady && WiFi.status()==WL_CONNECTED){
    uint32_t utc=time(nullptr);
    unsigned period=cfg[PERIOD].toInt();
    if(utc<trafficClockFloor || utc<budget.nextAllowed(period))return;
    TrafficRequest request{currentArea(),period};
    request.area.radius=max(request.area.radius,int(ceilf(viewRadius(zoomStep))));
    busy=xQueueSend(requests,&request,0)==pdTRUE;
  }
}
static void logEnvironment(){
  if(!sdOK || sdWriteFailed || cfg[LOGGING]!="1" || millis()-lastLog<60000) return;
  lastLog=millis();
  FsFile f=card.open("/flightdeck-env.csv",O_WRONLY|O_CREAT|O_APPEND);
  if(!f){Serial.println("SD naplo megnyitasi hiba");sdOK=false;return;}
  if(!f.fileSize()) f.println("epoch,uptime_s,temperature_C,pressure_hPa,humidity_pct,battery_V");
  f.printf("%ld,%lu,%.2f,%.2f,%.2f,%.3f\n",(long)time(nullptr),millis()/1000,temperature,pressure,humidity,voltage);
  bool ok=!f.getWriteError();
  if(!f.close() || !ok){sdOK=false;Serial.println("SD naplo irasi hiba");}
}
// Optional USB write test. Exclusive creation protects existing card files.
static void verifyCardWrite(){
  if(!sdOK){Serial.println("SDTEST no volume");return;}
  const char *path="/FDVERIFY.TMP";
  FsFile f=card.open(path,O_CREAT|O_EXCL|O_RDWR);
  if(!f){Serial.println("SDTEST create failed or test file already exists");return;}
  uint8_t sample[512],copy[512];
  uint32_t seed=esp_random();
  for(uint8_t &b:sample){seed=1664525u*seed+1013904223u;b=seed>>24;}
  bool ok=f.write(sample,sizeof(sample))==sizeof(sample);
  bool closed=f.close();
  mountCard();
  FsFile readback;
  if(sdOK)readback=card.open(path,O_RDONLY);
  bool match=readback && readback.fileSize()==sizeof(sample) &&
    readback.read(copy,sizeof(copy))==int(sizeof(copy)) && !memcmp(copy,sample,sizeof(copy));
  readback.close();
  bool removed=sdOK && card.remove(path);
  sdWriteFailed=!(ok && closed && match);
  Serial.printf("SDTEST write=%d close=%d remount=%d verified=%u removed=%d\n",ok,closed,sdOK,match?512u:0u,removed);
}
static void status(){
  Serial.printf("GUI page=%d menu=%d card=%d sleep=%d minutes=%ld theme=%ld lang=%s wxBusy=%d rain=%d heat=%d\n",page,menuOpen,cardOpen,sleeping,cfg[SAVER].toInt(),cfg[THEME].toInt(),cfg[LANG].c_str(),wxBusy,wxCache[0].frames,wxCache[1].frames);
  MapView v=mapView();
  Serial.printf("MapIO source=%s fat=%d wp=%d install=%d written=%u probe=%d reopenSize=%u read=%d err=%u/%u sdfat=%s fatfs=%s diff=%d actual=%d expected=%d\n",
    mapFromSD?"SD":"flash",sdOK?card.fatType():0,writeProtect,installedMap,writtenLength,sameHandle,storedLength,mapReadCount,ioCode,ioData,probeStatus,nativeStatus,diffByte,actualByte,diffByte<0?-1:flightdeck_map_start[diffByte]);
  Serial.printf("Map %s %s bytes=%u places=%u zoom=%.1f NM",mapStatus,mapFilename,mapProgress,atlas?atlas->places:0,weatherPage()?viewRadius(weatherZoomStep):v.nm);
  if(traffic.count){
    bool inside; float km;
    const Plane &p=traffic.planes[selected];
    const char *town=placeBelow(p,inside,km);
    Serial.printf(" town=%s inside=%d state=%s",town?town:"--",inside,flightState(p));
  }
  Serial.println();
  Serial.printf("Network configured=%d keySet=%d reason=%u scan=%d matchChannel=%d auth=%d clock=%d\n",
    !cfg[SSID].isEmpty(),!cfg[PASS].isEmpty(),disconnectReason,seenNetworks,matchingChannel,matchingAuth,time(nullptr)>1700000000);
  Serial.printf("ADS-B period=%ld used=%u/500 last=%u retryAt=%u next=%u busy=%d storage=%d\n",
    cfg[PERIOD].toInt(),budget.usedToday(time(nullptr)),budget.last,budget.retryAt,
    budget.nextAllowed(cfg[PERIOD].toInt()),busy,budgetReady);
  if(unknownBarometer.length() && sensor=="--") Serial.println(unknownBarometer);
  Serial.printf("FlightDeck I2C=[%s] baro=%s AHT=%d sht=%d temp=%.2f hPa=%.2f RH=%.1f GPS_RX=%s baud=%d crc=%lu fix=%d sat=%lu SD_MB=%u volume_MiB=%u FS=%d SDerr=%d/%d BAT=%.3f touch=%d kb=%d lora=%d mic=%d peak=%d spk=%d psram=%u wifi=%d api=%d planes=%d\n",
    busList.c_str(),sensor.c_str(),ahtOK,shtOK,temperature,pressure,humidity,cfg[GPS_PIN].c_str(),activeRate,gps.passedChecksum(),hasFix(),gps.satellites.value(),
    cardMiB,volumeMiB,sdOK,card.sdErrorCode(),card.sdErrorData(),voltage,touchOK,kbOK,radioOK,micOK,micPeak,speakerOK,ESP.getPsramSize(),int(WiFi.status()),apiCode,traffic.total);
}
void setup(){
  Serial.begin(115200);
  prefs.begin("flightdeck",false);
  for(int i=0;i<FIELDS;i++){String k="v"+String(i); cfg[i]=prefs.getString(k.c_str(),defaults[i]);}
  if(cfg[PERIOD].toInt()<trafficMinPeriod || cfg[PERIOD].toInt()>300)cfg[PERIOD]=defaults[PERIOD];
  loadBudget();
  setPalette();lastActivity=millis();
  startBoard(cfg[LIGHT].toInt()); startGps();
  WiFi.persistent(false); WiFi.mode(WIFI_STA);
  WiFi.onEvent([](WiFiEvent_t,WiFiEventInfo_t info){disconnectReason=info.wifi_sta_disconnected.reason;},ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
  if(cfg[ZONE]=="Budapest") esp_wifi_set_country_code("HU",true);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setAutoReconnect(false); network(); zone();
  networkGate=xSemaphoreCreateMutex();wxJobs=xQueueCreate(1,sizeof(WxRequest));wxReplies=xQueueCreate(1,sizeof(WxFrame));
  xTaskCreatePinnedToCore(wxWorker,"weather",14336,nullptr,1,nullptr,0);
  requests=xQueueCreate(1,sizeof(TrafficRequest)); results=xQueueCreate(1,sizeof(Traffic));
  xTaskCreatePinnedToCore(trafficWorker,"adsb",14336,nullptr,1,nullptr,0);
  if(!cfg[SSID].length()) page=2;
}
void loop(){
  static uint32_t lastInput,lastFrame,lastSample,lastStatus;
  uint32_t now=millis();
  readGps(); readAht(); weatherTick(); handleTraffic(); networkProbe();
  if(now-lastInput>=28){lastInput=now; if(bench.active)benchInput();else input();}
  if(!bench.active && !sleeping && cfg[SAVER].toInt()>0 && millis()-lastActivity>=60000UL*cfg[SAVER].toInt())startSaver();
  if(now-lastSample>=1500){lastSample=now; sampleEnvironment(); sampleInputs();}
  if(dirty || now-lastFrame>=1000){dirty=false;lastFrame=now; render();}
  if(now-lastStatus>=10000 || Serial.available()){
    while(Serial.available()){
      int c=Serial.read();
      if(c=='X') benchStart();
      if(c=='Y') benchReport();
      if(c=='E' && bench.active){benchReport();bench.active=false;dirty=true;}
      if(c=='V') verifyCardWrite();
      if((editing && field==SAVER && isDigit(c)) || strchr("12345+-ldufmsjk",c) || c==13)key(c);
      if(c=='!'){
        render();
        Serial.print("P6\n320 240\n255\n");
        uint8_t row[320*3];
        for(int y=0;y<240;y++){
          for(int x=0;x<320;x++){
            uint16_t rgb=screen.readPixel(x,y);
            row[3*x]=((rgb>>11)*255)/31; row[3*x+1]=(((rgb>>5)&63)*255)/63; row[3*x+2]=((rgb&31)*255)/31;
          }
          Serial.write(row,sizeof(row));
        }
      }
    }
    lastStatus=now; status();
  }
  if(!choosingNetwork && !probePending && cfg[SSID].length() && WiFi.status()!=WL_CONNECTED && millis()-lastNetAttempt>45000) network();
  logEnvironment(); delay(2);
}
