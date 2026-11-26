#pragma once

// TFT_eSPI's built-in bitmap fonts have ASCII glyphs. Draw the Hungarian
// accents on their base letters at the same advance as the underlying font.
struct AccentGlyph {uint16_t code;char base;uint8_t mark;};
static const AccentGlyph accentGlyphs[]={
 {0xc381,'A',1},{0xc389,'E',1},{0xc38d,'I',1},{0xc393,'O',1},{0xc396,'O',2},{0xc590,'O',3},{0xc39a,'U',1},{0xc39c,'U',2},{0xc5b0,'U',3},
 {0xc3a1,'a',1},{0xc3a9,'e',1},{0xc3ad,'i',1},{0xc3b3,'o',1},{0xc3b6,'o',2},{0xc591,'o',3},{0xc3ba,'u',1},{0xc3bc,'u',2},{0xc5b1,'u',3}
};
static int drawLabel(int x,int y,const String &s,uint16_t color,int font,bool paint){
  const uint8_t *p=(const uint8_t*)s.c_str();int start=x;
  if(paint)screen.setTextColor(color);
  while(*p){
    const uint8_t *first=p;while(*p && *p<128)++p;
    if(p>first){
      String part;part.concat((const char*)first,p-first);
      x+=paint?screen.drawString(part,x,y,font):screen.textWidth(part,font);
    }
    if(!*p)break;
    uint16_t code=*p++<<8;if(*p)code|=*p++;
    const AccentGlyph *glyph=nullptr;
    for(const auto &g:accentGlyphs)if(g.code==code){glyph=&g;break;}
    char letter[2]={glyph?glyph->base:'?',0};int width=screen.textWidth(letter,font);
    if(paint){
      screen.drawString(letter,x,y,font);
      if(glyph){
        bool cap=letter[0]<'a';int cx=x+width/2,top=y+(font==4?(cap?0:4):font==2?(cap?0:2):-1);
        if(glyph->mark==2){screen.drawPixel(cx-2,top,color);screen.drawPixel(cx+1,top,color);}
        else {
          int a=cx-(glyph->mark==3?2:0);screen.drawLine(a,top+1,a+1,top,color);
          if(glyph->mark==3)screen.drawLine(cx+1,top+1,cx+2,top,color);
        }
      }
    }
    x+=width;
  }
  return x-start;
}
static void mapLabel(int x,int y,const String &s,uint16_t color=ink,int font=2){
  const uint8_t *p=(const uint8_t*)s.c_str();while(*p && *p<128)++p;
  if(!*p){screen.setTextColor(color);screen.drawString(s,x,y,font);return;}
  drawLabel(x,y,s,color,font,true);
}
#include "ui_font.h"

static const UiFace &uiFace(int font){
  switch(font){case 1:return face11;case 4:return face24;case 6:return face46;default:return face15;}
}
static unsigned nextGlyph(const uint8_t *&p){
  unsigned c=*p++;
  if(c>=32 && c<127)return c-32;
  if(c>=0xc2 && c<=0xdf && (*p&0xc0)==0x80){
    c=((c&31)<<6)|(*p++&63);
    for(unsigned i=0;i<sizeof(uiAccents)/sizeof(*uiAccents);++i)
      if(uiAccents[i]==c)return 95+i;
  }else while((*p&0xc0)==0x80)++p;
  return '?'-32;
}
struct LabelMetrics { int width=0,top=0,bottom=0; };
static LabelMetrics measureLabel(const String &text,int font){
  const UiFace &face=uiFace(font);
  LabelMetrics m;
  const uint8_t *p=(const uint8_t*)text.c_str();
  while(*p){
    const UiGlyph &g=face.glyphs[nextGlyph(p)];
    m.width+=g.advance;
    if(g.height){m.top=min(m.top,int(g.top));m.bottom=max(m.bottom,g.top+g.height);}
  }
  return m;
}
static int labelWidth(const String &text,int font){return measureLabel(text,font).width;}
static int labelPrefixBytes(const String &text,int font,int available){
  const UiFace &face=uiFace(font);
  const uint8_t *start=(const uint8_t*)text.c_str(),*p=start;
  while(*p){
    const uint8_t *previous=p;
    available-=face.glyphs[nextGlyph(p)].advance;
    if(available<0)return previous-start;
  }
  return p-start;
}
static void drawGlyph(int left,int top,const UiFace &face,const UiGlyph &g,uint16_t color){
  unsigned i=0;
  for(int row=0;row<g.height;++row)for(int col=0;col<g.width;++col,++i){
    const uint8_t coverage=(face.pixels[g.offset+i/4]>>(6-2*(i%4)))&3;
    if(!coverage)continue;
    const int px=left+col,py=top+row;
    screen.drawPixel(px,py,coverage==3?color:screen.alphaBlend(coverage*85,color,screen.readPixel(px,py)));
  }
}
static void label(int x,int y,const String &text,uint16_t color=ink,int font=2){
  const UiFace &face=uiFace(font);
  const uint8_t *p=(const uint8_t*)text.c_str();
  while(*p){
    const UiGlyph &g=face.glyphs[nextGlyph(p)];
    drawGlyph(x+g.left,y+face.baseline+g.top,face,g,color);
    x+=g.advance;
  }
}
static void centeredLabel(int x,int y,int w,int h,const String &text,uint16_t color,int font=2){
  const LabelMetrics m=measureLabel(text,font);
  label(x+(w-m.width)/2,y+(h-m.bottom+m.top)/2-m.top-uiFace(font).baseline,text,color,font);
}
