#pragma once
static void savedBack(){
  notice="";
  if(wifiScreen==WifiScreen::forget)wifiScreen=WifiScreen::detail;
  else if(wifiScreen==WifiScreen::detail)wifiScreen=WifiScreen::list;
  else wifiScreen=WifiScreen::closed;
}
static void savedAction(){
  if(!wifiBook.count || savedIndex>=wifiBook.count)return;
  notice="";
  switch(wifiScreen){
    case WifiScreen::list:wifiScreen=WifiScreen::detail;break;
    case WifiScreen::detail:
      if(joinSaved(savedIndex))wifiScreen=WifiScreen::closed;
      else notice=tr("Mentési hiba","Save failed");
      break;
    case WifiScreen::forget:
      if(forgetSaved(savedIndex)){
        savedIndex=max(0,min(savedIndex,int(wifiBook.count)-1));wifiScreen=WifiScreen::list;
      }else notice=tr("Törlési hiba","Could not remove");
      break;
    default:break;
  }
}
static void drawWifiBook(){
  label(10,36,tr("Mentett hálózatok","Saved networks"),mint,2);
  if(wifiScreen==WifiScreen::list){
    int top=savedIndex/4*4;
    if(!wifiBook.count)label(10,76,tr("Még nincs mentett hálózat.","No saved networks yet."),muted);
    for(int i=top;i<min(top+4,int(wifiBook.count));i++){
      int y=62+(i-top)*28;
      if(i==savedIndex)screen.fillRoundRect(6,y-2,308,26,4,grid);
      String name=wifiBook.entries[i].ssid;
      if(WiFi.status()==WL_CONNECTED && WiFi.SSID()==name)screen.fillCircle(16,y+10,4,mint);
      label(27,y+2,name.substring(0,labelPrefixBytes(name,2,278)));
    }
    if(wifiBook.count>4){
      pill(8,173,50,24);centeredLabel(8,173,50,24,"<",ink);
      centeredLabel(60,173,200,24,String(top/4+1)+" / 2",muted,1);
      pill(262,173,50,24);centeredLabel(262,173,50,24,">",ink);
    }
    saveButton.draw(tr("Új hálózat","Add network"),mint,mint,bg);
  }else{
    String name=wifiBook.entries[savedIndex].ssid;
    label(10,66,name.substring(0,labelPrefixBytes(name,2,300)),ink);
    if(wifiScreen==WifiScreen::forget)
      label(10,108,tr("Törlöd ezt a mentett hálózatot?","Forget this saved network?"),amber);
    else{
      label(10,102,WiFi.status()==WL_CONNECTED && WiFi.SSID()==name?tr("Kapcsolódva","Connected"):
            tr("Mentve","Saved"),muted);
      pill(8,141,148,31);centeredLabel(8,141,148,31,tr("Törlés...","Forget..."),amber);
    }
    label(10,174,notice,amber);
    saveButton.draw(wifiScreen==WifiScreen::forget?tr("Törlés","Forget"):tr("Csatlakozás","Connect"),mint,mint,bg);
  }
  cancelButton.draw(wifiScreen==WifiScreen::forget?tr("Mégse","Cancel"):tr("Vissza","Back"),bg,muted,ink);
}
static void savedTouch(int x,int y){
  if(cancelButton.contains(x,y)){savedBack();return;}
  if(wifiScreen==WifiScreen::list){
    if(saveButton.contains(x,y)){
      wifiScreen=WifiScreen::closed;field=SSID;editing=true;edit="";pickedAuth=-1;pickedSsid="";notice="";
      probePending=false;WiFi.scanDelete();startScan();networkIndex=0;choosingNetwork=true;
    }else if(y>=173 && y<197 && wifiBook.count>4 && (x<59 || x>=262))savedIndex=savedIndex<4?4:0;
    else if(y>=60 && y<172){
      int i=savedIndex/4*4+(y-60)/28;
      if(i<wifiBook.count){savedIndex=i;savedAction();}
    }
  }else if(saveButton.contains(x,y))savedAction();
  else if(wifiScreen==WifiScreen::detail && x>=8 && x<156 && y>=141 && y<172)wifiScreen=WifiScreen::forget;
}
