#include <stdio.h>
#include <unistd.h>	/* optarg */
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>  /* strerror */
#include <errno.h>   /* errno */
#include <sys/time.h> 	// gettimeofday()

#include <zlib.h>
#include <pthread.h>

#include "cpu_lib.h"
#include "cpu_asv.h"

#define	DEFAULT_HOLD_TIME_MS		(1000)
#define	DEFAULT_ZIP_LENGTH_MB		(1)
#define	DEFAULT_ARM_REGULATOR_ID	(1)
#define	DEFAULT_THREAD_MAX			(16)

/* PMIC */

void print_usage(void)
{
	printf( "usage: options\n"
    		"-r random speed change (default sequential min -> max)\n"
    		"-o speed operation time (%dms)\n"
    		"-z zip/unzip in operation time\n"
    		"-l zip/unzip length, unit MB (use with \"-z\" option) (default %dMB)\n"
    		"-m 'mhz' app path to check cpu speed\n"
    		"-v change arm regulator ID (default %d)\n"
    		"-d change voltage (ex. +vol, -vol(mV) or +n%%, -n%%)\n"
    		"-b read margin from boot device\n"
    		"-C test count \n"
    		"-T test time (ms)\n"
    		, DEFAULT_HOLD_TIME_MS
    		, DEFAULT_ZIP_LENGTH_MB
    		, DEFAULT_ARM_REGULATOR_ID
    		);
}

/*
 * MAIN
 */
struct freq_volt {
	long khz;
	long uV;
};

static int get_freq_tables(struct freq_volt *tables, int size)
{
	long *tb;
	int i = 0, nr;
	int cnt = cpu_avail_freqnr();

	if (0 >= cnt)
		return -EINVAL;

	tb = malloc(sizeof(long)*cnt);
	if (NULL == tb)
		return -ENOMEM;

	memset(tb, 0, sizeof(sizeof(long)*cnt));

	cnt = cpu_avail_freqs(tb, size);
	if (0 > cnt)
		return -EINVAL;
	nr = cnt;

	for (i = 0; size > i && cnt > i; i++)
		tables[i].khz = tb[i];

	memset(tb, 0, sizeof(sizeof(long)*cnt));

	cnt = cpu_avail_volts(tb, size);
	if (0 > cnt)
		return nr;

	for (i = 0; size > i && cnt > i; i++)
		tables[i].uV = tb[i];

	return nr;
}

static unsigned long run_test_mhz(char *cmd)
{
    FILE *fp;
	char buf[256] = { 0, };
	char speed[32] = { 0, };
	size_t len = 0;

    fp = popen(cmd, "r");
	if (NULL == fp)  {
        printf("failed %s\n", cmd);
        return -errno;
    }

    len = fread((void*)buf, sizeof(char), sizeof(buf), fp);
    if(0 == len) {
        pclose(fp);
        printf("failed read for %s\n", cmd);
        return -errno;
    }
    pclose(fp);

	strncpy(speed, buf, strchr(buf, 'M')-1-buf);

    return (strtol(speed, NULL, 10)*1000);
}

#define Z_OK            0
#define Z_STREAM_END    1
#define Z_NEED_DICT     2
#define Z_ERRNO        (-1)
#define Z_STREAM_ERROR (-2)
#define Z_DATA_ERROR   (-3)
#define Z_MEM_ERROR    (-4)
#define Z_BUF_ERROR    (-5)
#define Z_VERSION_ERROR (-6)

static void *run_test_zip(void *data)
{
	long length = *(long*)data;
	unsigned char *inbuf = NULL, *zbuf = NULL, *uzbuf = NULL;
	long zlen, uzlen;
	int i = 0;
	long ret = -1;

	inbuf = (unsigned char *)malloc(length);
	zbuf  = (unsigned char *)malloc(length);
	uzbuf = (unsigned char *)malloc(length);

	if (!inbuf || !zbuf || !uzbuf) {
		printf("Not enought memory (len=%ld)\n", length);
		goto err_zip;
	}

	/* Make Input Pattern */
	for (i = 0; length > i; i++)
		inbuf[i] = i%256;

	zlen = length;
	ret = compress(zbuf, (uLongf*)&zlen, inbuf, length);
	if (Z_OK != ret) {
		printf("failed, compress (ret=%ld, len=%ld) !!!\n", ret, zlen);
		goto err_zip;
	}

	uzlen = length;
	ret = uncompress(uzbuf, (uLongf*)&uzlen, zbuf, zlen);
	if (Z_OK != ret) {
		printf("failed, uncompress (ret=%ld, len=%ld) !!!\n", ret, uzlen);
		goto err_zip;
	}

	/* Compare */
	for (i = 0; length > i; i++) {
		if (uzbuf[i] != (i%256)) {
			printf("failed, compared zip/uznip data (%d, len=%ld)!!!\n", i, length);
			ret = -1;
			goto err_zip;
		}
	}

err_zip:
	if (inbuf)
		free(inbuf);

	if (zbuf)
		free(zbuf);

	if (uzbuf)
		free(uzbuf);

	return (void*)ret;
}

static void run_test_dvfs(long new_khz, long new_uV, int regulator, char *app)
{
	long cpu_khz = get_cpu_speed();
	long max_khz = get_max_speed();
	long cpu_uV  = get_cpu_voltage(regulator);
	int reg_ID = regulator;

	printf("%7ldkhz -> %7ldkhz (%7lduV -> %7lduV) ", cpu_khz, new_khz, cpu_uV, new_uV);
	if (new_khz > max_khz) {
	//	printf("skip over max (%7ldkhz)", max_khz);
		return;
	}

	/* DOWN : change first frequency */
	if (cpu_khz >= new_khz) {
		if (new_khz > 0)
			set_cpu_speed(new_khz);
		if (new_uV > 0)
			set_cpu_voltage(reg_ID, new_uV);
	} else {
	/* UP : change first voltage */
		if (new_uV > 0)
			set_cpu_voltage(reg_ID, new_uV);
		if (new_khz > 0)
			set_cpu_speed(new_khz);
	}

	/* check cpu speed */
	if (app) {
		long khz = run_test_mhz(app);
		printf(" %s = %7ld khz", app, khz);
	}

	printf("\n");
	return;
}

static int run_threads(int threads, void* (*fn)(void *), void *arg)
{
	pthread_t hthread[DEFAULT_THREAD_MAX];
	int status;
	int i = 0, ret = 0;

	if (0 >= threads || threads > DEFAULT_THREAD_MAX) {
		printf("invalid thread count %d (1 ~ %d)\n", threads, DEFAULT_THREAD_MAX);
		return -1;
	}

	for(i = 0; threads > i; i++) {
		if (0 != pthread_create(&hthread[i], NULL, fn, arg)) {
			printf("failed, thread '%d' create \n", i);
			goto err_th;
		}
	}

	for(i = 0; threads > i; i++) {
		pthread_join(hthread[i], (void **)&status);
		if (0 > status)
			ret = status;
	}

err_th:
	return ret;
}

extern int read_env(const char *str, char *rbuf);

#define	AVAIL_FREQ_SIZE		30

int main(int argc, char* argv[])
{
    int opt;
	struct freq_volt fv_tbl[AVAIL_FREQ_SIZE];
	struct freq_volt fv_val[AVAIL_FREQ_SIZE];
	char *cmd = NULL;
	long cur_khz, cur_uV;
	int size, ret, i = 0, cpus = 1;
	char margin[512];
	int re_margin = 0, env_margin = 0;

	int  md_voltage = 0;
	bool md_percent = false, md_down = false;

	bool o_random = false, o_zip = false;
	int  o_optime = 0;
	int  o_counts = 1;
	int  o_arm_vID  = DEFAULT_ARM_REGULATOR_ID;
	long o_ziplen = DEFAULT_ZIP_LENGTH_MB;
	char *o_changeV = NULL;
	int  o_read_env = 0;

    while (-1 != (opt = getopt(argc, argv, "hro:zl:v:C:T:m:d:b"))) {
		switch(opt) {
		case 'r':   o_random = true;  break;
		case 'o':   o_optime = strtol(optarg, NULL, 10);	break;
		case 'z':   o_zip = true; 							break;
		case 'l':   o_ziplen = strtol(optarg, NULL, 10);	break;
		case 'v':   o_arm_vID  = strtol(optarg, NULL, 10);	break;
		case 'd':   o_changeV  = optarg;	break;
        case 'C':   o_counts = strtol(optarg, NULL, 10);	break;
       	case 'T':   printf("-T option is not support\n");	break;
		case 'm':   cmd = optarg;						break;
		case 'b':   o_read_env = 1;						break;
        case 'h':	print_usage();  exit(0);	break;
        default:
        	break;
      	}
	}

	if(o_read_env && 0 == read_env("margin", margin)) {
		printf("Read margin...\n");
		o_changeV = margin;
		env_margin = 1;
	}

	/*
	 * Get dvfs tables
	 */
	size = get_freq_tables(fv_tbl, AVAIL_FREQ_SIZE);
	if (0 > size) {
		printf("Not support dynamic voltage frequency !!!\n");
		return -EINVAL;
	}

	cpus = get_cpu_nr();
	cur_khz = get_cpu_speed();
	cur_uV = get_cpu_voltage(o_arm_vID);

	/*
	 * Get voltage change option
	 */
	if (o_changeV) {
		char *s = strchr(o_changeV, '-');

		if (s)
			md_down = true;
		else
			s = strchr(o_changeV, '+');

		if (!s)
			s = o_changeV;
		else
			s++;

		if (strchr(o_changeV, '%'))
			md_percent = 1;

		md_voltage = strtol(s, NULL, 10);
	}

repeat_margin:
	if (re_margin) {
		if (md_percent && md_down) {
			md_down = 0, o_random = 0;
		}
	}
	printf("VOL :%s%d%s\n", md_down?"-":"+", md_voltage, md_percent?"%":"mV");

	/* Copy tables */
	memcpy(&fv_val, &fv_tbl, sizeof(fv_tbl));

	/*
	 * set new voltage (+- vol option)
	 */
	if (md_voltage) {
		for (i = 0; FREQ_ARRAY_SIZE > i; i++) {
			long step_vol = VOLTAGE_STEP_UV;
			long uV, dv, new, al = 0;

			uV  = fv_val[i].uV;
			dv  = md_percent ? ((uV/100) * md_voltage) : (md_voltage * 1000);;
			new = md_down ? uV - dv : uV + dv;

			if ((new % step_vol)) {
				new = (new / step_vol) * step_vol;
				al = 1;
				if (md_down) new += step_vol;	/* Upper */
			}
			printf("%7ldkhz, %7ld (%s%ld) align %ld (%s) -> %7ld\n",
				fv_val[i].khz, fv_val[i].uV,
				md_down?"-":"+", dv, step_vol, al?"X":"O", new);

			fv_val[i].uV = new;	/* new voltage */
		}
	}

	if (o_random)
		srand(time(NULL));

	/*
	 * DVFS TEST
	 */
	printf("RUN DVFS: cores %d EA, %s\n", cpus, o_random?"random":"sequential");
	for (i = 0; o_counts > i; i++) {
		unsigned long asv_bits = 0;
		long new_khz;
		long new_uV;
		int j = 0, n = 0, l = 0;

		for (n = 0; FREQ_ARRAY_SIZE > n; n++)
			asv_bits |= 1<<n;

		for (n = 0; FREQ_ARRAY_SIZE > n; n++) {
			/* random */
			if (o_random) {
				j = rand()%(FREQ_ARRAY_SIZE - n);
				for (l = 0; FREQ_ARRAY_SIZE > l; l++) {
					if(0 > (j-l))
						j = (FREQ_ARRAY_SIZE - 1);
					if (asv_bits & 1<<(j-l)) {
						asv_bits &= ~(1<<(j-l));
						j -= l;
						break;
					}
				}
			/* sequential */
			} else {
				j = (FREQ_ARRAY_SIZE - n - 1);
				if (re_margin) {
					j = (FREQ_ARRAY_SIZE - ((FREQ_ARRAY_SIZE/2)*n) -
						(FREQ_ARRAY_SIZE%2 ? 1 : 0));
					if (0 > j)
						break;
				}
			}

			new_khz = fv_val[j].khz;
			new_uV  = fv_val[j].uV;

			run_test_dvfs(new_khz, new_uV, o_arm_vID, cmd);
			if (o_zip) {
				long len = o_ziplen * (1024 * 1024);
				struct timeval tv1, tv2;
				long us = 0, sec;

				gettimeofday(&tv1, NULL);
				ret = run_threads(cpus, run_test_zip, (void*)&len);
				if (0 > ret)
					goto end_test;

				gettimeofday(&tv2, NULL);
				us = tv2.tv_usec - tv1.tv_usec;
				if(0 > us) {
					us += 1000000;
					tv2.tv_sec--;
				}
				sec = (tv2.tv_sec-tv1.tv_sec);
				printf("Time: %3ld.%06ld sec \n", sec, us);
			}

			if (o_optime)
				usleep(o_optime*1000);
		}
	}

	if (env_margin && md_percent && md_down &&
		(0 == re_margin)) {
		re_margin++;
		goto repeat_margin;
	}

end_test:
	printf("RESTORE : ret=%d\n", ret);
	run_test_dvfs(cur_khz, cur_uV, o_arm_vID, cmd);

	return ret;
}
