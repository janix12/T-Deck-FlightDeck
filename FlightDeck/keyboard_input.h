#pragma once
#include <stdint.h>
#include <string.h>

// Column/row order of LilyGO's Keyboard_ESP32C3 five-byte matrix.
static const char keyLetters[5][8]={"qw\0a\0 \0","esdpxz\0","rgt\0vcf","uhy\rbnj","oli\b$mk"};
static const char keySymbols[5][8]={"#1\0*\0 0","245@87\0","3/(\0?96","_:)\r!,;","+\x22-\b\0.'"};

static char lockedSymbol(char ch){
  // Older keyboards send no event for the microphone key alone. Space gives 0.
  if(ch==' ')return '0';
  if(ch<'a' || ch>'z')return ch;
  for(int col=0;col<5;col++)
    for(int row=0;row<7;row++)
      if(keyLetters[col][row]==ch)return keySymbols[col][row];
  return ch;
}

template<typename Emit>
static void decodeMatrix(const uint8_t matrix[5],uint8_t previous[5],Emit emit){
  const bool symbol=matrix[0]&4,shift=(matrix[1]&64)||(matrix[2]&8),alt=matrix[0]&16;
  for(int col=0;col<5;col++){
    const uint8_t pressed=matrix[col]&~previous[col];
    previous[col]=matrix[col];
    for(int row=0;row<7;row++){
      if(!(pressed&(1<<row)))continue;
      char ch=(symbol?keySymbols:keyLetters)[col][row];
      if(alt && col==3 && row==4)continue; // ALT+B is handled by the controller.
      if(alt && col==2 && row==5)ch=12;
      if(shift && ch>='a' && ch<='z')ch-=32;
      if(ch)emit(ch);
    }
  }
}
