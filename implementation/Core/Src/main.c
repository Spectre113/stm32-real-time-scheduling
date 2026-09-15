/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "string.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <inttypes.h>
#include "app_config.h"
#include "workload_config.h"
#include "app_uart.h"
#include "cycle_metrics.h"
#include "dht11.h"
#include "edf_selector.h"
#include "edf_executor.h"
#include "debug_status.h"
#include "hcsr04.h"
#include "isolated_profile.h"
#include "minimal_superloop_config.h"
#include "minimal_superloop_profile.h"
#include "platform_time.h"
#include "profile_samples.h"
#include "scheduler_release.h"
#include "superloop_scalability_profile.h"
#include "superloop_checks_profile.h"
#include "task_stats.h"
#include "task_init.h"
#include "task_reporting.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma location=0x2007c000
ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
#pragma location=0x2007c0a0
ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */

__attribute__((at(0x2007c000))) ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT]; /* Ethernet Rx DMA Descriptors */
__attribute__((at(0x2007c0a0))) ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT]; /* Ethernet Tx DMA Descriptors */

#elif defined ( __GNUC__ ) /* GNU Compiler */

ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __attribute__((section(".RxDecripSection"))); /* Ethernet Rx DMA Descriptors */
ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __attribute__((section(".TxDecripSection")));   /* Ethernet Tx DMA Descriptors */
#endif

ETH_TxPacketConfig TxConfig;

ETH_HandleTypeDef heth;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart3;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

/* USER CODE BEGIN PV */

static Task_t tau1;
static Task_t tau2;

#if ENABLE_SYNTH_IMU
static Task_t tau_imu;
#endif

#if ENABLE_SYNTH_LIDAR
static Task_t tau_lidar;
#endif

#if ENABLE_SYNTH_CAMERA
static Task_t tau_camera;
#endif

#if ENABLE_SYNTH_CONTROL
static Task_t tau_control;
#endif

static int g_distance_cm = -1;

static uint8_t g_temp = 0;
static uint8_t g_hum = 0;
static int g_dht_res = -99;

static uint64_t g_profile_start_us = 0;
static uint32_t g_profile_start_ms = 0;

static CycleMetrics_t g_scheduler_metrics;
static CycleMetrics_t g_polling_metrics;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
static void MX_GPIO_Init(void);
static void MX_ETH_Init(void);
static void MX_USART3_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_TIM3_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

#if (SCHED_ALGO == SCHED_ALGO_CHUNKED_EDF) && ENABLE_REAL_TAU1
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  HCSR04_Async_OnExti(GPIO_Pin);
}
#endif

static void Tau1_Run(void)
{
  g_distance_cm = HCSR04_Read_cm_Blocking();
  tau1.run_count++;
}

static void Tau2_Run(void)
{
  g_dht_res = DHT11_Read(&g_temp, &g_hum);
  tau2.run_count++;
}

#if ENABLE_SYNTH_IMU
static void Tau_IMU_Run(void)
{
  Synthetic_Workload_us(TAU_IMU_WORKLOAD_US);
  tau_imu.run_count++;
}
#endif

#if ENABLE_SYNTH_LIDAR
static void Tau_LiDAR_Run(void)
{
  Synthetic_Workload_us(TAU_LIDAR_WORKLOAD_US);
  tau_lidar.run_count++;
}
#endif

#if ENABLE_SYNTH_CAMERA
static void Tau_Camera_Run(void)
{
  Synthetic_Workload_us(TAU_CAMERA_WORKLOAD_US);
  tau_camera.run_count++;
}
#endif

#if ENABLE_SYNTH_CONTROL
static void Tau_Control_Run(void)
{
  Synthetic_Workload_us(TAU_CONTROL_WORKLOAD_US);
  tau_control.run_count++;
}
#endif

#if SCHED_ALGO == SCHED_ALGO_CHUNKED_EDF
#if !ENABLE_SYNTH_CONTROL && !ENABLE_REAL_TAU1 && !ENABLE_SYNTH_LIDAR && \
    !ENABLE_SYNTH_IMU && !ENABLE_SYNTH_CAMERA && !ENABLE_REAL_TAU2
#error "Chunked EDF requires at least one enabled task"
#endif

static SchedTaskRef_t g_sched_tasks[] =
{
  #if ENABLE_SYNTH_CONTROL
    { &tau_control, Tau_Control_Run, TAU_CONTROL_WORKLOAD_US, 1U,
      SCHED_TASK_SYNTHETIC },
  #endif

  #if ENABLE_REAL_TAU1
    { &tau1, Tau1_Run, 0ULL, 1U, SCHED_TASK_STAGED_HCSR04 },
  #endif

  #if ENABLE_SYNTH_LIDAR
    { &tau_lidar, Tau_LiDAR_Run, TAU_LIDAR_WORKLOAD_US, 1U,
      SCHED_TASK_SYNTHETIC },
  #endif

  #if ENABLE_SYNTH_IMU
    { &tau_imu, Tau_IMU_Run, TAU_IMU_WORKLOAD_US, 1U,
      SCHED_TASK_SYNTHETIC },
  #endif

  #if ENABLE_SYNTH_CAMERA
    { &tau_camera, Tau_Camera_Run, TAU_CAMERA_WORKLOAD_US, 1U,
      SCHED_TASK_SYNTHETIC },
  #endif

  #if ENABLE_REAL_TAU2
    { &tau2, Tau2_Run, 0ULL, 1U, SCHED_TASK_STAGED_DHT11 },
  #endif
};

static const uint32_t g_sched_task_count =
    sizeof(g_sched_tasks) / sizeof(g_sched_tasks[0]);
#endif

static void Task_SaveExecSample(Task_t *task, uint64_t exec_time_us)
{
  if (task == &tau1)
  {
    ProfileSamples_SaveTau1(exec_time_us);
  }
  else if (task == &tau2)
  {
    ProfileSamples_SaveTau2(exec_time_us);
  }
}

static void Reset_Profiling_Stats(void)
{
  Task_ResetStats(&tau1);
  Task_ResetStats(&tau2);

  #if (SCHED_ALGO == SCHED_ALGO_CHUNKED_EDF) && ENABLE_REAL_TAU1
    HCSR04_Async_Reset();
  #endif

  #if (SCHED_ALGO == SCHED_ALGO_CHUNKED_EDF) && ENABLE_REAL_TAU2
    DHT11_Async_Reset();
  #endif

	#if ENABLE_SYNTH_IMU
	  Task_ResetStats(&tau_imu);
	#endif

	#if ENABLE_SYNTH_LIDAR
	  Task_ResetStats(&tau_lidar);
	#endif

	#if ENABLE_SYNTH_CAMERA
	  Task_ResetStats(&tau_camera);
	#endif

	#if ENABLE_SYNTH_CONTROL
	  Task_ResetStats(&tau_control);
	#endif

  CycleMetrics_Reset(&g_scheduler_metrics);
  CycleMetrics_Reset(&g_polling_metrics);

  ProfileSamples_Reset();
}

/* App-level adapter: binds module statistics to this firmware's task set and UART. */
static void Print_Profiling_Summary(void)
{
  char msg[384];

  uint64_t tau1_avg_exec = 0;
  uint64_t tau2_avg_exec = 0;

  uint64_t tau1_avg_response = 0;
  uint64_t tau2_avg_response = 0;

  #if ENABLE_SYNTH_IMU
    uint64_t tau_imu_avg_exec = 0;
    uint64_t tau_imu_avg_response = 0;
  #endif

	#if ENABLE_SYNTH_LIDAR
	  uint64_t tau_lidar_avg_exec = 0;
	  uint64_t tau_lidar_avg_response = 0;
	#endif

	#if ENABLE_SYNTH_CAMERA
	  uint64_t tau_camera_avg_exec = 0;
	  uint64_t tau_camera_avg_response = 0;
	#endif

	#if ENABLE_SYNTH_CONTROL
	  uint64_t tau_control_avg_exec = 0;
	  uint64_t tau_control_avg_response = 0;
	#endif

  uint32_t cycles_per_us = PlatformTime_CyclesPerUs();

  uint64_t sched_avg_cycles = 0;
  uint64_t sched_total_us = 0;
  uint64_t sched_overhead_x10000 = 0;

  uint64_t poll_avg_cycles = 0;
  uint64_t poll_total_us = 0;
  uint64_t poll_overhead_x10000 = 0;

  uint64_t task_exec_total_us = 0;
  uint64_t task_exec_percent_x10000 = 0;

  uint64_t logical_idle_us = 0;
  uint64_t logical_idle_percent_x10000 = 0;

  uint64_t measured_busy_us = 0;
  uint64_t measured_busy_percent_x10000 = 0;

  #if ENABLE_REAL_TAU1
    if (tau1.run_count > 0)
    {
      tau1_avg_exec = tau1.total_exec_us / tau1.run_count;
      tau1_avg_response = tau1.total_response_us / tau1.run_count;
    }
  #endif

	#if ENABLE_REAL_TAU2
		if (tau2.run_count > 0)
		{
			tau2_avg_exec = tau2.total_exec_us / tau2.run_count;
			tau2_avg_response = tau2.total_response_us / tau2.run_count;
		}
	#endif

  #if ENABLE_SYNTH_IMU
    if (tau_imu.run_count > 0)
    {
      tau_imu_avg_exec = tau_imu.total_exec_us / tau_imu.run_count;
      tau_imu_avg_response = tau_imu.total_response_us / tau_imu.run_count;
    }
  #endif

	#if ENABLE_SYNTH_LIDAR
	  if (tau_lidar.run_count > 0)
	  {
		tau_lidar_avg_exec = tau_lidar.total_exec_us / tau_lidar.run_count;
		tau_lidar_avg_response = tau_lidar.total_response_us / tau_lidar.run_count;
	  }
	#endif

	#if ENABLE_SYNTH_CAMERA
	  if (tau_camera.run_count > 0)
	  {
		tau_camera_avg_exec = tau_camera.total_exec_us / tau_camera.run_count;
		tau_camera_avg_response = tau_camera.total_response_us / tau_camera.run_count;
	  }
	#endif

  if (g_scheduler_metrics.count > 0U)
  {
    sched_avg_cycles = g_scheduler_metrics.total_cycles / g_scheduler_metrics.count;
    sched_total_us = g_scheduler_metrics.total_cycles / cycles_per_us;
    sched_overhead_x10000 = (sched_total_us * 1000000ULL) / PROFILE_WINDOW_US;
  }

  if (g_polling_metrics.count > 0U)
  {
    poll_avg_cycles = g_polling_metrics.total_cycles / g_polling_metrics.count;
    poll_total_us = g_polling_metrics.total_cycles / cycles_per_us;
    poll_overhead_x10000 = (poll_total_us * 1000000ULL) / PROFILE_WINDOW_US;
  }

  task_exec_total_us = 0;

	#if ENABLE_REAL_TAU1
	  task_exec_total_us += tau1.total_exec_us;
	#endif

	#if ENABLE_REAL_TAU2
	  task_exec_total_us += tau2.total_exec_us;
	#endif

  #if ENABLE_SYNTH_IMU
    task_exec_total_us += tau_imu.total_exec_us;
  #endif

  #if ENABLE_SYNTH_LIDAR
    task_exec_total_us += tau_lidar.total_exec_us;
  #endif

	#if ENABLE_SYNTH_CAMERA
		task_exec_total_us += tau_camera.total_exec_us;
	#endif

	#if ENABLE_SYNTH_CONTROL
	  task_exec_total_us += tau_control.total_exec_us;
	#endif

  task_exec_percent_x10000 = (task_exec_total_us * 1000000ULL) / PROFILE_WINDOW_US;

  /*
   * measured_busy_us includes:
   * - actual task execution time
   * - continuous polling/check overhead, if polling measurement is enabled
   *
   * This is still not full physical CPU busy time, because small profiling/statistics
   * updates and loop overhead outside measured blocks are not fully included.
   */
  measured_busy_us = task_exec_total_us + poll_total_us;

  if (measured_busy_us < PROFILE_WINDOW_US)
  {
    logical_idle_us = PROFILE_WINDOW_US - measured_busy_us;
  }
  else
  {
    logical_idle_us = 0;
  }

  logical_idle_percent_x10000 = (logical_idle_us * 1000000ULL) / PROFILE_WINDOW_US;
  measured_busy_percent_x10000 = (measured_busy_us * 1000000ULL) / PROFILE_WINDOW_US;

  snprintf(msg, sizeof(msg),
           "\r\n=== PROFILING SUMMARY %lu s ===\r\n",
           (unsigned long)(PROFILE_WINDOW_US / 1000000ULL));
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "scheduler algorithm: %s\r\n",
           SCHED_ALGO_NAME);
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "workload scenario: %s\r\n",
           WORKLOAD_SCENARIO_NAME);
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "theoretical utilization: %lu %%\r\n",
           (unsigned long)WORKLOAD_UTILIZATION_PERCENT);
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "theoretical utilization exact: %lu.%02lu %%\r\n",
           (unsigned long)(UTIL_X10000 / 100ULL),
           (unsigned long)(UTIL_X10000 % 100ULL));
  uart_print(msg);

  #if SCHED_ALGO == SCHED_ALGO_CHUNKED_EDF
    snprintf(msg, sizeof(msg),
             "edf chunk size: %lu us\r\n",
             (unsigned long)EDF_CHUNK_US);
    uart_print(msg);
  #endif

	#if ENABLE_REAL_TAU1
	  snprintf(msg, sizeof(msg),
			   "tau1 HC-SR04 | runs=%lu\r\n",
			   (unsigned long)tau1.run_count);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  exec_us     avg=%lu min=%lu max=%lu\r\n",
			   (unsigned long)tau1_avg_exec,
			   (unsigned long)tau1.min_exec_us,
			   (unsigned long)tau1.max_exec_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  response_us avg=%lu min=%lu max=%lu\r\n",
			   (unsigned long)tau1_avg_response,
			   (unsigned long)tau1.min_response_us,
			   (unsigned long)tau1.max_response_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  deadline   D=%lu ms | misses=%lu | max_lateness_us=%lu\r\n",
			   (unsigned long)tau1.deadline_ms,
			   (unsigned long)tau1.deadline_miss_count,
			   (unsigned long)tau1.max_lateness_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  skipped releases=%lu | total timing failures=%lu\r\n",
			   (unsigned long)tau1.skipped_release_count,
			   (unsigned long)tau1.total_timing_failures);
	  uart_print(msg);

	  TaskReporting_PrintExecHistogram("  exec distribution:", &tau1);
	#endif

	#if ENABLE_REAL_TAU2
	  snprintf(msg, sizeof(msg),
			   "tau2 DHT11   | runs=%lu\r\n",
			   (unsigned long)tau2.run_count);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  exec_us     avg=%lu min=%lu max=%lu\r\n",
			   (unsigned long)tau2_avg_exec,
			   (unsigned long)tau2.min_exec_us,
			   (unsigned long)tau2.max_exec_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  response_us avg=%lu min=%lu max=%lu\r\n",
			   (unsigned long)tau2_avg_response,
			   (unsigned long)tau2.min_response_us,
			   (unsigned long)tau2.max_response_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  deadline   D=%lu ms | misses=%lu | max_lateness_us=%lu\r\n",
			   (unsigned long)tau2.deadline_ms,
			   (unsigned long)tau2.deadline_miss_count,
			   (unsigned long)tau2.max_lateness_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  skipped releases=%lu | total timing failures=%lu\r\n",
			   (unsigned long)tau2.skipped_release_count,
			   (unsigned long)tau2.total_timing_failures);
	  uart_print(msg);

	  TaskReporting_PrintExecHistogram("  exec distribution:", &tau2);
	#endif

  #if ENABLE_SYNTH_IMU
    snprintf(msg, sizeof(msg),
             "tau_IMU synthetic | runs=%lu\r\n",
             (unsigned long)tau_imu.run_count);
    uart_print(msg);

    snprintf(msg, sizeof(msg),
             "  period     T=%lu ms | target C=%lu us | deadline D=%lu ms\r\n",
             (unsigned long)tau_imu.period_ms,
             (unsigned long)TAU_IMU_WORKLOAD_US,
             (unsigned long)tau_imu.deadline_ms);
    uart_print(msg);

    snprintf(msg, sizeof(msg),
             "  exec_us     avg=%lu min=%lu max=%lu\r\n",
             (unsigned long)tau_imu_avg_exec,
             (unsigned long)tau_imu.min_exec_us,
             (unsigned long)tau_imu.max_exec_us);
    uart_print(msg);

    snprintf(msg, sizeof(msg),
             "  response_us avg=%lu min=%lu max=%lu\r\n",
             (unsigned long)tau_imu_avg_response,
             (unsigned long)tau_imu.min_response_us,
             (unsigned long)tau_imu.max_response_us);
    uart_print(msg);

    snprintf(msg, sizeof(msg),
             "  deadline   D=%lu ms | misses=%lu | max_lateness_us=%lu\r\n",
             (unsigned long)tau_imu.deadline_ms,
             (unsigned long)tau_imu.deadline_miss_count,
             (unsigned long)tau_imu.max_lateness_us);
    uart_print(msg);

    snprintf(msg, sizeof(msg),
             "  skipped releases=%lu | total timing failures=%lu\r\n",
             (unsigned long)tau_imu.skipped_release_count,
             (unsigned long)tau_imu.total_timing_failures);
    uart_print(msg);

    TaskReporting_PrintExecHistogram("  exec distribution:", &tau_imu);
  #endif

	#if ENABLE_SYNTH_LIDAR
	  snprintf(msg, sizeof(msg),
			   "tau_LiDAR synthetic | runs=%lu\r\n",
			   (unsigned long)tau_lidar.run_count);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  period     T=%lu ms | target C=%lu us | deadline D=%lu ms\r\n",
			   (unsigned long)tau_lidar.period_ms,
			   (unsigned long)TAU_LIDAR_WORKLOAD_US,
			   (unsigned long)tau_lidar.deadline_ms);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  exec_us     avg=%lu min=%lu max=%lu\r\n",
			   (unsigned long)tau_lidar_avg_exec,
			   (unsigned long)tau_lidar.min_exec_us,
			   (unsigned long)tau_lidar.max_exec_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  response_us avg=%lu min=%lu max=%lu\r\n",
			   (unsigned long)tau_lidar_avg_response,
			   (unsigned long)tau_lidar.min_response_us,
			   (unsigned long)tau_lidar.max_response_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  deadline   D=%lu ms | misses=%lu | max_lateness_us=%lu\r\n",
			   (unsigned long)tau_lidar.deadline_ms,
			   (unsigned long)tau_lidar.deadline_miss_count,
			   (unsigned long)tau_lidar.max_lateness_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  skipped releases=%lu | total timing failures=%lu\r\n",
			   (unsigned long)tau_lidar.skipped_release_count,
			   (unsigned long)tau_lidar.total_timing_failures);
	  uart_print(msg);

	  TaskReporting_PrintExecHistogram("  exec distribution:", &tau_lidar);
	#endif

	#if ENABLE_SYNTH_CAMERA
	  snprintf(msg, sizeof(msg),
			   "tau_Camera synthetic | runs=%lu\r\n",
			   (unsigned long)tau_camera.run_count);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  period     T=%lu ms | target C=%lu us | deadline D=%lu ms\r\n",
			   (unsigned long)tau_camera.period_ms,
			   (unsigned long)TAU_CAMERA_WORKLOAD_US,
			   (unsigned long)tau_camera.deadline_ms);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  exec_us     avg=%lu min=%lu max=%lu\r\n",
			   (unsigned long)tau_camera_avg_exec,
			   (unsigned long)tau_camera.min_exec_us,
			   (unsigned long)tau_camera.max_exec_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  response_us avg=%lu min=%lu max=%lu\r\n",
			   (unsigned long)tau_camera_avg_response,
			   (unsigned long)tau_camera.min_response_us,
			   (unsigned long)tau_camera.max_response_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  deadline   D=%lu ms | misses=%lu | max_lateness_us=%lu\r\n",
			   (unsigned long)tau_camera.deadline_ms,
			   (unsigned long)tau_camera.deadline_miss_count,
			   (unsigned long)tau_camera.max_lateness_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  skipped releases=%lu | total timing failures=%lu\r\n",
			   (unsigned long)tau_camera.skipped_release_count,
			   (unsigned long)tau_camera.total_timing_failures);
	  uart_print(msg);

	  TaskReporting_PrintExecHistogram("  exec distribution:", &tau_camera);
	#endif

	#if ENABLE_SYNTH_CONTROL
	  snprintf(msg, sizeof(msg),
			   "tau_Control synthetic | runs=%lu\r\n",
			   (unsigned long)tau_control.run_count);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  period     T=%lu ms | target C=%lu us | deadline D=%lu ms\r\n",
			   (unsigned long)tau_control.period_ms,
			   (unsigned long)TAU_CONTROL_WORKLOAD_US,
			   (unsigned long)tau_control.deadline_ms);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  exec_us     avg=%lu min=%lu max=%lu\r\n",
			   (unsigned long)tau_control_avg_exec,
			   (unsigned long)tau_control.min_exec_us,
			   (unsigned long)tau_control.max_exec_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  response_us avg=%lu min=%lu max=%lu\r\n",
			   (unsigned long)tau_control_avg_response,
			   (unsigned long)tau_control.min_response_us,
			   (unsigned long)tau_control.max_response_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  deadline   D=%lu ms | misses=%lu | max_lateness_us=%lu\r\n",
			   (unsigned long)tau_control.deadline_ms,
			   (unsigned long)tau_control.deadline_miss_count,
			   (unsigned long)tau_control.max_lateness_us);
	  uart_print(msg);

	  snprintf(msg, sizeof(msg),
			   "  skipped releases=%lu | total timing failures=%lu\r\n",
			   (unsigned long)tau_control.skipped_release_count,
			   (unsigned long)tau_control.total_timing_failures);
	  uart_print(msg);

	  TaskReporting_PrintExecHistogram("  exec distribution:", &tau_control);
	#endif

  snprintf(msg, sizeof(msg),
           "scheduler decision | loops=%lu\r\n",
           (unsigned long)g_scheduler_metrics.count);
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "  cycles avg=%lu min=%lu max=%lu\r\n",
           (unsigned long)sched_avg_cycles,
           (unsigned long)g_scheduler_metrics.min_cycles,
           (unsigned long)g_scheduler_metrics.max_cycles);
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "  total_us=%lu | overhead=%lu.%04lu %%\r\n",
           (unsigned long)sched_total_us,
           (unsigned long)(sched_overhead_x10000 / 10000),
           (unsigned long)(sched_overhead_x10000 % 10000));
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "polling check | loops=%lu\r\n",
           (unsigned long)g_polling_metrics.count);
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "  cycles avg=%lu min=%lu max=%lu\r\n",
           (unsigned long)poll_avg_cycles,
           (unsigned long)g_polling_metrics.min_cycles,
           (unsigned long)g_polling_metrics.max_cycles);
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "  total_us=%lu | overhead=%lu.%04lu %%\r\n",
           (unsigned long)poll_total_us,
           (unsigned long)(poll_overhead_x10000 / 10000),
           (unsigned long)(poll_overhead_x10000 % 10000));
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "cpu time distribution\r\n");
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "  task_exec total_us=%lu | percent=%lu.%04lu %%\r\n",
           (unsigned long)task_exec_total_us,
           (unsigned long)(task_exec_percent_x10000 / 10000),
           (unsigned long)(task_exec_percent_x10000 % 10000));
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "  measured_busy total_us=%lu | percent=%lu.%04lu %%\r\n",
           (unsigned long)measured_busy_us,
           (unsigned long)(measured_busy_percent_x10000 / 10000),
           (unsigned long)(measured_busy_percent_x10000 % 10000));
  uart_print(msg);

  snprintf(msg, sizeof(msg),
           "  logical_idle total_us=%lu | percent=%lu.%04lu %%\r\n",
           (unsigned long)logical_idle_us,
           (unsigned long)(logical_idle_percent_x10000 / 10000),
           (unsigned long)(logical_idle_percent_x10000 % 10000));
  uart_print(msg);

  #if SCHEDULER_MODE == SCHED_WFI_OPTIMIZED
    uart_print("  WFI enabled: CPU may sleep between interrupts; physical idle is not directly measured\r\n");
  #else
    uart_print("  busy polling enabled: CPU does not sleep; logical idle is spent checking task releases\r\n");
  #endif

#if ENABLE_REAL_TAU1 || ENABLE_REAL_TAU2
  snprintf(msg, sizeof(msg),
           "Last values  | Distance=%d cm | DHT11 result=%d | Temp=%d C | Hum=%d %%\r\n",
           g_distance_cm,
           g_dht_res,
           g_temp,
           g_hum);
  uart_print(msg);
#endif

  /* ProfileSamples_Print(); */

  snprintf(msg, sizeof(msg),
           "CSV_RUN,SCHED=%s,SCENARIO=%s,U=%lu,WINDOW_US=%lu,SYNTH_TASKS=%u,CHUNK_US=%lu,SCHED_LOOPS=%lu,SCHED_OVH=%lu.%04lu,POLL_LOOPS=%lu,POLL_OVH=%lu.%04lu,TASK_EXEC=%lu.%04lu,BUSY=%lu.%04lu,IDLE=%lu.%04lu\r\n",
           SCHED_ALGO_NAME,
           WORKLOAD_SCENARIO_NAME,
           (unsigned long)WORKLOAD_UTILIZATION_PERCENT,
           (unsigned long)PROFILE_WINDOW_US,
           (unsigned int)INTEGRATED_SYNTH_TASK_COUNT,
           (unsigned long)SCHED_CSV_CHUNK_US,
           (unsigned long)g_scheduler_metrics.count,
           (unsigned long)(sched_overhead_x10000 / 10000),
           (unsigned long)(sched_overhead_x10000 % 10000),
           (unsigned long)g_polling_metrics.count,
           (unsigned long)(poll_overhead_x10000 / 10000),
           (unsigned long)(poll_overhead_x10000 % 10000),
           (unsigned long)(task_exec_percent_x10000 / 10000),
           (unsigned long)(task_exec_percent_x10000 % 10000),
           (unsigned long)(measured_busy_percent_x10000 / 10000),
           (unsigned long)(measured_busy_percent_x10000 % 10000),
           (unsigned long)(logical_idle_percent_x10000 / 10000),
           (unsigned long)(logical_idle_percent_x10000 % 10000));
  uart_print(msg);

  #if ENABLE_SYNTH_IMU
    TaskReporting_PrintCsvTask(SCHED_ALGO_NAME, WORKLOAD_SCENARIO_NAME,
                               WORKLOAD_UTILIZATION_PERCENT, "IMU", &tau_imu,
                               TAU_IMU_WORKLOAD_US);
  #endif

  #if ENABLE_SYNTH_LIDAR
    TaskReporting_PrintCsvTask(SCHED_ALGO_NAME, WORKLOAD_SCENARIO_NAME,
                               WORKLOAD_UTILIZATION_PERCENT, "LIDAR", &tau_lidar,
                               TAU_LIDAR_WORKLOAD_US);
  #endif

  #if ENABLE_SYNTH_CAMERA
    TaskReporting_PrintCsvTask(SCHED_ALGO_NAME, WORKLOAD_SCENARIO_NAME,
                               WORKLOAD_UTILIZATION_PERCENT, "CAMERA", &tau_camera,
                               TAU_CAMERA_WORKLOAD_US);
  #endif

  #if ENABLE_SYNTH_CONTROL
    TaskReporting_PrintCsvTask(SCHED_ALGO_NAME, WORKLOAD_SCENARIO_NAME,
                               WORKLOAD_UTILIZATION_PERCENT, "CONTROL", &tau_control,
                               TAU_CONTROL_WORKLOAD_US);
  #endif

  uart_print("=======================================\r\n\r\n");
}


static void Tasks_Init(void)
{
  uint64_t now_us = scheduler_now_us();

	#if ENABLE_REAL_TAU1
	  Task_InitPeriodic(&tau1, "tau1_hcsr04", TAU1_PERIOD_US,
	                    TAU1_PERIOD_MS, now_us);

    #if SCHED_ALGO == SCHED_ALGO_CHUNKED_EDF
      HCSR04_Async_Reset();
    #endif
	#endif

	#if ENABLE_REAL_TAU2
	  Task_InitPeriodic(&tau2, "tau2_dht11", TAU2_PERIOD_US,
	                    TAU2_PERIOD_MS, now_us);

    #if SCHED_ALGO == SCHED_ALGO_CHUNKED_EDF
      DHT11_Async_Reset();
    #endif
	#endif

  #if ENABLE_SYNTH_IMU
    Task_InitPeriodic(&tau_imu, "tau_imu_synthetic", TAU_IMU_PERIOD_US,
                      TAU_IMU_PERIOD_MS, now_us);
  #endif

	#if ENABLE_SYNTH_LIDAR
	  Task_InitPeriodic(&tau_lidar, "tau_lidar_synthetic", TAU_LIDAR_PERIOD_US,
	                    TAU_LIDAR_PERIOD_MS, now_us);
	#endif

	#if ENABLE_SYNTH_CAMERA
	  Task_InitPeriodic(&tau_camera, "tau_camera_synthetic", TAU_CAMERA_PERIOD_US,
	                    TAU_CAMERA_PERIOD_MS, now_us);
	#endif

	#if ENABLE_SYNTH_CONTROL
	  Task_InitPeriodic(&tau_control, "tau_control_synthetic", TAU_CONTROL_PERIOD_US,
	                    TAU_CONTROL_PERIOD_MS, now_us);
	#endif
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  PlatformTime_Init(HAL_RCC_GetHCLKFreq() / 1000000U);

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ETH_Init();
  MX_USART3_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_TIM3_Init();

  /* USER CODE BEGIN 2 */

  DWT_Init();

  #if EXPERIMENT_MODE == EXPERIMENT_MINIMAL_SUPERLOOP_PROFILE
  MinimalSuperloopProfile_Run();
  #elif EXPERIMENT_MODE == EXPERIMENT_SUPERLOOP_CHECKS_PROFILE
  SuperloopChecksProfile_Run();
  #elif EXPERIMENT_MODE == EXPERIMENT_SUPERLOOP_SCALABILITY_CLEAN
  SuperloopScalabilityProfile_RunClean();
  #elif EXPERIMENT_MODE == EXPERIMENT_SUPERLOOP_SCALABILITY_CHECKS
  SuperloopScalabilityProfile_RunChecks();
  #else
  char msg[128];

  uart_print("\r\nScheduler demo started\r\n");
  snprintf(msg, sizeof(msg),
           "Workload scenario: %s\r\n",
           WORKLOAD_SCENARIO_NAME);
  uart_print(msg);
  #if ENABLE_REAL_TAU1 || ENABLE_REAL_TAU2
    uart_print("Real sensor tasks are enabled\r\n");
  #else
    uart_print("Real sensor tasks are disabled\r\n");
  #endif

  HAL_Delay(500);

  Tasks_Init();

  g_profile_start_us = scheduler_now_us();
  g_profile_start_ms = HAL_GetTick();

  #if EXPERIMENT_MODE == EXPERIMENT_ISOLATED_TAU1
    IsolatedProfile_Run(&tau1, Tau1_Run,
                        "\r\n=== ISOLATED TAU1 HC-SR04 PROFILE ===\r\n",
                        "Only tau1 is running. No tau2. No scheduler competition.\r\n",
                        PROFILE_WINDOW_MS,
                        (SCHEDULER_MODE == SCHED_WFI_OPTIMIZED) ? 1U : 0U,
                        Reset_Profiling_Stats, ProfileSamples_SaveTau1,
                        Print_Profiling_Summary);
  #endif

  #if EXPERIMENT_MODE == EXPERIMENT_ISOLATED_TAU2
    IsolatedProfile_Run(&tau2, Tau2_Run,
                        "\r\n=== ISOLATED TAU2 DHT11 PROFILE ===\r\n",
                        "Only tau2 is running. No tau1. No scheduler competition.\r\n",
                        PROFILE_WINDOW_MS,
                        (SCHEDULER_MODE == SCHED_WFI_OPTIMIZED) ? 1U : 0U,
                        Reset_Profiling_Stats, ProfileSamples_SaveTau2,
                        Print_Profiling_Summary);
  #endif
  #endif

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    uint32_t sched_start_cycles;
    uint32_t sched_end_cycles;
    uint32_t scheduler_cycles_this_loop = 0;

    uint64_t now_us = scheduler_now_us();

    /*
     * ============================================================
     * Scheduler decision phase
     * ============================================================
     *
     * Each task has its own next_release_us.
     * A task is ready when now_us >= next_release_us.
     *
     * Real sensors can be disabled completely for synthetic-only
     * scheduling experiments.
     */

    #if SCHED_ALGO == SCHED_ALGO_SUPERLOOP

    #if ENABLE_REAL_TAU1
      sched_start_cycles = DWT->CYCCNT;

      uint8_t tau1_ready = ((int64_t)(now_us - tau1.next_release_us) >= 0);
      uint64_t tau1_release_us = tau1.next_release_us;

      sched_end_cycles = DWT->CYCCNT;
      scheduler_cycles_this_loop += (uint32_t)(sched_end_cycles - sched_start_cycles);
    #else
      uint8_t tau1_ready = 0;
      uint64_t tau1_release_us = 0;
    #endif


    #if ENABLE_REAL_TAU2
      sched_start_cycles = DWT->CYCCNT;

      uint8_t tau2_ready = ((int64_t)(now_us - tau2.next_release_us) >= 0);
      uint64_t tau2_release_us = tau2.next_release_us;

      sched_end_cycles = DWT->CYCCNT;
      scheduler_cycles_this_loop += (uint32_t)(sched_end_cycles - sched_start_cycles);
    #else
      uint8_t tau2_ready = 0;
      uint64_t tau2_release_us = 0;
    #endif


    #if ENABLE_SYNTH_IMU
      sched_start_cycles = DWT->CYCCNT;

      uint8_t tau_imu_ready = ((int64_t)(now_us - tau_imu.next_release_us) >= 0);
      uint64_t tau_imu_release_us = tau_imu.next_release_us;

      sched_end_cycles = DWT->CYCCNT;
      scheduler_cycles_this_loop += (uint32_t)(sched_end_cycles - sched_start_cycles);
    #else
      uint8_t tau_imu_ready = 0;
      uint64_t tau_imu_release_us = 0;
    #endif


    #if ENABLE_SYNTH_CONTROL
      sched_start_cycles = DWT->CYCCNT;

      uint8_t tau_control_ready = ((int64_t)(now_us - tau_control.next_release_us) >= 0);
      uint64_t tau_control_release_us = tau_control.next_release_us;

      sched_end_cycles = DWT->CYCCNT;
      scheduler_cycles_this_loop += (uint32_t)(sched_end_cycles - sched_start_cycles);
    #else
      uint8_t tau_control_ready = 0;
      uint64_t tau_control_release_us = 0;
    #endif


    #if ENABLE_SYNTH_LIDAR
      sched_start_cycles = DWT->CYCCNT;

      uint8_t tau_lidar_ready = ((int64_t)(now_us - tau_lidar.next_release_us) >= 0);
      uint64_t tau_lidar_release_us = tau_lidar.next_release_us;

      sched_end_cycles = DWT->CYCCNT;
      scheduler_cycles_this_loop += (uint32_t)(sched_end_cycles - sched_start_cycles);
    #else
      uint8_t tau_lidar_ready = 0;
      uint64_t tau_lidar_release_us = 0;
    #endif


    #if ENABLE_SYNTH_CAMERA
      sched_start_cycles = DWT->CYCCNT;

      uint8_t tau_camera_ready = ((int64_t)(now_us - tau_camera.next_release_us) >= 0);
      uint64_t tau_camera_release_us = tau_camera.next_release_us;

      sched_end_cycles = DWT->CYCCNT;
      scheduler_cycles_this_loop += (uint32_t)(sched_end_cycles - sched_start_cycles);
    #else
      uint8_t tau_camera_ready = 0;
      uint64_t tau_camera_release_us = 0;
    #endif


    #if ENABLE_POLLING_PROFILE
      CycleMetrics_Update(&g_polling_metrics, scheduler_cycles_this_loop);
    #endif

    uint8_t any_task_ready =
        tau1_ready ||
        tau2_ready ||
        tau_imu_ready ||
        tau_control_ready ||
        tau_lidar_ready ||
        tau_camera_ready;

    if (any_task_ready)
    {
      CycleMetrics_Update(&g_scheduler_metrics, scheduler_cycles_this_loop);
    }


    /*
     * ============================================================
     * Execution phase
     * ============================================================
     *
     * Synthetic-only baseline order:
     *
     *   tau_IMU -> tau_LiDAR -> tau_Camera
     *
     * Optional tasks:
     *   tau_Control can be re-enabled later.
     *   tau1/tau2 are disabled for pure scheduler analysis.
     *
     * Deadline miss:
     *   executed activation finished after its absolute deadline.
     *
     * Skipped release:
     *   activation occurred while the previous activation of the same
     *   task was still not accounted/executed separately.
     */

    #if ENABLE_SYNTH_CONTROL
      if (tau_control_ready)
      {
        uint64_t release_us = tau_control_release_us;

        uint64_t exec_start = micros();

        Tau_Control_Run();

        uint64_t exec_finish = micros();
        uint64_t finish_us = scheduler_now_us();

        uint64_t exec_time = exec_finish - exec_start;
        uint64_t response_time = finish_us - release_us;

        Task_UpdateExecStats(&tau_control, exec_time);
        Task_UpdateResponseStats(&tau_control, response_time);
        Task_CheckDeadline(&tau_control, response_time);

        Task_AdvanceRelease(&tau_control, finish_us);
      }
    #endif


    #if ENABLE_REAL_TAU1
      if (tau1_ready)
      {
        uint64_t release_us = tau1_release_us;

        uint64_t exec_start = micros();

        Tau1_Run();

        uint64_t exec_finish = micros();
        uint64_t finish_us = scheduler_now_us();

        uint64_t exec_time = exec_finish - exec_start;
        uint64_t response_time = finish_us - release_us;

        Task_UpdateExecStats(&tau1, exec_time);
        Task_SaveExecSample(&tau1, exec_time);
        Task_UpdateResponseStats(&tau1, response_time);
        Task_CheckDeadline(&tau1, response_time);

        Task_AdvanceRelease(&tau1, finish_us);
      }
    #endif


    #if ENABLE_SYNTH_LIDAR
      if (tau_lidar_ready)
      {
        uint64_t release_us = tau_lidar_release_us;

        uint64_t exec_start = micros();

        Tau_LiDAR_Run();

        uint64_t exec_finish = micros();
        uint64_t finish_us = scheduler_now_us();

        uint64_t exec_time = exec_finish - exec_start;
        uint64_t response_time = finish_us - release_us;

        Task_UpdateExecStats(&tau_lidar, exec_time);
        Task_UpdateResponseStats(&tau_lidar, response_time);
        Task_CheckDeadline(&tau_lidar, response_time);

        Task_AdvanceRelease(&tau_lidar, finish_us);
      }
    #endif

	#if ENABLE_SYNTH_IMU
      if (tau_imu_ready)
      {
        uint64_t release_us = tau_imu_release_us;

        uint64_t exec_start = micros();

        Tau_IMU_Run();

        uint64_t exec_finish = micros();
        uint64_t finish_us = scheduler_now_us();

        uint64_t exec_time = exec_finish - exec_start;
        uint64_t response_time = finish_us - release_us;

        Task_UpdateExecStats(&tau_imu, exec_time);
        Task_UpdateResponseStats(&tau_imu, response_time);
        Task_CheckDeadline(&tau_imu, response_time);

        Task_AdvanceRelease(&tau_imu, finish_us);
      }
    #endif


    #if ENABLE_SYNTH_CAMERA
      if (tau_camera_ready)
      {
        uint64_t release_us = tau_camera_release_us;

        uint64_t exec_start = micros();

        Tau_Camera_Run();

        uint64_t exec_finish = micros();
        uint64_t finish_us = scheduler_now_us();

        uint64_t exec_time = exec_finish - exec_start;
        uint64_t response_time = finish_us - release_us;

        Task_UpdateExecStats(&tau_camera, exec_time);
        Task_UpdateResponseStats(&tau_camera, response_time);
        Task_CheckDeadline(&tau_camera, response_time);

        Task_AdvanceRelease(&tau_camera, finish_us);
      }
    #endif


    #if ENABLE_REAL_TAU2
      if (tau2_ready)
      {
        uint64_t release_us = tau2_release_us;

        uint64_t exec_start = micros();

        Tau2_Run();

        uint64_t exec_finish = micros();
        uint64_t finish_us = scheduler_now_us();

        uint64_t exec_time = exec_finish - exec_start;
        uint64_t response_time = finish_us - release_us;

        Task_UpdateExecStats(&tau2, exec_time);
        Task_SaveExecSample(&tau2, exec_time);
        Task_UpdateResponseStats(&tau2, response_time);
        Task_CheckDeadline(&tau2, response_time);

        Task_AdvanceRelease(&tau2, finish_us);
      }
    #endif

    #elif SCHED_ALGO == SCHED_ALGO_CHUNKED_EDF

    sched_start_cycles = DWT->CYCCNT;
    EdfExecutorContext_t executor_context =
    {
      &g_distance_cm, &g_temp, &g_hum, &g_dht_res, Task_SaveExecSample
    };
    SchedTaskRef_t *selected_task = Scheduler_SelectChunkedEDF(g_sched_tasks,
                                                               g_sched_task_count,
                                                               now_us);
    sched_end_cycles = DWT->CYCCNT;
    scheduler_cycles_this_loop += (uint32_t)(sched_end_cycles - sched_start_cycles);

    #if ENABLE_POLLING_PROFILE
      CycleMetrics_Update(&g_polling_metrics, scheduler_cycles_this_loop);
    #endif

    if (selected_task != NULL)
    {
      CycleMetrics_Update(&g_scheduler_metrics, scheduler_cycles_this_loop);
      Scheduler_RunChunkedTask(selected_task, &executor_context);
    }

    #endif


    #if ENABLE_DEBUG_PRINT
      /*
       * Debug print. Disabled during real profiling because UART blocks CPU.
       */
      static uint64_t next_debug_us = 0;

      uint64_t now_debug_us = micros();

      if (now_debug_us >= next_debug_us)
      {
        DebugStatus_Print(tau1.run_count, tau2.run_count, g_distance_cm,
                          g_temp, g_hum, g_dht_res);
        next_debug_us = now_debug_us + DEBUG_PERIOD_US;
      }
    #endif


    /*
     * ============================================================
     * End of profiling window
     * ============================================================
     */
    uint64_t profile_now_us = scheduler_now_us();

    if ((profile_now_us - g_profile_start_us) >= PROFILE_WINDOW_US)
    {
      Print_Profiling_Summary();

      Reset_Profiling_Stats();

      g_profile_start_us = scheduler_now_us();
      g_profile_start_ms = HAL_GetTick();

      #if ENABLE_REAL_TAU1
        tau1.next_release_us = g_profile_start_us + tau1.period_us;
        tau1.next_release_ms = (uint32_t)(tau1.next_release_us / 1000ULL);
      #endif

      #if ENABLE_REAL_TAU2
        tau2.next_release_us = g_profile_start_us + tau2.period_us;
        tau2.next_release_ms = (uint32_t)(tau2.next_release_us / 1000ULL);
      #endif

      #if ENABLE_SYNTH_IMU
        tau_imu.next_release_us = g_profile_start_us + tau_imu.period_us;
        tau_imu.next_release_ms = (uint32_t)(tau_imu.next_release_us / 1000ULL);
      #endif

      #if ENABLE_SYNTH_CONTROL
        tau_control.next_release_us = g_profile_start_us + tau_control.period_us;
        tau_control.next_release_ms = (uint32_t)(tau_control.next_release_us / 1000ULL);
      #endif

      #if ENABLE_SYNTH_LIDAR
        tau_lidar.next_release_us = g_profile_start_us + tau_lidar.period_us;
        tau_lidar.next_release_ms = (uint32_t)(tau_lidar.next_release_us / 1000ULL);
      #endif

      #if ENABLE_SYNTH_CAMERA
        tau_camera.next_release_us = g_profile_start_us + tau_camera.period_us;
        tau_camera.next_release_ms = (uint32_t)(tau_camera.next_release_us / 1000ULL);
      #endif
    }


    /*
     * Optional WFI mode.
     *
     * For the current synthetic-only busy baseline, keep:
     *   SCHEDULER_MODE = SCHED_BUSY_POLLING
     *
     * WFI can be tested later separately.
     */
    #if SCHEDULER_MODE == SCHED_WFI_OPTIMIZED
      __WFI();
    #endif
  }

  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 96;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ETH Initialization Function
  * @param None
  * @retval None
  */
static void MX_ETH_Init(void)
{

  /* USER CODE BEGIN ETH_Init 0 */

  /* USER CODE END ETH_Init 0 */

   static uint8_t MACAddr[6];

  /* USER CODE BEGIN ETH_Init 1 */

  /* USER CODE END ETH_Init 1 */
  heth.Instance = ETH;
  MACAddr[0] = 0x00;
  MACAddr[1] = 0x80;
  MACAddr[2] = 0xE1;
  MACAddr[3] = 0x00;
  MACAddr[4] = 0x00;
  MACAddr[5] = 0x00;
  heth.Init.MACAddr = &MACAddr[0];
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.RxBuffLen = 1524;

  /* USER CODE BEGIN MACADDRESS */

  /* USER CODE END MACADDRESS */

  if (HAL_ETH_Init(&heth) != HAL_OK)
  {
    Error_Handler();
  }

  memset(&TxConfig, 0 , sizeof(ETH_TxPacketConfig));
  TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CSUM | ETH_TX_PACKETS_FEATURES_CRCPAD;
  TxConfig.ChecksumCtrl = ETH_CHECKSUM_IPHDR_PAYLOAD_INSERT_PHDR_CALC;
  TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;
  /* USER CODE BEGIN ETH_Init 2 */

  /* USER CODE END ETH_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 83;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 19999;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim3, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */
  HAL_TIM_MspPostInit(&htim3);

}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.dma_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = ENABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, LD1_Pin|GPIO_PIN_2|LD3_Pin|LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(USB_PowerSwitchOn_GPIO_Port, USB_PowerSwitchOn_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : USER_Btn_Pin */
  GPIO_InitStruct.Pin = USER_Btn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USER_Btn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : PC0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  #if (SCHED_ALGO == SCHED_ALGO_CHUNKED_EDF) && ENABLE_REAL_TAU1
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  #else
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  #endif
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  #if (SCHED_ALGO == SCHED_ALGO_CHUNKED_EDF) && ENABLE_REAL_TAU1
  /* HC-SR04 ECHO is captured by EXTI0; ISR only records edge events. */
  HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);
  #endif

  /*Configure GPIO pin : PA5 */
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : LD1_Pin PB2 LD3_Pin LD2_Pin */
  GPIO_InitStruct.Pin = LD1_Pin|GPIO_PIN_2|LD3_Pin|LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_PowerSwitchOn_Pin */
  GPIO_InitStruct.Pin = USB_PowerSwitchOn_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(USB_PowerSwitchOn_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : USB_OverCurrent_Pin */
  GPIO_InitStruct.Pin = USB_OverCurrent_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(USB_OverCurrent_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
