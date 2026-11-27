#include "../FlightDeck/keyboard_input.h"
#include <cassert>
#include <string>

int main(){
  uint8_t previous[5]{},matrix[5]{};
  std::string typed;
  bool lockedMode=false;
  const bool *editor=nullptr;
  auto scan=[&]{decodeMatrix(matrix,previous,editor,[&](int ch){
    if(ch==toggleSymbolKey)lockedMode=!lockedMode;
    else typed+=char(ch);
  });};
  auto tap=[&](int col,int row){
    matrix[col]|=1<<row;scan();scan();
    matrix[col]&=~(1<<row);scan();
  };
  matrix[0]=4;scan(); // SYM held through all four presses
  tap(0,1);tap(1,0);tap(2,0);tap(0,6);
  matrix[0]=0;scan();tap(0,1);
  matrix[1]=64;scan();tap(1,0);
  matrix[1]=0;scan();tap(3,3);tap(4,3);
  matrix[0]=16;scan();tap(3,4); // The controller owns the light shortcut.
  assert(typed=="1230wE\r\b");
  std::string locked;
  for(char ch:std::string("wersdfzxc "))locked+=lockedSymbol(ch);
  assert(locked=="1234567890");
  assert(lockedSymbol('p')=='@' && lockedSymbol('9')=='9' && lockedSymbol('\b')=='\b');
  assert(editorCharacter('#',false)==toggleSymbolKey && editorCharacter('#',true)==toggleSymbolKey);
  assert(editorCharacter('q',true)=='#' && editorCharacter(' ',true)=='0');
  assert(editorCharacter('0',false)=='0' && editorCharacter('0',true)=='0');
  editor=&lockedMode;typed.clear();
  matrix[0]=6;scan();scan(); // Physical SYM and W pressed in the same scan.
  matrix[0]=0;scan();
  tap(1,0);tap(0,6);tap(0,5);tap(0,0); // After release: E, microphone, space, Q.
  tap(0,2);tap(0,1);
  assert(!lockedMode && typed=="1200#w");
}
