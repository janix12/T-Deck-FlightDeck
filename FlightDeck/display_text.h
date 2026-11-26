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
static void label(int x,int y,const String &s,uint16_t color=ink,int font=2){
  const uint8_t *p=(const uint8_t*)s.c_str();while(*p && *p<128)++p;
  if(!*p){screen.setTextColor(color);screen.drawString(s,x,y,font);return;}
  drawLabel(x,y,s,color,font,true);
}
static int labelWidth(const String &s,int font){return drawLabel(0,0,s,ink,font,false);}
