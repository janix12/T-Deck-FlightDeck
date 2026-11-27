#pragma once
#include <atomic>
#include <esp_freertos_hooks.h>
#include <esp_heap_caps.h>

struct CoreLoad {
  TaskHandle_t idle=nullptr;
  unsigned ticks=0,busy=0;
  std::atomic<int> percent{-1};
};
static CoreLoad coreLoad[portNUM_PROCESSORS];

// Each tick hook owns its core's counters. Only the completed sample crosses cores.
static void IRAM_ATTR sampleCoreLoad(){
  CoreLoad &core=coreLoad[xPortGetCoreID()];
  if(xTaskGetCurrentTaskHandle()!=core.idle)++core.busy;
  if(++core.ticks==configTICK_RATE_HZ){
    core.percent.store((100*core.busy+core.ticks/2)/core.ticks,std::memory_order_relaxed);
    core.ticks=core.busy=0;
  }
}
static void startCoreSampling(){
  for(unsigned i=0;i<portNUM_PROCESSORS;i++){
    coreLoad[i].idle=xTaskGetIdleTaskHandleForCPU(i);
    if(coreLoad[i].idle && esp_register_freertos_tick_hook_for_cpu(sampleCoreLoad,i)!=ESP_OK)
      Serial.printf("CPU%u sampling unavailable\n",i);
  }
}
struct HeapUsage {
  size_t used,total;
  int percent() const {return total?int((used*100ULL+total/2)/total):-1;}
};
static HeapUsage sampleHeap(uint32_t region){
  multi_heap_info_t info{};
  heap_caps_get_info(&info,region|MALLOC_CAP_8BIT);
  return {info.total_allocated_bytes,info.total_allocated_bytes+info.total_free_bytes};
}
