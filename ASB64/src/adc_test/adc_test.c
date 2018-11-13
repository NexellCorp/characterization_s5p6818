#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <time.h>		// ctime

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>		// open

#define _GNU_SOURCE
#ifndef __USE_GNU
#define __USE_GNU
#endif
#include <sched.h>

#include "adc_test.h"

#define nx_debug(fmt, ...)			\
		if (g_debug == 1)			\
			printf(fmt, ##__VA_ARGS__)


int skip_channels[ADC_CHAN_MAX] = { 0, };
int g_repeat = DEF_REPEAT;


enum _test_mode 		{ MODE_ASB = 100, MODE_VTK = 200 };
typedef enum _test_mode TEST_MODE;

TEST_MODE g_mode;					/* ASB, VTK */ 
double g_involt;					/* voltage */
int g_torelance;					/* percentage */
int g_chan = ADC_CHAN_MAX;			/* adc channel : ADC_CHAN_MAX => all, 0~ADC_CHAN_MAX-1 => channel */
int g_sample;						/* sample count */

int g_debug = 0;					/* print debug message. hidden */


void print_info(struct stat *sb)
{

	if (!sb) return;

	printf("File type:                ");

	switch (sb->st_mode & S_IFMT) {
		case S_IFBLK:  printf("block device\n");            break;
		case S_IFCHR:  printf("character device\n");        break;
		case S_IFDIR:  printf("directory\n");               break;
		case S_IFIFO:  printf("FIFO/pipe\n");               break;
		case S_IFLNK:  printf("symlink\n");                 break;
		case S_IFREG:  printf("regular file\n");            break;
		case S_IFSOCK: printf("socket\n");                  break;
		default:       printf("unknown?\n");                break;
	}

	printf("I-node number:            %ld\n", (long) sb->st_ino);

	printf("Mode:                     %lo (octal)\n",
			(unsigned long) sb->st_mode);

	printf("Link count:               %ld\n", (long) sb->st_nlink);
	printf("Ownership:                UID=%ld   GID=%ld\n",
			(long) sb->st_uid, (long) sb->st_gid);

	printf("Preferred I/O block size: %ld bytes\n",
			(long) sb->st_blksize);
	printf("File size:                %lld bytes\n",
			(long long) sb->st_size);
	printf("Blocks allocated:         %lld\n",
			(long long) sb->st_blocks);

	printf("Last status change:       %s", ctime(&sb->st_ctime));
	printf("Last file access:         %s", ctime(&sb->st_atime));
	printf("Last file modification:   %s", ctime(&sb->st_mtime));
}


static int sys_read(const char *file, char *buffer, int buflen)
{
	int fd, ret;

	if (0 != access(file, F_OK)) {
		printf("cannot access file (%s).\n", file);
		return -errno;
	}

	fd = open(file, O_RDONLY);
	if (0 > fd) {
		printf("cannot open file (%s).\n", file);
		return -errno;
	}

	ret = read(fd, buffer, buflen);
	if (0 > ret) {
		printf("failed, read (file=%s, data=%s)\n", file, buffer);
		close(fd);
		return -errno;
	}

	close(fd);

	return ret;
}

#if 0
static int sys_write(const char *file, const char *buffer, int buflen)
{
	int fd;

	if (0 != access(file, F_OK)) {
		printf("cannot access file (%s).\n", file);
		return -errno;
	}

	fd = open(file, O_RDWR|O_SYNC);
	if (0 > fd) {
		printf("cannot open file (%s).\n", file);
		return -errno;
	}

	if (0 > write(fd, buffer, buflen)) {
		printf("failed, write (file=%s, data=%s)\n", file, buffer);
		close(fd);
		return -errno;
	}
	close(fd);

	return 0;
}
#endif



#ifdef USING_POPEN
int get_adc_value(int ch, int is_raw, int count)
{
	unsigned int sample_data[SAMPLE_RANGE] = { 0, };

	FILE *fp;
	int ret = -1;
	char buf[256] = { 0, };
	char cmd[256] = { 0, };
	int err = 0;

	uint32_t idx;

	int min = 0, max = 0, freq = 0;
	int min_flag = 1;
	int i;


	if (count < 0)
		return -1;

	/* /sys/bus/iio/devices/iio\:device0/in_voltage0_raw */
	sprintf(cmd, "%s%d%s", "cat " ADC_SYS_DEV "/in_voltage", ch, is_raw ? "_raw" : "");

	for (i = 0; i < count; i++)
	{
		int _sys_read_err = 0;
		int _sys_read_retry = 5;


		do { 
			memset (buf, 0x00, sizeof buf);

			fp = popen (cmd, "r");
			if (!fp) {
				perror ("popen");
				return -1;
			}
			if (fgets(buf, 256, fp) == NULL) {
				printf("read nothing.");
				_sys_read_err = 1;
			}
			else {
				_sys_read_err = 0;
			}
			pclose (fp);
		} while(_sys_read_err && --_sys_read_retry);		/* w/a specific sys file read failed */


		idx = strtoul(buf, NULL, 10);
		if (idx >= SAMPLE_RANGE)
		{
			err++;
			continue;
		}

		(sample_data[idx])++;
	}


	for (i = 0; i < SAMPLE_RANGE; i++) {
		if (sample_data[i] > 0)
		{
			if (min_flag)
			{
				min = i;
				min_flag = 0;
			}
			max = i;
		}
		if (sample_data[i] >= sample_data[freq])
			freq = i;
	}

	nx_debug("ch[%d]  -    min: %d    max: %d    freq: %d\n", ch, min, max, freq);

	ret = freq;

	if (err > 0)
		ret = -1;

	return ret;
}
#else
int get_adc_value(int ch, int is_raw, int count)
{
	unsigned int sample_data[SAMPLE_RANGE] = { 0, };

	int ret = -1;
	char buf[256] = { 0, };
	char path[256] = { 0, };
	int err = 0;

	uint32_t idx;

	int min = 0, max = 0, freq = 0;
	int min_flag = 1;
	int i;


	if (count < 0)
		return -1;

	/* /sys/bus/iio/devices/iio\:device0/in_voltage0_raw */
	sprintf(path, "%s%d%s", ADC_SYS_DEV "/in_voltage", ch, is_raw ? "_raw" : "");
	nx_debug("path: %s\n", path);

	for (i = 0; i < count; i++)
	{
		ret = sys_read(path, buf, sizeof(buf));
		if (0 > ret) {
			printf("  # sys read error\n");
			return -1;
		}
		nx_debug("buf: [%s]\n", buf);

		idx = strtoul(buf, NULL, 10);
		if (idx >= SAMPLE_RANGE)
		{
			err++;
			continue;
		}

		(sample_data[idx])++;
	}


	for (i = 0; i < SAMPLE_RANGE; i++) {
		if (sample_data[i] > 0)
		{
			if (min_flag)
			{
				min = i;
				min_flag = 0;
			}
			max = i;
		}
		if (sample_data[i] >= sample_data[freq])
			freq = i;
	}

	nx_debug("ch[%d]  -    min: %d    max: %d    freq: %d\n", ch, min, max, freq);

	ret = freq;

	if (err > 0)
		ret = -1;

	return ret;
}
#endif


int asb_pwm_adc_on(int pwm_id)
{
	char cmd[256] = { 0, };

	sprintf(cmd, "echo %d,%d > /sys/devices/platform/pwm/pwm.%d", PWM_FREQ, PWM_DUTY, pwm_id);
	nx_debug("%s\n", cmd);
	system(cmd);

	usleep(300000);

#ifdef ARCH_S5P4418
	memset(cmd, 0x00, 256);

	sprintf(cmd, "echo %d > /sys/class/gpio/unexport 2>/dev/null", to_gpionum(pwm_id));
	nx_debug("%s\n", cmd);
	system(cmd);
#endif

	return 0;
}

int asb_pwm_adc_off(int pwm_id)
{
#ifdef ARCH_S5P4418
	char cmd1[256] = { 0, };
	char cmd2[256] = { 0, };
	char cmd3[256] = { 0, };

	sprintf(cmd1, "echo %d > /sys/class/gpio/export 2>/dev/null", to_gpionum(pwm_id));
	sprintf(cmd2, "echo out > /sys/class/gpio/gpio%d/direction", to_gpionum(pwm_id));
	sprintf(cmd3, "echo 0 > /sys/class/gpio/gpio%d/value", to_gpionum(pwm_id));

	nx_debug("%s\n", cmd1);
	nx_debug("%s\n", cmd2);
	nx_debug("%s\n", cmd3);

	system(cmd1);
	system(cmd2);
	system(cmd3);
#endif

	return 0;
}


int asb_init_pwm(void)
{
	int pwm_chan;
	int i;

	for(i = 0; i < ADC_CHAN_MAX; i++)
		pwm_chan = to_pwm_chan(i);
		if (pwm_chan >= 0)
			asb_pwm_adc_off(pwm_chan);

	return 0;
}

int asb_prepare_pwm(int adc_chan)
{
	int pwm_chan = to_pwm_chan(adc_chan);
	if (pwm_chan >= 0)
		asb_pwm_adc_on(pwm_chan);

	return 0;
}

int asb_finish_pwm(int adc_chan)
{
	int pwm_chan = to_pwm_chan(adc_chan);
	if (pwm_chan >= 0)
		asb_pwm_adc_off(pwm_chan);

	return 0;
}


int get_adc_values(int *values)
{
	int i;
	int ch = ADC_CHAN_MAX;

	if (g_mode == MODE_ASB)
		asb_init_pwm();

	for (i = 0; i < ch; i++) {
		if (g_mode == MODE_VTK && g_chan != ADC_CHAN_MAX && g_chan != i)
			continue;

		if (skip_channels[i])		/* skipping */
			continue;

		if (g_mode == MODE_ASB)
			asb_prepare_pwm(i);

		values[i] = get_adc_value(i, GET_RAW, g_sample);
		if (values[i] < 0) {
			printf("Cannot get adc channel %d value.\n", i);
			return -1;
		}

		if (g_mode == MODE_ASB)
			asb_finish_pwm(i);
	}

	return 0;
}


/*
 * Test accuracy for ASB.
 *
 * argument:
 *      value         read from adc
 *      input         input voltage (Min 0 ~ Max 1.8v)
 *      thr           threshold
 *
 * return:
 *      PASS          test pass
 *      FAIL          test fail
 *      otherwise     error
 */
int test_accuracy_asb(int value, double input, int thr)
{
	double exp;
	int min, max;

	if (input < 0 || input > MAX_VOLTAGE)
		return -EINVAL;

	if (thr < 0 || thr > 100)
		return -EINVAL;

	exp = input / MAX_VOLTAGE * 4096;

	min = (int)(exp * ((100 - thr)/100.0));
	max = (int)(exp * ((100 + thr)/100.0));

	nx_debug(" val: %d (permit range: %d ~ %d)\n", value, min, max);

	if (value >= min && value <= max)
		return PASS;

	return FAIL;
}


/*
 *  Test accuracy for VTK.
 *
 *  return:
 *      PASS          test pass ((max - min)/4096 < torelance)
 *      FAIL          test fail
 *      otherwise     error
 */
int test_accuracy_vtk(int *values)
{
	int i = 0;
	int min, max;
	double diff;

	if (!values) {
		nx_debug("Cannot check accuracy! Invalid arguments.\n");
		return -EINVAL;
	}

	if (g_chan != ADC_CHAN_MAX) {
		printf("val: %d\n", values[g_chan]);
		return PASS;
	}


	for (i = 0; i < ADC_CHAN_MAX; i++) {
		if (skip_channels[i])
			continue;

		break;
	}
	nx_debug(" Start channel: %d\n", i);

	min = max = values[i];

	for (; i < ADC_CHAN_MAX; i++) {
		if (skip_channels[i])
			continue;

		if (min > values[i]) min = values[i];
		if (max < values[i]) max = values[i];
	}

	diff = ((max - min) / (double)SAMPLE_RANGE) * 100;
	nx_debug("\tvalues -    min: %d,  max: %d,   diff: %f\n", min, max, diff);

	if (diff < g_torelance)
		return PASS;
	else
		return FAIL;
}

/*
 *  Test accuracy with get from ADC.
 *
 *  return:
 *      PASS          test pass
 *      FAIL          test fail
 *      otherwise     error
 */
int test_accuracy(int *values)
{
	int i = 0;
	int ret, errcnt = 0;
	int result = PASS;

	if (g_mode == MODE_ASB) {
		for (i = 0; i < ADC_CHAN_MAX; i++)
		{
			nx_debug("  ch[%d] -", i);
			ret = test_accuracy_asb(values[i], ASB_INVOLT_TABLE[i], g_torelance);
			if (ret != PASS)
				errcnt++;
		}

		if (errcnt == 0)
			result = PASS;
		else
			result = FAIL;
	}
	else if (g_mode == MODE_VTK) {
		result = test_accuracy_vtk(values);
	}

	return result;
}


void print_usage(void)
{
	fprintf(stderr, "Usage: %s (ver.%s)\n", PROG_NAME, VERSION_STRING);
	fprintf(stderr, "                [-m TestMode]        (ASB or VTK. default:VTK)\n");
	fprintf(stderr, "                [-t torelance]       (default:%d)\n", DEF_TORELANCE);
	fprintf(stderr, "                [-n sample count]    (default:%d)\n", DEF_SAMPLE_COUNT);
	fprintf(stderr, "                [-v voltage]         (0~1.8V. default:%f)\n", DEF_VOLTAGE);
	fprintf(stderr, "                [-c channel]         (0~%d. 0:all(default), else:channel)\n\n",
					ADC_CHAN_MAX);
	fprintf(stderr, "                [-s skip channel]    \n");
	fprintf(stderr, "                [-C test_count]      (default %d)\n", DEF_REPEAT);
	fprintf(stderr, "                [-T not support]\n");
	fprintf(stderr, "    NOTE: 'voltage' and 'channel' option applies only if the VTK.\n");
	fprintf(stderr, "          ASB tests only 1-6 channel.\n");
}

int parse_opt(int argc, char *argv[])
{
	int opt;

	TEST_MODE _mode = MODE_VTK;
	double _involt = 0.0;
	int _torelance = 0;
	int _chan = 0;
	int _skip = 0;
	int _sample = 0;
	int _repeat = 0;

	char tmp[256] = { 0, };


	while ((opt = getopt(argc, argv, "m:v:c:n:t:s:C:Tdh")) != -1) {
		switch (opt) {
			case 'C':
				_repeat		= atoi(optarg);
				break;
			case 'T':
				print_usage();
				break;
			case 'm':
				snprintf(tmp, 256, "%s", optarg);
				_mode = (strcmp(tmp, "ASB") == 0) ? MODE_ASB : MODE_VTK;
				break;
			case 'v':
				_involt		= strtod(optarg, NULL);
				break;
			case 'c':
				_chan		= atoi(optarg);
				break;
			case 'n':
				_sample		= atoi(optarg);
				break;
			case 't':
				_torelance	= atoi(optarg);
				break;
			case 's':
				_skip		= atoi(optarg);
				if (_skip >= 0 && _skip < ADC_CHAN_MAX)
					skip_channels[_skip] = 1;
				break;
			case 'd':
				g_debug		= 1;
				break;
			case 'h':
			default: /* '?' */
				print_usage();
				exit(EINVAL);
		}
	}


	nx_debug("m: %d, v:%f, c:%d, n:%d, t:%d\n", _mode, _involt, _chan, _sample, _torelance);

	if (_repeat < 0) {
		goto _exit;
	}

	if (_involt < 0.0 || _involt > MAX_VOLTAGE) {
		goto _exit;
	}

	if (_torelance < 0 || _torelance > 100) {
		goto _exit;
	}

	if (_chan < 0 || _chan > ADC_CHAN_MAX) {
		goto _exit;
	}

	if (_sample < 0) {
		goto _exit;
	}

	g_repeat = (_repeat == 0) ? DEF_REPEAT : _repeat;
	g_mode = _mode;
	g_chan = (_chan == 0) ? ADC_CHAN_MAX : _chan-1;
	g_torelance = (_torelance == 0) ? DEF_TORELANCE : _torelance;
	g_involt = (_involt == 0) ? DEF_VOLTAGE  : _involt;
	g_sample = (_sample == 0) ? DEF_SAMPLE_COUNT : _sample;

	return 0;

_exit:
	print_usage();
	exit(EINVAL);
}

int run_test(void)
{
	int values[ADC_CHAN_MAX];
	int result = FAIL;

	if (get_adc_values(values) < 0)
		exit(-1);

	result = test_accuracy(values);

	printf("\tresult =========>  %s\n", (result == PASS) ? "pass" : "fail");

	return result;
}






/* ================== DEBUGGING START ===================== */

void print_running_cpu(void)
{
	int cpu;
	cpu = sched_getcpu();

	nx_debug(" > currently %s running on CPU #%d.\n", PROG_NAME, cpu);
}

void set_cpu_affinity(void)
{
	int cpu = 0;					/* run cpu 1 : 0 test ok */

	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	CPU_SET(cpu, &cpu_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &cpu_set);

	nx_debug(" * setting cpu affinity : [%d]\n", cpu);
}

void print_cpu_affinity(void)
{
	int i, count;

	cpu_set_t cpu_set;
	CPU_ZERO(&cpu_set);
	sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);

	count = CPU_COUNT(&cpu_set);

	nx_debug(" * affinity count : %d(", count);
	for (i = 0; i < count; i++) {
		if (CPU_ISSET(i, &cpu_set))
			nx_debug(" %d", i);
	}
	nx_debug(")\n");
}

void pin_cpu(void)
{
	print_running_cpu();

	print_cpu_affinity();
	set_cpu_affinity();
	print_cpu_affinity();

	print_running_cpu();
}

/* ================== DEBUGGING END ======================= */








/* */
int main(int argc, char *argv[])
{
	struct stat sb;

	int err;
	int r;			// repeat


	pin_cpu();	// w/a translation fault issue

	parse_opt(argc, argv);

	if (stat(ADC_SYS_DEV, &sb) == -1) {
		err = errno;
		perror("stat");
		exit(err);
	}

	//print_info(&sb);

	for (r = 1; r <= g_repeat; r++) {
		printf("[TRY #%d] ....\n", r);
		if (run_test() != PASS) {
			exit(EBADMSG);
		}
	}

	exit(EXIT_SUCCESS);
}
