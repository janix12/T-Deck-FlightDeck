#pragma once

static bool english(){ return cfg[LANG]=="EN"; }
static const char *tr(const char *hu,const char *en){ return english()?en:hu; }
static const char *fieldLabel(int i){
  static const char *en[]={"Wi-Fi name","Wi-Fi password","Time zone","Latitude","Longitude","Radius (NM)","Refresh (sec)","Brightness 1-16","GPS baud","GPS RX GPIO","SD logging 0/1","UTC clock","Language","Theme","Sleep (minutes)","Keyboard light (%)","Saved networks","Firmware update"};
  return english()?en[i]:labels[i];
}
static const char *themeName(){
  switch(cfg[THEME].toInt()){
    case 1: return tr("Papír","Paper");
    case 2: return tr("Alkony","Dusk");
    default: return tr("Óceán","Ocean");
  }
}
constexpr uint16_t rgb565(uint32_t c){return ((c>>8)&0xf800)|((c>>5)&0x7e0)|((c>>3)&31);}
struct Palette{uint32_t card,text,sub,line,accent,hot,land,river,edge;};
static const Palette palettes[]={
 {0x102632,0xf2f5ec,0xacc4c9,0x35505c,0x7ce6be,0xffba72,0x1b3442,0x538d9a,0x36515c},
 {0xfff9e9,0x263735,0x56675d,0xcbcab6,0x00695e,0xac441f,0xe9e3d2,0x659798,0xb3b7a3},
 {0x282431,0xfff1df,0xd3bec7,0x514252,0xf3ad85,0xb9d6ff,0x392f40,0x7b839c,0x564759}
};
static uint16_t bg,ink,muted,grid,mint,amber,land,river,edge;
static void setPalette(){
 const Palette &p=palettes[constrain(cfg[THEME].toInt(),0L,2L)];
 bg=rgb565(p.card);ink=rgb565(p.text);muted=rgb565(p.sub);grid=rgb565(p.line);
 mint=rgb565(p.accent);amber=rgb565(p.hot);land=rgb565(p.land);river=rgb565(p.river);edge=rgb565(p.edge);
}
static bool sleeping,cardOpen;
static int menuIndex;
static uint32_t lastActivity;
static bool interact(){
 lastActivity=millis();
 if(!sleeping)return true;
 sleeping=false;backlight(cfg[LIGHT].toInt());keyboardLight(cfg[KEY_LIGHT].toInt());dirty=true;return false;
}
static void startSaver(){sleeping=true;backlight(1);keyboardLight(0);dirty=true;}
static void framedBox(int x,int y,int w,int h,int radius,uint16_t fill,uint16_t border){
 screen.fillRoundRect(x,y,w,h,radius,border);
 screen.fillRoundRect(x+2,y+2,w-4,h-4,radius-2,fill);
}
static void pill(int x,int y,int w,int h){
 framedBox(x,y,w,h,6,bg,grid);
}
static String stateText(const Plane &p){
 const char *state=flightState(p);
 if(!english())return state;
 const char *hu[]={"Leszallt*","Felszallt*","Foldon","All a foldon*","Foldon mozog*","Nincs adat","Korozes?*","Levegoben","Emelkedik*","Sullyed*","Szintrepules*"};
 const char *en[]={"Landed*","Took off*","On ground","Stopped*","Taxiing*","Unknown","Circling?*","Airborne","Climbing*","Descending*","Level flight*"};
 for(unsigned i=0;i<sizeof(hu)/sizeof(*hu);++i)if(!strcmp(hu[i],state))return en[i];
 return state;
}
