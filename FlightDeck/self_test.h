#pragma once

// USB X starts an isolated input test, E ends it. Keep settings and selected page intact.
struct BenchResults {
  bool active,press,oldBall[5];
  unsigned keys,taps,ball[5],peak;
  int touchX,touchY;
  uint32_t since,gpsStart;
};
static BenchResults bench;

static void benchReport(){
  Serial.printf("BENCH events keys=%u taps=%u xy=%d,%d ball=%u,%u,%u,%u push=%u micMax=%u GPSdelta=%lu lostCRC=%lu fix=%d packets=%d\n",
    bench.keys,bench.taps,bench.touchX,bench.touchY,bench.ball[0],bench.ball[1],bench.ball[2],bench.ball[3],bench.ball[4],
    bench.peak,gps.passedChecksum()-bench.gpsStart,gps.failedChecksum(),hasFix(),radioPackets);
}
static void benchStart(){
  bench={};bench.active=true;bench.since=millis();bench.gpsStart=gps.passedChecksum();
  for(int i=0;i<5;i++)bench.oldBall[i]=digitalRead(ballPins[i]);
  const unsigned words=256*1024;
  uint32_t *ram=(uint32_t*)ps_malloc(words*sizeof(uint32_t));bool memoryOK=ram;
  if(ram){
    for(uint32_t mask:{0x5a1de0d5u,0xa5e21f2au}){
      for(unsigned i=0;i<words;i++)ram[i]=mask^(i*2654435761u);
      for(unsigned i=0;i<words;i++)if(ram[i]!=(mask^(i*2654435761u))){memoryOK=false;break;}
    }
    free(ram);
  }
  Preferences scratch;uint32_t token=esp_random();
  bool nvs=scratch.begin("fdtest",false);
  if(nvs){nvs=scratch.putUInt("probe",token)==sizeof(token);scratch.end();}
  if(nvs){nvs=scratch.begin("fdtest",false);if(nvs){nvs=scratch.getUInt("probe",~token)==token && scratch.remove("probe");scratch.end();}}
  uint16_t expected=0x5aab;screen.drawPixel(0,0,expected);
  uint16_t pixel=screen.readPixel(0,0);
  Serial.printf("BENCH memory psramBytes=%u testedBytes=%u patterns=2 pass=%d NVS=%d flash=%u TFT=%dx%d pixel=%d\n",
    ESP.getPsramSize(),ram?words*4:0,memoryOK,nvs,ESP.getFlashChipSize(),panel.width(),panel.height(),pixel==expected);
  Serial.printf("BENCH sensors bmpId=%d bmpT=%.2f bmpHpa=%.2f ahtCRC=%d ahtT=%.2f humidity=%.1f batteryADC=%.3f sensorBus=%s\n",
    regByte(0x77,0xd0),sensor=="BMP280"?bmp.readTemperature():NAN,sensor=="BMP280"?bmp.readPressure()/100.f:NAN,
    ahtOK,ahtTemp,ahtRH,voltage,busList.c_str());
  Serial.printf("BENCH links WiFi=%d RSSI=%d epoch=%ld API=%d GNSSbaud=%d loraInit=%d loraRSSI=%.1f micInit=%d speakerInit=%d\n",
    WiFi.status(),WiFi.RSSI(),(long)time(nullptr),apiCode,activeRate,radioOK,radioOK?radio.getRSSI(false):NAN,micOK,speakerOK);
  beep();dirty=true;
}
static void benchInput(){
  readKeyboard([](int ch){++bench.keys;if(ch=='b'||ch=='B')beep();});
  bool down=touchOK && touch.isPressed();
  if(down){int16_t x,y;if(touch.getPoint(&x,&y,1)){if(!bench.press)++bench.taps;bench.touchX=x;bench.touchY=y;}}
  bench.press=down;
  for(int i=0;i<5;i++){
    bool state=digitalRead(ballPins[i]);
    if(state!=bench.oldBall[i] && (i!=4 || !state))++bench.ball[i];
    bench.oldBall[i]=state;
  }
  bench.peak=std::max(bench.peak,(unsigned)micPeak);
  if(millis()-bench.since>=150000){benchReport();bench.active=false;dirty=true;}
}
static void benchScreen(){
  screen.fillSprite(bg);label(9,7,"T-Deck hardverteszt",mint,4);
  label(10,49,"Erintsd a kijelzot, nyomj B-t.");label(10,72,"Gorgess 4 iranyba, majd nyomd le.");
  label(10,110,"Billentyu: "+String(bench.keys)+"     Erintes: "+String(bench.taps),amber);
  label(10,138,"Gorgo: "+String(bench.ball[0])+" / "+String(bench.ball[1])+" / "+String(bench.ball[2])+" / "+String(bench.ball[3]));
  label(10,164,"Lenyomas: "+String(bench.ball[4])+"     Mik.: "+String(bench.peak));
  label(10,188,tr("Akkuikon-próba","Battery icon test"),muted,1);
  drawBatteryIcon(131,184,100,false);
  drawBatteryIcon(177,184,-1,true);
  label(10,206,"A teszt automatikusan kilep.",muted);
  screen.pushSprite(0,0);
}
