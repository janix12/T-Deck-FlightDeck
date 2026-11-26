#pragma once
#include <Wire.h>
#include <HWCDC.h>
#include <SPI.h>
#include <SdFat.h>
#include <TFT_eSPI.h>
#include <TouchDrvGT911.hpp>
#include <RadioLib.h>
#include <TinyGPS++.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include <Adafruit_BMP3XX.h>
#include <Adafruit_DPS310.h>
#include <Adafruit_SHT31.h>
#include <driver/i2s.h>
#include <es7210.h>
#include "board.h"

static TFT_eSPI panel;
static TFT_eSprite screen(&panel);
static TouchDrvGT911 touch;
static SX1262 radio = new Module(RADIO_CS_PIN,RADIO_DIO1_PIN,RADIO_RST_PIN,RADIO_BUSY_PIN);
static TinyGPSPlus gps;
static Adafruit_BMP280 bmp;
static Adafruit_BME280 bme;
static Adafruit_BMP3XX bmp3;
static Adafruit_DPS310 dps;
static Adafruit_SHT31 sht;
static SdFs card;
static uint32_t cardMiB,volumeMiB;
static bool ahtPresent,ahtOK;
static uint32_t ahtStarted;
static float ahtTemp=NAN,ahtRH=NAN;
static bool sdOK, touchOK, kbOK, radioOK, micOK, speakerOK, shtOK;
static int radioPackets, micPeak;
static float voltage, temperature=NAN, pressure=NAN, humidity=NAN;
static float batteryEstimateV=NAN;
static bool externalPower;
// Approximate single-cell Li-ion charge from terminal voltage, in 10% steps.
static int batteryPercent(float volts){
  if(!isfinite(volts) || volts<2.5f || volts>4.30f)return -1;
  constexpr int mv[]={3100,3300,3420,3530,3630,3720,3800,3890,3990,4050,4190};
  const int measured=lroundf(volts*1000);
  if(measured<=mv[0])return 0;
  for(int i=1;i<11;++i){
    if(measured<=mv[i]){
      const int span=mv[i]-mv[i-1];
      return (i-1)*10+((measured-mv[i-1])*10+span/2)/span;
    }
  }
  return 100;
}
static String sensor="--", busList, unknownBarometer;
static const int ballPins[] = {BOARD_TBOX_G02,BOARD_TBOX_G01,BOARD_TBOX_G04,BOARD_TBOX_G03,BOARD_BOOT_PIN};

static bool responds(int addr) {
  Wire.beginTransmission(addr);
  return Wire.endTransmission() == 0;
}
static int regByte(int addr, uint8_t reg) {
  Wire.beginTransmission(addr);
  Wire.write(reg);
  if (Wire.endTransmission(false) || Wire.requestFrom(addr,1)!=1) return -1;
  return Wire.read();
}
static void detectSensors() {
  for (int a=8; a<120; ++a) if (responds(a)) {
    char hex[5]; snprintf(hex,sizeof(hex),"%02X ",a); busList+=hex;
  }
  for (int a : {0x76,0x77}) {
    if (!responds(a)) continue;
    int id=regByte(a,0xd0);
    if (id==0x60 && bme.begin(a)) sensor="BME280";
    else if (id==0x58 && bmp.begin(a)) sensor="BMP280";
    else if ((regByte(a,0)==0x50 || regByte(a,0)==0x60) && bmp3.begin_I2C(a)) sensor="BMP3xx";
    else if (regByte(a,0x0d)==0x10 && dps.begin_I2C(a)) sensor="DPS310";
    if (sensor!="--") break;
    char detail[64];
    snprintf(detail,sizeof(detail),"addr=%02x d0=%02x reg00=%02x reg0d=%02x",a,id,regByte(a,0),regByte(a,0x0d));
    unknownBarometer=detail;
  }
  for (int a : {0x44,0x45}) if (responds(a) && sht.begin(a)) { shtOK=true; break; }
  ahtPresent=responds(0x38);
  if(ahtPresent && Wire.requestFrom(0x38,1)==1 && !(Wire.read()&8)){
    const uint8_t init[]={0xbe,8,0};
    Wire.beginTransmission(0x38); Wire.write(init,sizeof(init)); Wire.endTransmission(); delay(10);
  }
}
static void readAht(){
  if(!ahtStarted || millis()-ahtStarted<85) return;
  uint8_t data[7];
  if(Wire.requestFrom(0x38,7)!=7){ ahtStarted=0; return; }
  for(auto &b:data) b=Wire.read();
  if((data[0]&0x80) && millis()-ahtStarted<200) return;
  ahtStarted=0;
  uint8_t crc=0xff;
  for(int i=0;i<6;i++){
    crc ^= data[i];
    for(int bit=0;bit<8;bit++) crc=(crc&0x80)?(crc<<1)^0x31:crc<<1;
  }
  if((data[0]&0x88)!=8 || crc!=data[6]){ahtOK=false;return;}
  uint32_t rawH=(uint32_t(data[1])<<12)|(uint32_t(data[2])<<4)|(data[3]>>4);
  uint32_t rawT=(uint32_t(data[3]&15)<<16)|(uint32_t(data[4])<<8)|data[5];
  ahtTemp=rawT*(200.f/1048576)-50; ahtRH=rawH*(100.f/1048576);
  temperature=ahtTemp; humidity=ahtRH; ahtOK=true;
}
static void sampleEnvironment() {
  if (sensor=="BMP280") { temperature=bmp.readTemperature(); pressure=bmp.readPressure()/100.f; }
  if (sensor=="BME280") { temperature=bme.readTemperature(); pressure=bme.readPressure()/100.f; humidity=bme.readHumidity(); }
  if (sensor=="BMP3xx" && bmp3.performReading()) { temperature=bmp3.temperature; pressure=bmp3.pressure/100.f; }
  if (sensor=="DPS310" && dps.temperatureAvailable() && dps.pressureAvailable()) {
    sensors_event_t t,p; if(dps.getEvents(&t,&p)) {temperature=t.temperature; pressure=p.pressure;}
  }
  if (shtOK) { temperature=sht.readTemperature(); humidity=sht.readHumidity(); }
  if(ahtPresent){
    if(ahtOK){temperature=ahtTemp;humidity=ahtRH;}
    const uint8_t measure[]={0xac,0x33,0};
    Wire.beginTransmission(0x38); Wire.write(measure,sizeof(measure));
    if(Wire.endTransmission()==0) ahtStarted=millis();
  }
  uint32_t mv=0;
  for(int i=0;i<8;i++) mv+=analogReadMilliVolts(BOARD_BAT_ADC);
  voltage=mv*0.00025f;
  // BAT_ADC follows the switched supply. A powered USB host sends SOFs;
  // a power-only adapter can also lift the rail above the single-cell range.
  externalPower=HWCDC::isPlugged() || (voltage>(externalPower?4.26f:4.30f) && voltage<=5.5f);
  if(externalPower || batteryPercent(voltage)<0)batteryEstimateV=NAN;
  else if(!isfinite(batteryEstimateV))batteryEstimateV=voltage;
  else batteryEstimateV+=(voltage-batteryEstimateV)*0.2f;
}

static bool startI2S(i2s_port_t port,bool input) {
  i2s_config_t cfg{};
  cfg.mode=i2s_mode_t(I2S_MODE_MASTER|(input?I2S_MODE_RX:I2S_MODE_TX));
  cfg.sample_rate=16000; cfg.bits_per_sample=I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format=input?I2S_CHANNEL_FMT_ALL_LEFT:I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format=I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags=ESP_INTR_FLAG_LEVEL1; cfg.dma_buf_count=4; cfg.dma_buf_len=128;
  cfg.tx_desc_auto_clear=true;
  i2s_pin_config_t pins{};
  pins.bck_io_num=input?BOARD_ES7210_SCK:BOARD_I2S_BCK;
  pins.ws_io_num=input?BOARD_ES7210_LRCK:BOARD_I2S_WS;
  pins.data_in_num=input?BOARD_ES7210_DIN:-1;
  pins.data_out_num=input?-1:BOARD_I2S_DOUT;
  pins.mck_io_num=input?BOARD_ES7210_MCLK:-1;
  if(i2s_driver_install(port,&cfg,0,nullptr)!=ESP_OK) return false;
  if(i2s_set_pin(port,&pins)==ESP_OK) return true;
  i2s_driver_uninstall(port); return false;
}
static void startAudio() {
  speakerOK=startI2S(I2S_NUM_0,false);
  if(!responds(ES7210_ADDR)) return;
  audio_hal_codec_config_t cfg{};
  cfg.adc_input=AUDIO_HAL_ADC_INPUT_ALL; cfg.codec_mode=AUDIO_HAL_CODEC_MODE_ENCODE;
  cfg.i2s_iface.mode=AUDIO_HAL_MODE_SLAVE; cfg.i2s_iface.fmt=AUDIO_HAL_I2S_NORMAL;
  cfg.i2s_iface.samples=AUDIO_HAL_16K_SAMPLES; cfg.i2s_iface.bits=AUDIO_HAL_BIT_LENGTH_16BITS;
  micOK=es7210_adc_init(&Wire,&cfg)==ESP_OK &&
        es7210_adc_config_i2s(cfg.codec_mode,&cfg.i2s_iface)==ESP_OK &&
        es7210_adc_set_gain((es7210_input_mics_t)(ES7210_INPUT_MIC1|ES7210_INPUT_MIC2),GAIN_6DB)==ESP_OK &&
        es7210_adc_ctrl_state(cfg.codec_mode,AUDIO_HAL_CTRL_START)==ESP_OK && startI2S(I2S_NUM_1,true);
}
static void beep() {
  if(!speakerOK) return;
  int16_t wave[160];
  for(int i=0;i<160;i++) wave[i]=int16_t(1700*sin(i*PI/10));
  size_t n;
  for(int i=0;i<12;i++) i2s_write(I2S_NUM_0,wave,sizeof(wave),&n,pdMS_TO_TICKS(30));
}
static void sampleInputs() {
  if(micOK) {
    int16_t buf[128]; size_t n=0;
    if(i2s_read(I2S_NUM_1,buf,sizeof(buf),&n,0)==ESP_OK && n) {
      micPeak=0;
      for(size_t i=0;i<n/2;i++) micPeak=max(micPeak,abs(int(buf[i])));
    }
  }
  if(radioOK && digitalRead(RADIO_DIO1_PIN)) {
    uint8_t data[255];
    if(radio.readData(data,0)==RADIOLIB_ERR_NONE) ++radioPackets;
    radio.startReceive();
  }
}
static void backlight(int target) {
  digitalWrite(BOARD_BL_PIN,LOW); delay(3);
  digitalWrite(BOARD_BL_PIN,HIGH); delayMicroseconds(30);
  for(int i=target;i<16;i++){ digitalWrite(BOARD_BL_PIN,LOW); digitalWrite(BOARD_BL_PIN,HIGH); }
}
static void mountCard(){
  digitalWrite(BOARD_TFT_CS,HIGH); digitalWrite(RADIO_CS_PIN,HIGH); digitalWrite(BOARD_SDCARD_CS,HIGH);
  bool ready=card.cardBegin(SdSpiConfig(BOARD_SDCARD_CS,SHARED_SPI,800000,&SPI));
  cardMiB=ready?card.card()->sectorCount()/2048:0;
  sdOK=ready && card.volumeBegin();
  volumeMiB=sdOK?(uint64_t(card.clusterCount())*card.bytesPerCluster())/(1024*1024):0;
}
static void loadMap(bool retry=false);
static void startBoard(int light) {
  pinMode(BOARD_POWERON,OUTPUT); digitalWrite(BOARD_POWERON,HIGH); delay(100);
  for(int pin:{BOARD_SDCARD_CS,BOARD_TFT_CS,RADIO_CS_PIN}) { pinMode(pin,OUTPUT); digitalWrite(pin,HIGH); }
  for(int pin:ballPins) pinMode(pin,INPUT_PULLUP);
  pinMode(BOARD_SPI_MISO,INPUT_PULLUP); SPI.begin(BOARD_SPI_SCK,BOARD_SPI_MISO,BOARD_SPI_MOSI);
  // Switch the card into SPI mode before the display starts sending commands.
  mountCard();
  loadMap();
  panel.begin(); panel.setRotation(1); panel.fillScreen(TFT_BLACK);
  pinMode(BOARD_BL_PIN,OUTPUT); backlight(light);
  panel.setTextColor(TFT_WHITE); panel.drawString("FlightDeck indul...",12,12,2);
  screen.setColorDepth(16); screen.createSprite(320,240);
  Wire.begin(BOARD_I2C_SDA,BOARD_I2C_SCL); Wire.setTimeOut(25);
  detectSensors(); kbOK=responds(0x55);
  touch.setPins(-1,BOARD_TOUCH_INT); touchOK=touch.begin(Wire);
  if(touchOK) { touch.setMaxCoordinates(320,240); touch.setSwapXY(true); touch.setMirrorXY(false,true); }
  radioOK=radio.begin(868.0)==RADIOLIB_ERR_NONE && radio.startReceive()==RADIOLIB_ERR_NONE;
  startAudio(); sampleEnvironment();
}
