#include "../FlightDeck/traffic_budget.h"
#include <cassert>

int main(){
  constexpr unsigned midnight=1795651200; // 2026-11-26 00:00 UTC
  static_assert(midnight%86400==0,"UTC day boundary");
  TrafficBudget budget{};
  assert(!budget.reserve(0,120));
  for(unsigned second=0;second<16*3600;++second)
    assert(budget.reserve(midnight+second,120)==(second%120==0));
  assert(budget.used==480);
  for(unsigned i=480;i<500;++i)assert(budget.reserve(midnight+120*i,120));
  assert(!budget.reserve(midnight+86399,120));
  assert(budget.reserve(midnight+86400,120) && budget.used==1);
  assert(!budget.reserve(midnight,120));
  TrafficBudget rebooted=budget;
  assert(!rebooted.reserve(budget.last+119,120));
  assert(rebooted.reserve(budget.last+120,120));

  assert(trafficRetryTime("3600",midnight)==midnight+3600);
  assert(trafficRetryTime("Thu, 26 Nov 2026 01:00:00 GMT",midnight)==midnight+3600);
  assert(!trafficRetryTime("Fri, 31 Feb 2026 00:00:00 GMT",midnight));
  assert(!trafficRetryTime("10 minutes",midnight));
  assert(!trafficRetryTime("",midnight));
  budget.limited(budget.last,budget.last+7200);
  assert(!budget.reserve(budget.last+7199,120));
  assert(budget.reserve(budget.last+7200,120));
  TrafficBudget fallback{};
  fallback.limited(midnight,0);assert(fallback.retryAt==midnight+120);
  fallback.limited(midnight+120,0);assert(fallback.retryAt==midnight+360);
  TrafficBudget late{};
  assert(late.reserve(midnight+86390,180));
  assert(!late.reserve(midnight+86400,180));
  assert(late.reserve(midnight+86570,180) && late.used==1);
}
