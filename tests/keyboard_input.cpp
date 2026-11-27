#include "../FlightDeck/keyboard_input.h"
#include <cassert>
#include <string>

int main(){
  uint8_t previous[5]{},matrix[5]{};
  std::string typed;
  auto scan=[&]{decodeMatrix(matrix,previous,[&](char ch){typed+=ch;});};
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
}
