
#include <common.h>
#include <sys/time.h>

u64 get_os_timer_freq() {
  return 1000000;
}

u64 read_os_timer() {
  timeval v;
  gettimeofday(&v, 0);
  return get_os_timer_freq() * (u64)v.tv_sec + (u64)v.tv_usec;
}

inline u64 read_cpu_timer() {
  return __rdtsc();
}

u64 estimate_clock_speed() {
  u64 freq = get_os_timer_freq(); // How much the os clock advances in 1 second
  u64 measurement_time = freq * 100 / 1000; // 100 milliseconds in os clock time
  u64 cpu_start = read_cpu_timer();
  u64 start = read_os_timer();
  u64 elapsed = 0;
  while (elapsed < measurement_time) { // Wait until one measurement time has passed
    elapsed = read_os_timer() - start;
  }
  u64 cpu_elapsed = read_cpu_timer() - cpu_start; // How many clocks happened in the measurement time
  u64 cpu_freq = cpu_elapsed * freq / elapsed; // clocks per second
  return cpu_freq;
}

#ifndef NO_PROFILE

struct TimerMetric {
  const char *file, *name;
  u32 line_number;
  u32 call_count;
  // Time elapsed only within this block, excluding child blocks
  u64 cycles_elapsed_internal;
  // Total time elapsed within block, including child blocks
  u64 cycles_elapsed_total;
  u64 bytes_processed;
};

#define MAX_GLOBAL_TIMER_METRICS 64
TimerMetric global_timer_metrics[MAX_GLOBAL_TIMER_METRICS] = {};
u32 global_timer_active_block_id = 0;

struct TimedBlock {
  u64 start_time;
  u64 starting_total_cycles_elapsed;
  const char *file, *name;
  u32 line_number;
  u32 block_id;
  u32 parent_block_id;
  inline TimedBlock(u32 id, const char* file, u32 line_number, const char *name, u64 bytes_processed) {
    start_time = read_cpu_timer();
    this->file = file;
    this->name = name;
    this->line_number = line_number;
    this->block_id = id;
    this->starting_total_cycles_elapsed = global_timer_metrics[id].cycles_elapsed_total;
    this->parent_block_id = global_timer_active_block_id;
    global_timer_metrics[id].bytes_processed = bytes_processed;
    global_timer_active_block_id = id;
  }
  inline ~TimedBlock() {
    u64 cycles_elapsed = read_cpu_timer() - start_time;
    auto metric = &global_timer_metrics[block_id];
    metric->file = file;
    metric->name = name;
    metric->line_number = line_number;
    metric->call_count++;
    metric->cycles_elapsed_internal += cycles_elapsed;
    metric->cycles_elapsed_total = cycles_elapsed + starting_total_cycles_elapsed;

    if (parent_block_id) {
      global_timer_metrics[parent_block_id].cycles_elapsed_internal -= cycles_elapsed;
    }
    global_timer_active_block_id = parent_block_id;
  }
};

#define _BEGIN_TIMED_BLOCK(mangle, name, bytes_processed) \
  TimedBlock internal_timed_block__##mangle(__COUNTER__ + 1, __FILE__, __LINE__, name, bytes_processed)
#define TIME_BANDWIDTH(name, bytes_processed) _BEGIN_TIMED_BLOCK(name, #name, bytes_processed)
#define FUNCTION_BANDWIDTH(bytes_processed) _BEGIN_TIMED_BLOCK(__func__, __func__, bytes_processed)
#define BEGIN_TIMED_BLOCK(name) _BEGIN_TIMED_BLOCK(name, #name, 0)
#define END_TIMED_BLOCK(name) internal_timed_block__##name.~TimedBlock()
#define TIMED_FUNCTION() _BEGIN_TIMED_BLOCK(__func__, __func__, 0)
#define CHECK_GLOBAL_TIMER_COUNT() static_assert(MAX_GLOBAL_TIMER_METRICS >= __COUNTER__, "Max timed block count exceeded")

#else

#define TIME_BANDWIDTH(name, bytes_processed)
#define FUNCTION_BANDWIDTH(bytes_processed)
#define BEGIN_TIMED_BLOCK(name)
#define END_TIMED_BLOCK(name)
#define TIMED_FUNCTION()
#define CHECK_GLOBAL_TIMER_COUNT()

#endif

f64 percent(u64 n, u64 d) {
  if (n == 0 && d == 0) return 0;
  return f64(n) / f64(d) * 100.0;
}

void print_timers(u64 start_time, u64 end_time) {
  u64 total_cycles = end_time - start_time;
  u64 clock_speed = estimate_clock_speed();
  printf("Clock speed:  %15f GHz\n", f64(clock_speed)/1000000000.0);
  printf("Total cycles: %15llu (%.2fms)\n\n", total_cycles, f64(total_cycles)/f64(clock_speed)*1000);

#ifndef NO_PROFILE
  for (int i = 0; i < MAX_GLOBAL_TIMER_METRICS; i++) {
    auto metric = global_timer_metrics[i];
    if (!metric.name || !metric.file || !metric.call_count) continue;
    printf(KWHT "%s" KNRM "  (%s:%d):\n", metric.name, metric.file, metric.line_number);
    printf(
      "%15llu " KYEL "(%4.1f%%)" KNRM " \tin %d calls\n",
      metric.cycles_elapsed_internal,
      percent(metric.cycles_elapsed_internal, total_cycles),
      metric.call_count
    );
    if (metric.cycles_elapsed_internal != metric.cycles_elapsed_total) {
      printf( 
        "%15llu " KGRN "(%4.1f%%)" KNRM " total\n",
        metric.cycles_elapsed_total,
        percent(metric.cycles_elapsed_total, total_cycles)
      );
    }
    if (metric.bytes_processed) {
      f64 seconds = f64(metric.cycles_elapsed_total)/f64(clock_speed);
      f64 megabyte = 1024.0 * 1024.0;
      f64 gigabyte = megabyte * 1024.0;
      f64 throughput = f64(metric.bytes_processed) / gigabyte / seconds;
      f64 processed_mb = f64(metric.bytes_processed) / megabyte;
      printf("%15.2f mb processed at a rate of %.2f gb/s\n", processed_mb, throughput);
    }
  }
#endif
}

