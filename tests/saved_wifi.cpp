#include "../FlightDeck/saved_wifi.h"
#include <cassert>
#include <cstdio>

int main(){
  WifiBook book;
  assert(book.valid() && book.count==0);
  assert(book.put("office","") && book.put("home","test-password"));
  assert(book.put("home","new-password") && book.count==2 && book.preferred==1);
  assert(strcmp(book.entries[1].password,"new-password")==0);
  assert(!book.put("","invalid"));
  for(int i=2;i<WifiBook::capacity;i++){
    char name[10];snprintf(name,sizeof(name),"test%d",i);assert(book.put(name,""));
  }
  assert(!book.put("extra","") && book.valid());
  book.erase(1);
  assert(book.find("home")==-1 && book.count==7 && book.preferred==6 && book.valid());
  while(book.count)book.erase(book.preferred);
  assert(book.valid() && !book.entries[0].ssid[0] && !book.entries[0].password[0]);
  book.count=255;assert(!book.valid());
}
