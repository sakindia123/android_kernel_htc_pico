#include <linux/sched.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/wakelock.h>
#include <linux/time.h>
#include <linux/kernel_stat.h>
#include <linux/rtc.h>
#include <../clock.h>
#include <../clock-local.h>
#include <../pm.h>
#include <linux/vmalloc.h>
#include <mach/board.h>
#include <mach/perflock.h>
#include <mach/msm_iomap.h>


#define HTC_PM_STATSTIC_DELAY			10000

extern void htc_print_active_wake_locks(int type);
extern void htc_show_interrupts(void);
extern void htc_timer_stats_OnOff(char onoff);
extern void htc_timer_stats_show(u16 water_mark);

#ifndef arch_idle_time
#define arch_idle_time(cpu) 0
#endif

static struct workqueue_struct *htc_pm_monitor_wq = NULL;
struct delayed_work htc_pm_delayed_work;

void htc_kernel_top(void);

static spinlock_t lock;

struct st_htc_IDLE_Statistic {
	u32 count;
	u32 time;
};

struct st_htc_IDLE_Statistic htc_IDLE_Stat[3] = {{0, 0} , {0, 0} , {0, 0} };

#if CONFIG_SMD_OFFSET_TCXO_STAT
struct smem_sleep_stat {
	uint32_t tcxo_time;
	uint32_t tcxo_cnt;
	uint32_t suspend_tcxo_time;
	uint32_t suspend_tcxo_cnt;
	uint32_t garbage_pkt_cnt;
	uint32_t zone_based_reg_cnt;
	uint32_t idle_hand_off_cnt;
	uint32_t mo_2g_probe_cnt;
	uint32_t mo_3g_probe_cnt;
	uint32_t garbage_cnt;
	uint32_t cell_swt_cnt;
	uint32_t reserved[3];
};

void print_sleep_stat(void)
{
	struct timespec ts;
	struct rtc_time tm;
	struct smem_sleep_stat *sleep_stat;
	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);

	sleep_stat = (struct smem_sleep_stat *)
		(MSM_SHARED_RAM_BASE + CONFIG_SMD_OFFSET_TCXO_STAT);

	if (!sleep_stat)
		return;

	printk(KERN_INFO "[K] tcxo_stat: %dms %d %dms %d - "
		"%d %d %d - %d %d - %d %d %d %d %d"
		"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n",
		sleep_stat->tcxo_time, sleep_stat->tcxo_cnt,
		sleep_stat->suspend_tcxo_time, sleep_stat->suspend_tcxo_cnt,
		sleep_stat->garbage_pkt_cnt, sleep_stat->zone_based_reg_cnt,
		sleep_stat->idle_hand_off_cnt, sleep_stat->mo_2g_probe_cnt,
		sleep_stat->mo_3g_probe_cnt, sleep_stat->garbage_cnt,
		sleep_stat->cell_swt_cnt, sleep_stat->reserved[0],
		sleep_stat->reserved[1], sleep_stat->reserved[2],
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);

}
#endif

void htc_idle_stat_clear(void)
{
	memset(htc_IDLE_Stat, 0, sizeof(htc_IDLE_Stat));
}

void htc_idle_stat_add(int sleep_mode, u32 time)
{
	switch (sleep_mode) {
	case MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT:
		htc_IDLE_Stat[0].count++;
		htc_IDLE_Stat[0].time += time;
		break;
	case MSM_PM_SLEEP_MODE_POWER_COLLAPSE_STANDALONE:
		htc_IDLE_Stat[1].count++;
		htc_IDLE_Stat[1].time += time;
		break;
	case MSM_PM_SLEEP_MODE_POWER_COLLAPSE:
		htc_IDLE_Stat[2].count++;
		htc_IDLE_Stat[2].time += time;
		break;
	default:
		break;
	}
}

void htc_idle_stat_show(u32 total_time)
{
	int i = 0;
	u32 idle_time = 0;
	total_time *= 1000;

	for (i = 0 ; i < 3 ; i++) {
		if (htc_IDLE_Stat[i].count) {
			idle_time += htc_IDLE_Stat[i].time;
			printk(KERN_INFO "[K] C%d: %d %dms\n", i, htc_IDLE_Stat[i].count, htc_IDLE_Stat[i].time / 1000);
		}
	}

	printk(KERN_INFO "[K] CPU0 usage: %d\n", ((total_time - (idle_time)) * 100) / total_time);
}

void htc_PM_monitor_work(struct work_struct *work)
{
	struct timespec ts;
	struct rtc_time tm;

	if (htc_pm_monitor_wq == NULL)
		return;

	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec - (sys_tz.tz_minuteswest * 60), &tm);
	printk(KERN_INFO "[K] [PM] hTC PM Statistic ");
	printk(KERN_INFO "[K] %02d-%02d %02d:%02d:%02d \n", tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	/* htc_show_interrupts(); */
	htc_idle_stat_show(HTC_PM_STATSTIC_DELAY);
	#if CONFIG_SMD_OFFSET_TCXO_STAT
	print_sleep_stat();
	#endif
	htc_idle_stat_clear();
	htc_timer_stats_OnOff('0');
	htc_timer_stats_show(300);
	htc_timer_stats_OnOff('1');
	htc_print_active_perf_locks();
	htc_print_active_wake_locks(WAKE_LOCK_IDLE);
	htc_print_active_wake_locks(WAKE_LOCK_SUSPEND);

	queue_delayed_work(htc_pm_monitor_wq, &htc_pm_delayed_work, msecs_to_jiffies(HTC_PM_STATSTIC_DELAY));

	htc_kernel_top();
}

static u32 full_loading_counter = 0;

#define MAX_PID 32768
#define NUM_BUSY_THREAD_CHECK 5
/* Previous process state */
unsigned int *prev_proc_stat = NULL;
int *curr_proc_delta = NULL;
struct task_struct **task_ptr_array = NULL;
struct cpu_usage_stat  new_cpu_stat, old_cpu_stat;

int findBiggestInRange(int *array, int max_limit_idx)
{
	int largest_idx = 0, i;

	for (i = 0 ; i < MAX_PID ; i++) {
		if (array[i] > array[largest_idx] && (max_limit_idx == -1 || array[i] < array[max_limit_idx]))
			largest_idx = i;
	}

	return largest_idx;
}

/* sorting from large to small */
void sorting(int *source, int *output)
{
	int i;
	for (i = 0 ; i < NUM_BUSY_THREAD_CHECK ; i++) {
		if (i == 0)
			output[i] = findBiggestInRange(source, -1);
		else
			output[i] = findBiggestInRange(source, output[i-1]);
	}
}

/* Sync fs/proc/stat.c to caculate all cpu statistics */
static void get_all_cpu_stat(struct cpu_usage_stat *cpu_stat)
{
	int i;
	cputime64_t user, nice, system, idle, iowait, irq, softirq, steal;
	cputime64_t guest, guest_nice;

	if (!cpu_stat)
		return;

	user = nice = system = idle = iowait =
		irq = softirq = steal = cputime64_zero;
	guest = guest_nice = cputime64_zero;

	for_each_possible_cpu(i) {
		user = cputime64_add(user, kstat_cpu(i).cpustat.user);
		nice = cputime64_add(nice, kstat_cpu(i).cpustat.nice);
		system = cputime64_add(system, kstat_cpu(i).cpustat.system);
		idle = cputime64_add(idle, kstat_cpu(i).cpustat.idle);
		idle = cputime64_add(idle, arch_idle_time(i));
		iowait = cputime64_add(iowait, kstat_cpu(i).cpustat.iowait);
		irq = cputime64_add(irq, kstat_cpu(i).cpustat.irq);
		softirq = cputime64_add(softirq, kstat_cpu(i).cpustat.softirq);
		steal = cputime64_add(steal, kstat_cpu(i).cpustat.steal);
		guest = cputime64_add(guest, kstat_cpu(i).cpustat.guest);
		guest_nice = cputime64_add(guest_nice,
			kstat_cpu(i).cpustat.guest_nice);
	}
	cpu_stat->user = user;
	cpu_stat->nice = nice;
	cpu_stat->system = system;
	cpu_stat->softirq = softirq;
	cpu_stat->irq = irq;
	cpu_stat->idle = idle;
	cpu_stat->iowait = iowait;
	cpu_stat->steal = steal;
	cpu_stat->guest = guest;
	cpu_stat->guest_nice = guest_nice;
}

void htc_kernel_top(void)
{
	struct task_struct *p;
	int top_loading[NUM_BUSY_THREAD_CHECK], i;
	unsigned long user_time, system_time, io_time;
	unsigned long irq_time, idle_time, delta_time;
	ulong flags;
	struct task_cputime cputime;
	int dump_top_stack = 0;

	if (task_ptr_array == NULL ||
			curr_proc_delta == NULL ||
			prev_proc_stat == NULL)
		return;

	spin_lock_irqsave(&lock, flags);
	get_all_cpu_stat(&new_cpu_stat);

	/* calculate the cpu time of each process */
	for_each_process(p) {
		thread_group_cputime(p, &cputime);

		if (p->pid < MAX_PID) {
			curr_proc_delta[p->pid] =
				(cputime.utime + cputime.stime)
				- (prev_proc_stat[p->pid]);
			task_ptr_array[p->pid] = p;
		}
	}

	/* sorting to get the top cpu consumers */
	sorting(curr_proc_delta, top_loading);

	/* calculate the total delta time */
	user_time = (unsigned long)((new_cpu_stat.user + new_cpu_stat.nice)
			- (old_cpu_stat.user + old_cpu_stat.nice));
	system_time = (unsigned long)(new_cpu_stat.system - old_cpu_stat.system);
	io_time = (unsigned long)(new_cpu_stat.iowait - old_cpu_stat.iowait);
	irq_time = (unsigned long)((new_cpu_stat.irq + new_cpu_stat.softirq)
			- (old_cpu_stat.irq + old_cpu_stat.softirq));
	idle_time = (unsigned long)
	((new_cpu_stat.idle + new_cpu_stat.steal + new_cpu_stat.guest)
	 - (old_cpu_stat.idle + old_cpu_stat.steal + old_cpu_stat.guest));
	delta_time = user_time + system_time + io_time + irq_time + idle_time;

	/*
	 * Check if we need to dump the call stack of top CPU consumers
	 * If CPU usage keeps 100% for 90 secs
	 */
	if ((full_loading_counter >= 9) && (full_loading_counter % 3 == 0))
		 dump_top_stack = 1;

	/* print most time consuming processes */
	printk(KERN_INFO "[K] CPU Usage\tPID\t\tName\n");
	for (i = 0 ; i < NUM_BUSY_THREAD_CHECK ; i++) {
		printk(KERN_INFO "[K] %lu%%\t\t%d\t\t%s\t\t\t%d\n",
				curr_proc_delta[top_loading[i]] * 100 / delta_time,
				top_loading[i],
				task_ptr_array[top_loading[i]]->comm,
				curr_proc_delta[top_loading[i]]);
	}

	/* check if dump busy thread stack */
	if (dump_top_stack) {
	   struct task_struct *t;
	   for (i = 0 ; i < NUM_BUSY_THREAD_CHECK ; i++) {
		if (task_ptr_array[top_loading[i]] != NULL && task_ptr_array[top_loading[i]]->stime > 0) {
			t = task_ptr_array[top_loading[i]];
			/* dump all the thread stack of this process */
			do {
				printk(KERN_INFO "\n[K] ###pid:%d name:%s state:%lu ppid:%d stime:%lu utime:%lu\n",
				t->pid, t->comm, t->state, t->real_parent->pid, t->stime, t->utime);
				show_stack(t, t->stack);
				t = next_thread(t);
			} while (t != task_ptr_array[top_loading[i]]);
		}
	   }
	}
	/* save old values */
	for_each_process(p) {
		if (p->pid < MAX_PID) {
			thread_group_cputime(p, &cputime);
			prev_proc_stat[p->pid] = cputime.stime + cputime.utime;
		}
	}

	old_cpu_stat = new_cpu_stat;

	memset(curr_proc_delta, 0, sizeof(int) * MAX_PID);
	memset(task_ptr_array, 0, sizeof(int) * MAX_PID);

	spin_unlock_irqrestore(&lock, flags);

}

void htc_PM_monitor_init(void)
{
	if (htc_pm_monitor_wq == NULL)
		return;

	queue_delayed_work(htc_pm_monitor_wq, &htc_pm_delayed_work, msecs_to_jiffies(HTC_PM_STATSTIC_DELAY));

	spin_lock_init(&lock);

	prev_proc_stat = vmalloc(sizeof(int) * MAX_PID);
	curr_proc_delta = vmalloc(sizeof(int) * MAX_PID);
	task_ptr_array = vmalloc(sizeof(int) * MAX_PID);

	memset(prev_proc_stat, 0, sizeof(int) * MAX_PID);
	memset(curr_proc_delta, 0, sizeof(int) * MAX_PID);
	memset(task_ptr_array, 0, sizeof(int) * MAX_PID);

	get_all_cpu_stat(&new_cpu_stat);
	get_all_cpu_stat(&old_cpu_stat);
}


void htc_monitor_init(void)
{
	if (htc_pm_monitor_wq == NULL) {
		/* Create private workqueue... */
		htc_pm_monitor_wq = create_workqueue("htc_pm_monitor_wq");
		printk(KERN_INFO "[K] Create HTC private workqueue(0x%x)...\n", (unsigned int)htc_pm_monitor_wq);
	}

	if (htc_pm_monitor_wq)
		INIT_DELAYED_WORK(&htc_pm_delayed_work, htc_PM_monitor_work);
}

