#include <assert.h>
#include "task_stats.h"
#include "cycle_metrics.h"
#include "app_config.h"
int main(void) {
 Task_t t={0}; CycleMetrics_t c={0};
 Task_ResetStats(&t); t.run_count=1; Task_UpdateExecStats(&t,100); Task_UpdateResponseStats(&t,200);
 t.run_count=2; Task_UpdateExecStats(&t,50); Task_UpdateResponseStats(&t,300);
 Task_CheckDeadline(&t,1); CycleMetrics_Update(&c,10); CycleMetrics_Update(&c,20);
 assert(t.total_exec_us==150 && t.total_response_us==500 && t.max_response_us==300);
 assert(t.deadline_miss_count==1 && c.total_cycles==30 && c.count==2);
#if ENABLE_EXTENDED_STATS
 assert(t.min_exec_us==50 && t.max_exec_us==100 && t.exec_hist[0]==2 && t.min_response_us==200 && c.max_cycles==20);
#else
 assert(t.min_exec_us==0 && t.max_exec_us==0 && t.exec_hist[0]==0 && t.min_response_us==0 && c.max_cycles==0);
#endif
 return 0;
}
