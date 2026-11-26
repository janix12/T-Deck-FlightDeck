#pragma once

static int mapWidth=320,midX=160;
static constexpr int mapTop=0,mapHeight=240,midY=120;
static int zoomStep,weatherZoomStep;
static bool cardList;
static int detailScroll,listScroll,listCursor;
static constexpr int cardX=190,cardBodyY=58,cardBodyH=176,listRow=29,detailHeight=314;
static constexpr int cardTop=29,listTabX=cardX+55,closeButtonWidth=30;
static int scrollLimit(){
  return max(0,(cardList?traffic.count*listRow:detailHeight)-cardBodyH);
}
static void scrollCard(int pixels){
  int &offset=cardList?listScroll:detailScroll;
  offset=constrain(offset+pixels,0,scrollLimit());dirty=true;
}
static void cardMode(bool list){
  cardOpen=true;cardList=list;listCursor=selected;
  if(list) listScroll=constrain(selected*listRow-listRow,0,scrollLimit());
  dirty=true;
}
static void pickPlane(int i){
  if(i<0 || i>=traffic.count) return;
  cardOpen=true;selected=listCursor=i;detailScroll=0;cardList=false;dirty=true;
}
static void cardBall(int direction){
  if(!cardList){scrollCard(direction*19);return;}
  listCursor=constrain(listCursor+direction,0,max(0,traffic.count-1));
  if(listCursor*listRow<listScroll) listScroll=listCursor*listRow;
  if((listCursor+1)*listRow>listScroll+cardBodyH) listScroll=(listCursor+1)*listRow-cardBodyH;
  dirty=true;
}
struct MapView {
  float lat,lon,nm,pixelsPerDeg,lonFactor;
  float units=5000.f;
  int x(float lng) const {return midX+lroundf(remainderf(lng-lon,360)*lonFactor);}
  int y(float latitude) const {return midY-lroundf((latitude-lat)*pixelsPerDeg);}
  int mapX(int v) const {return midX+lroundf((v-(lon-19)*units)*(lonFactor/units));}
  int mapY(int v) const {return midY-lroundf((v-(lat-47)*units)*(pixelsPerDeg/units));}
};
static float viewRadius(int level){
  return constrain(ldexpf(float(cfg[RANGE].toInt()),-level),1.f,250.f);
}
static MapView mapView(){
  Area area=traffic.stamp?traffic.area:currentArea();
  MapView v{};
  v.lat=area.lat; v.lon=area.lon;
  if(zoomStep>0 && traffic.count){v.lat=traffic.planes[selected].lat;v.lon=traffic.planes[selected].lon;}
  v.nm=viewRadius(zoomStep);
  v.pixelsPerDeg=111.195f*(mapHeight/2)/(1.852f*v.nm);
  v.lonFactor=v.pixelsPerDeg*cosf(radians(v.lat));
  return v;
}
static void zoomMap(int direction){
  int &level=page>=3?weatherZoomStep:zoomStep;
  if(viewRadius(level)==viewRadius(level+direction)) return;
  level+=direction;
  if(page==0 && (viewRadius(level)>cfg[RANGE].toInt() || level<=0)) nextFetch=millis();
  dirty=true;
}
static void drawPlane(int x,int y,float track,bool active,bool stale){
  const uint16_t color=active?amber:mint;
  // Material Design Icons airplane (Pictogrammers, Apache-2.0), native nose NE.
  static const int8_t outline[][2]={{9,-9},{10,-8},{9,-6},{5,-2},{7,7},{5,9},{1,1},{-3,5},{-2,8},{-3,9},{-5,6},{-9,4},{-8,2},{-5,3},{-1,-1},{-9,-5},{-7,-7},{2,-5},{6,-9},{8,-10}};
  const float h=radians((isfinite(track)?track:0.f)-45.f),sn=sinf(h),cs=cosf(h);
  const int n=sizeof(outline)/sizeof(*outline);
  float xs[n],ys[n];float size=active?1.03f:.81f;
  for(int i=0;i<n;i++){
    xs[i]=size*(cs*outline[i][0]-sn*outline[i][1]);
    ys[i]=size*(sn*outline[i][0]+cs*outline[i][1]);
  }
  if(stale){
    for(int i=0,j=n-1;i<n;j=i++)
      screen.drawLine(x+lroundf(xs[j]),y+lroundf(ys[j]),x+lroundf(xs[i]),y+lroundf(ys[i]),color);
  }else for(int dy=-12;dy<=12;dy++){
    float cuts[n];int k=0;float scan=dy+.5f;
    for(int i=0,j=n-1;i<n;j=i++)
      if((ys[i]>scan)!=(ys[j]>scan))cuts[k++]=xs[i]+(scan-ys[i])*(xs[j]-xs[i])/(ys[j]-ys[i]);
    std::sort(cuts,cuts+k);
    for(int i=0;i+1<k;i+=2){
      int left=ceilf(cuts[i]),right=floorf(cuts[i+1]);
      if(right>=left)screen.drawFastHLine(x+left,y+dy,right-left+1,color);
    }
  }
  if(active)screen.drawCircle(x,y,13,amber);
  if(!isfinite(track)) mapLabel(x+8,y-11,"?",color,1);
}
static void cardField(int y,const char *title,const String &value){
  if(y+30<cardBodyY || y>=cardBodyY+cardBodyH) return;
  label(cardX+6,y,title,muted,1);
  label(cardX+6,y+9,value,ink,2);
}
static void drawCard(const Plane *p,const char *town,bool inside,float distance){
  framedBox(cardX,cardTop,130,211,7,bg,grid);
  screen.fillRoundRect(cardList?listTabX:cardX+2,32,cardList?73:52,23,4,grid);
  centeredLabel(cardX+2,32,52,23,tr("Gép","Flight"),cardList?muted:mint);
  centeredLabel(listTabX,32,73,23,String(tr("Lista ","List "))+String(traffic.count),cardList?mint:muted);
  screen.setViewport(cardX+2,cardBodyY,125,cardBodyH,false);
  if(!p) label(cardX+7,65,tr("Nincs gép","No aircraft"),muted);
  else if(cardList){
    listScroll=constrain(listScroll,0,scrollLimit());
    for(int i=listScroll/listRow;i<traffic.count;i++){
      int y=cardBodyY+i*listRow-listScroll;
      if(y>=cardBodyY+cardBodyH) break;
      const Plane &q=traffic.planes[i];
      if(i==listCursor) screen.fillRoundRect(cardX+3,y,120,listRow-1,3,grid);
      label(cardX+7,y,q.name,i==selected?amber:ink);
      label(cardX+7,y+16,number(q.km,1)+" km",muted,1);
      screen.drawFastHLine(cardX+6,y+listRow-1,110,grid);
    }
  } else{
    int y=cardBodyY+3-detailScroll;
    int font=labelWidth(p->name,4)<=112?4:2;
    label(cardX+6,y,p->name,amber,font);
    String name=town?town:atlas?tr("Csomagon kívül","Outside map"):mapStatus;
    label(cardX+6,y+32,inside?tr("TELEPÜLÉS","MUNICIPALITY"):town?tr("KÖZELÉBEN","NEAREST TOWN"):tr("TÉRKÉP","MAP"),muted,1);
    if(labelWidth(name,2)<=115) label(cardX+6,y+42,name,ink);
    else {
      const int split=labelPrefixBytes(name,1,115);
      label(cardX+6,y+42,name.substring(0,split),ink,1);
      label(cardX+6,y+53,name.substring(split),ink,1);
    }
    cardField(y+67,tr("ÁLLAPOT","STATUS"),stateText(*p));
    cardField(y+98,tr("MAGASSÁG","ALTITUDE"),p->ground?tr("Foldon","Ground"):number(p->altitude)+" m");
    cardField(y+129,tr("SEBESSÉG","SPEED"),number(p->speed)+" km/h");
    cardField(y+160,tr("TÁVOLSÁG","DISTANCE"),number(p->km,1)+" km");
    cardField(y+191,tr("HALADÁSI IRÁNY","TRACK"),number(p->heading)+" deg");
    cardField(y+222,tr("FÜGGŐLEGES SEB.","VERTICAL RATE"),number(p->verticalRate)+" ft/min");
    cardField(y+253,tr("ICAO / AZONOSÍTÓ","ICAO ADDRESS"),p->id);
    if(town && !inside) cardField(y+284,tr("HELYSÉG TÁVOLSÁGA","TOWN DISTANCE"),number(distance,1)+" km");
    else label(cardX+6,y+287,tr("* Becsült állapot","* Estimated state"),muted,1);
  }
  screen.resetViewport();
  if(p && scrollLimit()){
    int span=cardBodyH, limit=scrollLimit(), thumb=max(10,span*span/(limit+span));
    int offset=cardList?listScroll:detailScroll;
    screen.drawFastVLine(317,cardBodyY,span,grid);
    screen.drawFastVLine(317,cardBodyY+offset*(span-thumb)/limit,thumb,muted);
  }
}
extern "C" const uint8_t flightdeck_overview_start[];
static void drawGeography(const MapView &v,const char *selectedTown,const MapHeader *data=atlas){
  if(!data) return;
  const auto *towns=reinterpret_cast<const MapPlace*>(data+1);
  const auto *lines=reinterpret_cast<const MapPath*>(towns+data->places);
  const auto *points=reinterpret_cast<const MapPoint*>(lines+data->paths);
  const char *names=reinterpret_cast<const char*>(points+data->points);
  for(unsigned i=0;i<data->paths;i++){
    const MapPath &p=lines[i];
    const bool highlight=selectedTown && (names+p.name)==selectedTown && p.kind>=4;
    if(p.kind>=4 && !highlight){
      if(v.nm>(page>=3?15:70))continue;
      if(v.nm>30 && v.mapX(p.right)-v.mapX(p.left)<12)continue;
    }
    if(v.mapX(p.right)<0 || v.mapX(p.left)>=mapWidth || v.mapY(p.top)>=mapTop+mapHeight || v.mapY(p.bottom)<mapTop) continue;
    uint16_t color=p.kind<=2?river:highlight?amber:edge;
    const MapPoint *pts=points+p.first;
    int x=v.mapX(pts[0].x),y=v.mapY(pts[0].y);
    for(unsigned j=1;j<p.count;j++){
      int nx=v.mapX(pts[j].x),ny=v.mapY(pts[j].y);
      if((nx!=x || ny!=y) && max(x,nx)>=0 && min(x,nx)<mapWidth &&
         max(y,ny)>=mapTop && min(y,ny)<mapTop+mapHeight) screen.drawLine(x,y,nx,ny,color);
      x=nx;y=ny;
    }
  }
  struct Box {int left,top,right,bottom;};
  Box used[16]; int count=0;
  for(unsigned i=0;i<data->places && count<16;i++){
    const MapPlace &p=towns[i]; int x=v.mapX(p.at.x),y=v.mapY(p.at.y);
    if(x<2 || y<mapTop+6 || x>=mapWidth-4 || y>=mapTop+mapHeight-6) continue;
    const char *name=names+p.name;
    Box b{x+3,y-4,x+3+int(strlen(name))*6,y+5};
    if(b.right>=mapWidth) {b.right=x-3;b.left=b.right-int(strlen(name))*6;}
    if(b.left<0) continue;
    bool free=true;
    for(int k=0;k<count;k++) if(b.left<=used[k].right+3 && b.right+3>=used[k].left &&
       b.top<=used[k].bottom+3 && b.bottom+3>=used[k].top){free=false;break;}
    if(free){screen.fillCircle(x,y,1,muted); if(page>=3)mapLabel(b.left+1,b.top+1,name,bg,1);
      mapLabel(b.left,b.top,name,page>=3?ink:muted,1);used[count++]=b;}
  }
}
constexpr int mapButtonHeight=29, mapButtonY=240-3-mapButtonHeight;
static void zoomButtons(int width){
  for(int i=0;i<2;i++){
    int x=width-67+i*32;pill(x,mapButtonY,30,mapButtonHeight);
    screen.fillRect(x+8,mapButtonY+14,14,2,ink);if(i)screen.fillRect(x+14,mapButtonY+8,2,14,ink);
  }
}
static void radar(){
  mapWidth=cardOpen?188:320;midX=mapWidth/2;
  MapView v=mapView();
  bool inside=false; float distance=INFINITY;
  const Plane *p=traffic.count?&traffic.planes[selected]:nullptr;
  const char *town=p?placeBelow(*p,inside,distance):nullptr;
  screen.setViewport(0,mapTop,mapWidth,mapHeight,false);
  screen.fillRect(0,mapTop,mapWidth,mapHeight,land);
  drawGeography(v,inside?town:nullptr);
  Area center=traffic.stamp?traffic.area:currentArea();
  int ox=v.x(center.lon),oy=v.y(center.lat);
  screen.drawCircle(ox,oy,4,mint);screen.drawPixel(ox,oy,mint);
  const time_t now=time(nullptr);
  bool stale=traffic.stamp && now-traffic.stamp>cfg[PERIOD].toInt()+30;
  for(int i=traffic.count-1;i>=-1;i--){
    if(!p || i==selected) continue;
    const Plane &q=i<0?*p:traffic.planes[i];
    int x=v.x(q.lon),y=v.y(q.lat);
    if(x>=-9 && x<mapWidth+9 && y>=mapTop-9 && y<mapTop+mapHeight+9) drawPlane(x,y,q.heading,i<0,now-q.observed>90);
  }

  float scale=1;
  for(int n:{1,2,5,10,20,50,100,200}) if(n*1.852f*v.pixelsPerDeg/111.195f<=54) scale=n;
  int bar=lroundf(scale*1.852f*v.pixelsPerDeg/111.195f);
  const int scaleWidth=max(bar+12,56);
  pill(5,mapButtonY,scaleWidth,mapButtonHeight);
  centeredLabel(5,mapButtonY+5,scaleWidth,15,String(int(scale))+" NM",ink,2);
  screen.drawFastHLine(5+(scaleWidth-bar)/2,mapButtonY+22,bar,ink);
  pill(72,mapButtonY,29,mapButtonHeight);
  for(int i=0;i<3;i++){
    const int row=mapButtonY+10+i*4;
    screen.fillRect(79,row,2,2,mint);
    screen.fillRect(84,row,10,2,ink);
  }
  zoomButtons(mapWidth);
  screen.resetViewport();
  if(cardOpen)drawCard(p,town,inside,distance);
  else if(p){
    int x=v.x(p->lon),y=v.y(p->lat);
    if(x>=0 && x<268 && y>34 && y<196){int w=screen.textWidth(p->name,1)+8;pill(x+13,y-9,w,16);mapLabel(x+17,y-5,p->name,amber,1);}
  }
  if(!traffic.stamp || stale || apiCode!=200 || WiFi.status()!=WL_CONNECTED ||
     budget.usedToday(now)>=trafficDailyLimit || budget.retryAt>uint32_t(now)){
    constexpr int top=185,height=20;
    String msg=feedStatus();
    int textWidth=labelWidth(msg,1),w=min(mapWidth-8,textWidth+10);
    pill(4,top,w,height);
    screen.setViewport(6,top+2,w-4,height-4,false);
    centeredLabel(9,top,textWidth,height,msg,muted,1);
    screen.resetViewport();
  }
}
