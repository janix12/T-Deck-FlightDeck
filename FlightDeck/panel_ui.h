#pragma once

static void chevron(int x,int y,uint16_t color,int direction=1){
  for(int i=0;i<=5;++i) {
    screen.drawFastHLine(x+direction*i,y-5+i,2,color);
    screen.drawFastHLine(x+direction*i,y+5-i,2,color);
  }
}

static void actionButton(int x,int y,int width,const char *title,bool focused){
  framedBox(x,y,width,32,7,land,focused?mint:grid);
  centeredLabel(x,y,width,32,title,ink);
}

static bool hardwareDetails;
static int instrumentFocus;
static void instrumentAction(int action){
  if(hardwareDetails || action==0)hardwareDetails=!hardwareDetails;
  else if(action==1)beep();
  else {mountCard();loadMap(true);}
  dirty=true;
}

static void sensorCard(int index,const char *name,float reading,int decimals,const char *unit){
  const int x=5+index*104,w=102,y=37;
  const uint16_t accent=index==0?amber:index==1?mint:ink;
  framedBox(x,y,w,86,9,land,grid);
  const int cx=x+16,cy=y+17;
  if(index==0){
    screen.drawRoundRect(cx-2,cy-8,5,13,2,accent);
    screen.fillCircle(cx,cy+5,4,accent);
    screen.drawFastVLine(cx,cy-3,8,accent);
  }else if(index==1){
    screen.fillTriangle(cx,cy-8,cx-6,cy+1,cx+6,cy+1,accent);
    screen.fillCircle(cx,cy+2,6,accent);
    screen.drawPixel(cx-3,cy+3,land);
  }else{
    screen.drawCircle(cx,cy,7,accent);
    screen.drawLine(cx,cy,cx+4,cy-4,accent);
    screen.fillCircle(cx,cy,2,accent);
  }
  const int ux=x+w-10-labelWidth(unit,2);
  if(index==0)screen.drawCircle(ux-3,y+12,2,muted);
  label(ux,y+7,unit,muted);
  String value=number(reading,decimals);
  if(!english())value.replace('.',',');
  label(x+10,y+28,value,isfinite(reading)?ink:muted,labelWidth(value,4)<=w-18?4:2);
  label(x+10,y+62,name,muted);
}

static void hardwareRow(int y,const char *title,const String &value){
  label(12,y,title,muted,1);
  label(74,y-1,fitText(value,2,233),ink);
}

static void diagnostics(){
  String gpsState=hasFix()?"FIX":lastSentence?tr("NMEA / fixre vár","NMEA / waiting for fix"):tr("Jelet keres","Searching");
  String storage=sdOK?String(volumeMiB)+" MiB":cardMiB?tr("FS hiba","FS error"):"--";
  if(hardwareDetails){
    label(9,34,tr("Hardveradatok","Hardware details"),mint);
    screen.fillRoundRect(5,56,310,143,7,land);
    hardwareRow(60,"I2C",busList);
    hardwareRow(80,"GPS","RX"+cfg[GPS_PIN]+"  "+String(activeRate)+"  "+gpsState);
    hardwareRow(100,tr("Tároló / táp","SD / power"),storage+"   "+number(voltage,2)+" V");
    hardwareRow(120,tr("Bevitel","Input"),String("GT911 ")+(touchOK?"OK":"--")+tr("   Bill. ","   Keys ")+(kbOK?"OK":"--"));
    hardwareRow(140,"LoRa / Mic",(radioOK?String(radioPackets)+tr(" csomag"," packets"):"--")+"   Mic "+(micOK?String(micPeak):"--"));
    hardwareRow(160,"Wi-Fi",WiFi.status()==WL_CONNECTED?WiFi.localIP().toString():wifiStatusText());
    hardwareRow(180,"ADS-B",String(budget.usedToday(time(nullptr)))+"/500 UTC   "+(WiFi.status()==WL_CONNECTED?String(WiFi.RSSI())+" dBm":"--"));
    actionButton(5,203,310,tr("Vissza","Back"),true);
    return;
  }
  sensorCard(0,tr("Hőmérséklet","Temperature"),temperature,1,"C");
  sensorCard(1,tr("Páratartalom","Humidity"),humidity,0,"%");
  sensorCard(2,tr("Légnyomás","Pressure"),pressure,1,"hPa");
  String sensors=sensor;
  if(ahtOK || shtOK){if(sensors=="--")sensors="";else sensors+="  /  ";sensors+=ahtOK?"AHT2x":"SHT3x";}
  label(10,128,tr("Szenzorok","Sensors"),muted,1);
  label(80,128,sensors,ink,1);
  framedBox(5,148,310,46,8,land,instrumentFocus==0?mint:grid);
  label(15,151,tr("Hardveradatok","Hardware details"),ink);
  label(15,174,"GPS "+gpsState+"   /   SD "+storage,muted,1);
  chevron(298,171,mint);
  actionButton(5,203,151,tr("Hangpróba [B]","Speaker [B]"),instrumentFocus==1);
  actionButton(162,203,153,tr("SD újra [R]","Retry SD [R]"),instrumentFocus==2);
}

// Visual order is independent of the preference keys stored on existing devices.
static constexpr Field settingsOrder[]={SSID,PASS,WIFI_LIST,ZONE,CLOCK,FW_UPDATE,
  LAT,LON,RANGE,PERIOD,BAUD,GPS_PIN,LANG,THEME,LIGHT,KEY_LIGHT,SAVER,LOGGING};
static_assert(sizeof(settingsOrder)/sizeof(*settingsOrder)==FIELDS,"Missing settings row");
static int settingsPosition(){
  for(int i=0;i<FIELDS;i++)if(settingsOrder[i]==field)return i;
  return 0;
}
static constexpr int settingsRowTop=52,settingsPitch=26;
static void settingIcon(int f,int x,int y,uint16_t color){
  if(f==WIFI_LIST || f==FW_UPDATE){chevron(x-2,y,color);return;}
  if(f==LANG || f==THEME){
    screen.drawFastHLine(x-5,y-3,10,color);screen.drawFastHLine(x-5,y+3,10,color);
    screen.fillTriangle(x+5,y-6,x+5,y,x+8,y-3,color);
    screen.fillTriangle(x-5,y,x-5,y+6,x-8,y+3,color);return;
  }
  if(f==LOGGING){
    bool enabled=cfg[LOGGING]=="1";
    screen.drawRoundRect(x-9,y-5,19,11,5,color);screen.fillCircle(x+(enabled?4:-4),y,3,color);return;
  }
  screen.drawLine(x-4,y+3,x+3,y-4,color);screen.drawLine(x-3,y+4,x+4,y-3,color);
  screen.fillTriangle(x-5,y+2,x-2,y+5,x-6,y+6,color);screen.drawLine(x+4,y-6,x+6,y-4,color);
}
static void settingsList(){
  int group=settingsPosition()/6;
  const char *title=group==0?tr("Kapcsolat és rendszer","Network and system"):
                    group==1?tr("Térkép és GPS","Map and GPS"):tr("Kijelző és eszköz","Display and device");
  label(10,32,title,mint);
  for(int row=0;row<6;row++){
    Field f=settingsOrder[group*6+row];int y=settingsRowTop+row*settingsPitch;
    bool current=field==f;
    screen.fillRoundRect(5,y,310,24,5,current?grid:land);
    if(current)screen.fillRoundRect(8,y+5,3,14,1,mint);
    String value=f==LOGGING?(cfg[f]=="1"?tr("Be","On"):tr("Ki","Off")):valueFor(f);
    const char *name=f==LOGGING?tr("SD-naplózás","SD logging"):fieldLabel(f);
    label(16,y+3,fitText(name,2,137),ink);
    int font=labelWidth(value,2)<=132?2:1;
    value=fitText(value,font,132);
    int w=labelWidth(value,font);
    centeredLabel(285-w,y,w,24,value,current?ink:muted,font);
    settingIcon(f,301,y+12,current?mint:muted);
  }
  pill(5,212,50,25);chevron(31,224,mint,-1);
  centeredLabel(55,212,210,25,String(group+1)+" / 3",muted);
  pill(265,212,50,25);chevron(286,224,mint);
}
