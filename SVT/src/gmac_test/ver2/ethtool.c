/*
 * ethtool.c: Linux ethernet device configuration tool.
 *
 * Copyright (C) 1998 David S. Miller (davem@dm.cobaltmicro.com)
 * Portions Copyright 2001 Sun Microsystems
 * Kernel 2.4 update Copyright 2001 Jeff Garzik <jgarzik@mandrakesoft.com>
 * Wake-on-LAN,natsemi,misc support by Tim Hockin <thockin@sun.com>
 * Portions Copyright 2002 Intel
 * Portions Copyright (C) Sun Microsystems 2008
 * do_test support by Eli Kupermann <eli.kupermann@intel.com>
 * ETHTOOL_PHYS_ID support by Chris Leech <christopher.leech@intel.com>
 * e1000 support by Scott Feldman <scott.feldman@intel.com>
 * e100 support by Wen Tao <wen-hwa.tao@intel.com>
 * ixgb support by Nicholas Nunley <Nicholas.d.nunley@intel.com>
 * amd8111e support by Reeja John <reeja.john@amd.com>
 * long arguments by Andi Kleen.
 * SMSC LAN911x support by Steve Glendinning <steve.glendinning@smsc.com>
 * Rx Network Flow Control configuration support <santwona.behera@sun.com>
 * Various features by Ben Hutchings <bhutchings@solarflare.com>;
 *	Copyright 2009, 2010 Solarflare Communications
 * MDI-X set support by Jesse Brandeburg <jesse.brandeburg@intel.com>
 *	Copyright 2012 Intel Corporation
 * vmxnet3 support by Shrikrishna Khare <skhare@vmware.com>
 *
 * TODO:
 *   * show settings for all devices
 */

#include "internal.h"
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stddef.h>
#include <errno.h>
#include <sys/utsname.h>
#include <limits.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/sockios.h>

#ifndef MAX_ADDR_LEN
#define MAX_ADDR_LEN	32
#endif

#define ALL_ADVERTISED_MODES			\
	(ADVERTISED_10baseT_Half |		\
	 ADVERTISED_10baseT_Full |		\
	 ADVERTISED_100baseT_Half |		\
	 ADVERTISED_100baseT_Full |		\
	 ADVERTISED_1000baseT_Half |		\
	 ADVERTISED_1000baseT_Full |		\
	 ADVERTISED_1000baseKX_Full|		\
	 ADVERTISED_2500baseX_Full |		\
	 ADVERTISED_10000baseT_Full |		\
	 ADVERTISED_10000baseKX4_Full |		\
	 ADVERTISED_10000baseKR_Full |		\
	 ADVERTISED_10000baseR_FEC |		\
	 ADVERTISED_20000baseMLD2_Full |	\
	 ADVERTISED_20000baseKR2_Full |		\
	 ADVERTISED_40000baseKR4_Full |		\
	 ADVERTISED_40000baseCR4_Full |		\
	 ADVERTISED_40000baseSR4_Full |		\
	 ADVERTISED_40000baseLR4_Full |		\
	 ADVERTISED_56000baseKR4_Full |		\
	 ADVERTISED_56000baseCR4_Full |		\
	 ADVERTISED_56000baseSR4_Full |		\
	 ADVERTISED_56000baseLR4_Full)

#define ALL_ADVERTISED_FLAGS			\
	(ADVERTISED_Autoneg |			\
	 ADVERTISED_TP |			\
	 ADVERTISED_AUI |			\
	 ADVERTISED_MII |			\
	 ADVERTISED_FIBRE |			\
	 ADVERTISED_BNC |			\
	 ADVERTISED_Pause |			\
	 ADVERTISED_Asym_Pause |		\
	 ADVERTISED_Backplane |			\
	 ALL_ADVERTISED_MODES)

#ifndef HAVE_NETIF_MSG
enum {
	NETIF_MSG_DRV		= 0x0001,
	NETIF_MSG_PROBE		= 0x0002,
	NETIF_MSG_LINK		= 0x0004,
	NETIF_MSG_TIMER		= 0x0008,
	NETIF_MSG_IFDOWN	= 0x0010,
	NETIF_MSG_IFUP		= 0x0020,
	NETIF_MSG_RX_ERR	= 0x0040,
	NETIF_MSG_TX_ERR	= 0x0080,
	NETIF_MSG_TX_QUEUED	= 0x0100,
	NETIF_MSG_INTR		= 0x0200,
	NETIF_MSG_TX_DONE	= 0x0400,
	NETIF_MSG_RX_STATUS	= 0x0800,
	NETIF_MSG_PKTDATA	= 0x1000,
	NETIF_MSG_HW		= 0x2000,
	NETIF_MSG_WOL		= 0x4000,
};
#endif

#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

static void exit_bad_args(void) __attribute__((noreturn));

static void exit_bad_args(void)
{
	fprintf(stderr,
		"ethtool: bad command line argument(s)\n"
		"For more information run ethtool -h\n");
	exit(1);
}

typedef enum {
	CMDL_NONE,
	CMDL_BOOL,
	CMDL_S32,
	CMDL_U8,
	CMDL_U16,
	CMDL_U32,
	CMDL_U64,
	CMDL_BE16,
	CMDL_IP4,
	CMDL_STR,
	CMDL_FLAG,
	CMDL_MAC,
} cmdline_type_t;

struct cmdline_info {
	const char *name;
	cmdline_type_t type;
	/* Points to int (BOOL), s32, u16, u32 (U32/FLAG/IP4), u64,
	 * char * (STR) or u8[6] (MAC).  For FLAG, the value accumulates
	 * all flags to be set. */
	void *wanted_val;
	void *ioctl_val;
	/* For FLAG, the flag value to be set/cleared */
	u32 flag_val;
	/* For FLAG, points to u32 and accumulates all flags seen.
	 * For anything else, points to int and is set if the option is
	 * seen. */
	void *seen_val;
};

struct flag_info {
	const char *name;
	u32 value;
};

static const struct flag_info flags_msglvl[] = {
	{ "drv",	NETIF_MSG_DRV },
	{ "probe",	NETIF_MSG_PROBE },
	{ "link",	NETIF_MSG_LINK },
	{ "timer",	NETIF_MSG_TIMER },
	{ "ifdown",	NETIF_MSG_IFDOWN },
	{ "ifup",	NETIF_MSG_IFUP },
	{ "rx_err",	NETIF_MSG_RX_ERR },
	{ "tx_err",	NETIF_MSG_TX_ERR },
	{ "tx_queued",	NETIF_MSG_TX_QUEUED },
	{ "intr",	NETIF_MSG_INTR },
	{ "tx_done",	NETIF_MSG_TX_DONE },
	{ "rx_status",	NETIF_MSG_RX_STATUS },
	{ "pktdata",	NETIF_MSG_PKTDATA },
	{ "hw",		NETIF_MSG_HW },
	{ "wol",	NETIF_MSG_WOL },
};

struct off_flag_def {
	const char *short_name;
	const char *long_name;
	const char *kernel_name;
	u32 get_cmd, set_cmd;
	u32 value;
	/* For features exposed through ETHTOOL_GFLAGS, the oldest
	 * kernel version for which we can trust the result.  Where
	 * the flag was added at the same time the kernel started
	 * supporting the feature, this is 0 (to allow for backports).
	 * Where the feature was supported before the flag was added,
	 * it is the version that introduced the flag.
	 */
	u32 min_kernel_ver;
};
static const struct off_flag_def off_flag_def[] = {
	{ "rx",     "rx-checksumming",		    "rx-checksum",
	  ETHTOOL_GRXCSUM, ETHTOOL_SRXCSUM, ETH_FLAG_RXCSUM,	0 },
	{ "tx",     "tx-checksumming",		    "tx-checksum-*",
	  ETHTOOL_GTXCSUM, ETHTOOL_STXCSUM, ETH_FLAG_TXCSUM,	0 },
	{ "sg",     "scatter-gather",		    "tx-scatter-gather*",
	  ETHTOOL_GSG,	   ETHTOOL_SSG,     ETH_FLAG_SG,	0 },
	{ "tso",    "tcp-segmentation-offload",	    "tx-tcp*-segmentation",
	  ETHTOOL_GTSO,	   ETHTOOL_STSO,    ETH_FLAG_TSO,	0 },
	{ "ufo",    "udp-fragmentation-offload",    "tx-udp-fragmentation",
	  ETHTOOL_GUFO,	   ETHTOOL_SUFO,    ETH_FLAG_UFO,	0 },
	{ "gso",    "generic-segmentation-offload", "tx-generic-segmentation",
	  ETHTOOL_GGSO,	   ETHTOOL_SGSO,    ETH_FLAG_GSO,	0 },
	{ "gro",    "generic-receive-offload",	    "rx-gro",
	  ETHTOOL_GGRO,	   ETHTOOL_SGRO,    ETH_FLAG_GRO,	0 },
	{ "lro",    "large-receive-offload",	    "rx-lro",
	  0,		   0,		    ETH_FLAG_LRO,
	  KERNEL_VERSION(2,6,24) },
	{ "rxvlan", "rx-vlan-offload",		    "rx-vlan-hw-parse",
	  0,		   0,		    ETH_FLAG_RXVLAN,
	  KERNEL_VERSION(2,6,37) },
	{ "txvlan", "tx-vlan-offload",		    "tx-vlan-hw-insert",
	  0,		   0,		    ETH_FLAG_TXVLAN,
	  KERNEL_VERSION(2,6,37) },
	{ "ntuple", "ntuple-filters",		    "rx-ntuple-filter",
	  0,		   0,		    ETH_FLAG_NTUPLE,	0 },
	{ "rxhash", "receive-hashing",		    "rx-hashing",
	  0,		   0,		    ETH_FLAG_RXHASH,	0 },
};

struct feature_def {
	char name[ETH_GSTRING_LEN];
	int off_flag_index; /* index in off_flag_def; negative if none match */
};

struct feature_defs {
	size_t n_features;
	/* Number of features each offload flag is associated with */
	unsigned int off_flag_matched[ARRAY_SIZE(off_flag_def)];
	/* Name and offload flag index for each feature */
	struct feature_def def[0];
};

#define FEATURE_BITS_TO_BLOCKS(n_bits)		DIV_ROUND_UP(n_bits, 32U)
#define FEATURE_WORD(blocks, index, field)	((blocks)[(index) / 32U].field)
#define FEATURE_FIELD_FLAG(index)		(1U << (index) % 32U)
#define FEATURE_BIT_SET(blocks, index, field)			\
	(FEATURE_WORD(blocks, index, field) |= FEATURE_FIELD_FLAG(index))
#define FEATURE_BIT_CLEAR(blocks, index, field)			\
	(FEATURE_WORD(blocks, index, filed) &= ~FEATURE_FIELD_FLAG(index))
#define FEATURE_BIT_IS_SET(blocks, index, field)		\
	(FEATURE_WORD(blocks, index, field) & FEATURE_FIELD_FLAG(index))

static long long
get_int_range(char *str, int base, long long min, long long max)
{
	long long v;
	char *endp;

	if (!str)
		exit_bad_args();
	errno = 0;
	v = strtoll(str, &endp, base);
	if (errno || *endp || v < min || v > max)
		exit_bad_args();
	return v;
}

static unsigned long long
get_uint_range(char *str, int base, unsigned long long max)
{
	unsigned long long v;
	char *endp;

	if (!str)
		exit_bad_args();
	errno = 0;
	v = strtoull(str, &endp, base);
	if ( errno || *endp || v > max)
		exit_bad_args();
	return v;
}

static int get_int(char *str, int base)
{
	return get_int_range(str, base, INT_MIN, INT_MAX);
}

static void get_mac_addr(char *src, unsigned char *dest)
{
	int count;
	int i;
	int buf[ETH_ALEN];

	count = sscanf(src, "%2x:%2x:%2x:%2x:%2x:%2x",
		&buf[0], &buf[1], &buf[2], &buf[3], &buf[4], &buf[5]);
	if (count != ETH_ALEN)
		exit_bad_args();

	for (i = 0; i < count; i++) {
		dest[i] = buf[i];
	}
}

static void parse_generic_cmdline(struct cmd_context *ctx,
				  int *changed,
				  struct cmdline_info *info,
				  unsigned int n_info)
{
	int argc = ctx->argc;
	char **argp = ctx->argp;
	int i, idx;
	int found;

	for (i = 0; i < argc; i++) {
		found = 0;
		for (idx = 0; idx < (int)n_info; idx++) {
			if (!strcmp(info[idx].name, argp[i])) {
				found = 1;
				*changed = 1;
				if (info[idx].type != CMDL_FLAG &&
				    info[idx].seen_val)
					*(int *)info[idx].seen_val = 1;
				i += 1;
				if (i >= argc)
					exit_bad_args();
				switch (info[idx].type) {
				case CMDL_BOOL: {
					int *p = info[idx].wanted_val;
					if (!strcmp(argp[i], "on"))
						*p = 1;
					else if (!strcmp(argp[i], "off"))
						*p = 0;
					else
						exit_bad_args();
					break;
				}
				case CMDL_S32: {
					s32 *p = info[idx].wanted_val;
					*p = get_int_range(argp[i], 0,
							   -0x80000000LL,
							   0x7fffffff);
					break;
				}
				case CMDL_U8: {
					u8 *p = info[idx].wanted_val;
					*p = get_uint_range(argp[i], 0, 0xff);
					break;
				}
				case CMDL_U16: {
					u16 *p = info[idx].wanted_val;
					*p = get_uint_range(argp[i], 0, 0xffff);
					break;
				}
				case CMDL_U32: {
					u32 *p = info[idx].wanted_val;
					*p = get_uint_range(argp[i], 0,
							    0xffffffff);
					break;
				}
				case CMDL_U64: {
					u64 *p = info[idx].wanted_val;
					*p = get_uint_range(
						argp[i], 0,
						0xffffffffffffffffLL);
					break;
				}
				case CMDL_BE16: {
					u16 *p = info[idx].wanted_val;
					*p = cpu_to_be16(
						get_uint_range(argp[i], 0,
							       0xffff));
					break;
				}
				case CMDL_IP4: {
					u32 *p = info[idx].wanted_val;
					struct in_addr in;
					if (!inet_aton(argp[i], &in))
						exit_bad_args();
					*p = in.s_addr;
					break;
				}
				case CMDL_MAC:
					get_mac_addr(argp[i],
						     info[idx].wanted_val);
					break;
				case CMDL_FLAG: {
					u32 *p;
					p = info[idx].seen_val;
					*p |= info[idx].flag_val;
					if (!strcmp(argp[i], "on")) {
						p = info[idx].wanted_val;
						*p |= info[idx].flag_val;
					} else if (strcmp(argp[i], "off")) {
						exit_bad_args();
					}
					break;
				}
				case CMDL_STR: {
					char **s = info[idx].wanted_val;
					*s = strdup(argp[i]);
					break;
				}
				default:
					exit_bad_args();
				}
				break;
			}
		}
		if( !found)
			exit_bad_args();
	}
}

static void flag_to_cmdline_info(const char *name, u32 value,
				 u32 *wanted, u32 *mask,
				 struct cmdline_info *cli)
{
	memset(cli, 0, sizeof(*cli));
	cli->name = name;
	cli->type = CMDL_FLAG;
	cli->flag_val = value;
	cli->wanted_val = wanted;
	cli->seen_val = mask;
}

static int parse_wolopts(char *optstr, u32 *data)
{
	*data = 0;
	while (*optstr) {
		switch (*optstr) {
			case 'p':
				*data |= WAKE_PHY;
				break;
			case 'u':
				*data |= WAKE_UCAST;
				break;
			case 'm':
				*data |= WAKE_MCAST;
				break;
			case 'b':
				*data |= WAKE_BCAST;
				break;
			case 'a':
				*data |= WAKE_ARP;
				break;
			case 'g':
				*data |= WAKE_MAGIC;
				break;
			case 's':
				*data |= WAKE_MAGICSECURE;
				break;
			case 'd':
				*data = 0;
				break;
			default:
				return -1;
		}
		optstr++;
	}
	return 0;
}

static int do_sset(struct cmd_context *ctx)
{
	int speed_wanted = -1;
	int duplex_wanted = -1;
	int port_wanted = -1;
	int mdix_wanted = -1;
	int autoneg_wanted = -1;
	int phyad_wanted = -1;
	int xcvr_wanted = -1;
	int full_advertising_wanted = -1;
	int advertising_wanted = -1;
	int gset_changed = 0; /* did anything in GSET change? */
	u32 wol_wanted = 0;
	int wol_change = 0;
	u8 sopass_wanted[SOPASS_MAX];
	int sopass_change = 0;
	int gwol_changed = 0; /* did anything in GWOL change? */
	int msglvl_changed = 0;
	u32 msglvl_wanted = 0;
	u32 msglvl_mask = 0;
	struct cmdline_info cmdline_msglvl[ARRAY_SIZE(flags_msglvl)];
	int argc = ctx->argc;
	char **argp = ctx->argp;
	int i;
	int err;

	for (i = 0; i < (int)ARRAY_SIZE(flags_msglvl); i++)
		flag_to_cmdline_info(flags_msglvl[i].name,
				     flags_msglvl[i].value,
				     &msglvl_wanted, &msglvl_mask,
				     &cmdline_msglvl[i]);

	for (i = 0; i < argc; i++) {
		if (!strcmp(argp[i], "speed")) {
			gset_changed = 1;
			i += 1;
			if (i >= argc)
				exit_bad_args();
			speed_wanted = get_int(argp[i],10);
		} else if (!strcmp(argp[i], "duplex")) {
			gset_changed = 1;
			i += 1;
			if (i >= argc)
				exit_bad_args();
			if (!strcmp(argp[i], "half"))
				duplex_wanted = DUPLEX_HALF;
			else if (!strcmp(argp[i], "full"))
				duplex_wanted = DUPLEX_FULL;
			else
				exit_bad_args();
		} else if (!strcmp(argp[i], "port")) {
			gset_changed = 1;
			i += 1;
			if (i >= argc)
				exit_bad_args();
			if (!strcmp(argp[i], "tp"))
				port_wanted = PORT_TP;
			else if (!strcmp(argp[i], "aui"))
				port_wanted = PORT_AUI;
			else if (!strcmp(argp[i], "bnc"))
				port_wanted = PORT_BNC;
			else if (!strcmp(argp[i], "mii"))
				port_wanted = PORT_MII;
			else if (!strcmp(argp[i], "fibre"))
				port_wanted = PORT_FIBRE;
			else
				exit_bad_args();
		} else if (!strcmp(argp[i], "mdix")) {
			gset_changed = 1;
			i += 1;
			if (i >= argc)
				exit_bad_args();
			if (!strcmp(argp[i], "auto"))
				mdix_wanted = ETH_TP_MDI_AUTO;
			else if (!strcmp(argp[i], "on"))
				mdix_wanted = ETH_TP_MDI_X;
			else if (!strcmp(argp[i], "off"))
				mdix_wanted = ETH_TP_MDI;
			else
				exit_bad_args();
		} else if (!strcmp(argp[i], "autoneg")) {
			i += 1;
			if (i >= argc)
				exit_bad_args();
			if (!strcmp(argp[i], "on")) {
				gset_changed = 1;
				autoneg_wanted = AUTONEG_ENABLE;
			} else if (!strcmp(argp[i], "off")) {
				gset_changed = 1;
				autoneg_wanted = AUTONEG_DISABLE;
			} else {
				exit_bad_args();
			}
		} else if (!strcmp(argp[i], "advertise")) {
			gset_changed = 1;
			i += 1;
			if (i >= argc)
				exit_bad_args();
			full_advertising_wanted = get_int(argp[i], 16);
		} else if (!strcmp(argp[i], "phyad")) {
			gset_changed = 1;
			i += 1;
			if (i >= argc)
				exit_bad_args();
			phyad_wanted = get_int(argp[i], 0);
		} else if (!strcmp(argp[i], "xcvr")) {
			gset_changed = 1;
			i += 1;
			if (i >= argc)
				exit_bad_args();
			if (!strcmp(argp[i], "internal"))
				xcvr_wanted = XCVR_INTERNAL;
			else if (!strcmp(argp[i], "external"))
				xcvr_wanted = XCVR_EXTERNAL;
			else
				exit_bad_args();
		} else if (!strcmp(argp[i], "wol")) {
			gwol_changed = 1;
			i++;
			if (i >= argc)
				exit_bad_args();
			if (parse_wolopts(argp[i], &wol_wanted) < 0)
				exit_bad_args();
			wol_change = 1;
		} else if (!strcmp(argp[i], "sopass")) {
			gwol_changed = 1;
			i++;
			if (i >= argc)
				exit_bad_args();
			get_mac_addr(argp[i], sopass_wanted);
			sopass_change = 1;
		} else if (!strcmp(argp[i], "msglvl")) {
			i++;
			if (i >= argc)
				exit_bad_args();
			if (isdigit((unsigned char)argp[i][0])) {
				msglvl_changed = 1;
				msglvl_mask = ~0;
				msglvl_wanted =
					get_uint_range(argp[i], 0,
						       0xffffffff);
			} else {
				ctx->argc -= i;
				ctx->argp += i;
				parse_generic_cmdline(
					ctx, &msglvl_changed,
					cmdline_msglvl,
					ARRAY_SIZE(cmdline_msglvl));
				break;
			}
		} else {
			exit_bad_args();
		}
	}

	if (full_advertising_wanted < 0) {
		/* User didn't supply a full advertisement bitfield:
		 * construct one from the specified speed and duplex.
		 */
		if (speed_wanted == SPEED_10 && duplex_wanted == DUPLEX_HALF)
			advertising_wanted = ADVERTISED_10baseT_Half;
		else if (speed_wanted == SPEED_10 &&
			 duplex_wanted == DUPLEX_FULL)
			advertising_wanted = ADVERTISED_10baseT_Full;
		else if (speed_wanted == SPEED_100 &&
			 duplex_wanted == DUPLEX_HALF)
			advertising_wanted = ADVERTISED_100baseT_Half;
		else if (speed_wanted == SPEED_100 &&
			 duplex_wanted == DUPLEX_FULL)
			advertising_wanted = ADVERTISED_100baseT_Full;
		else if (speed_wanted == SPEED_1000 &&
			 duplex_wanted == DUPLEX_HALF)
			advertising_wanted = ADVERTISED_1000baseT_Half;
		else if (speed_wanted == SPEED_1000 &&
			 duplex_wanted == DUPLEX_FULL)
			advertising_wanted = ADVERTISED_1000baseT_Full;
		else if (speed_wanted == SPEED_2500 &&
			 duplex_wanted == DUPLEX_FULL)
			advertising_wanted = ADVERTISED_2500baseX_Full;
		else if (speed_wanted == SPEED_10000 &&
			 duplex_wanted == DUPLEX_FULL)
			advertising_wanted = ADVERTISED_10000baseT_Full;
		else
			/* auto negotiate without forcing,
			 * all supported speed will be assigned below
			 */
			advertising_wanted = 0;
	}

	if (gset_changed) {
		struct ethtool_cmd ecmd;

		ecmd.cmd = ETHTOOL_GSET;
		err = send_ioctl(ctx, &ecmd);
		if (err < 0) {
			perror("Cannot get current device settings");
		} else {
			/* Change everything the user specified. */
			if (speed_wanted != -1)
				ethtool_cmd_speed_set(&ecmd, speed_wanted);
			if (duplex_wanted != -1)
				ecmd.duplex = duplex_wanted;
			if (port_wanted != -1)
				ecmd.port = port_wanted;
			if (mdix_wanted != -1) {
				/* check driver supports MDI-X */
				if (ecmd.eth_tp_mdix_ctrl != ETH_TP_MDI_INVALID)
					ecmd.eth_tp_mdix_ctrl = mdix_wanted;
				else
					fprintf(stderr, "setting MDI not supported\n");
			}
			if (autoneg_wanted != -1)
				ecmd.autoneg = autoneg_wanted;
			if (phyad_wanted != -1)
				ecmd.phy_address = phyad_wanted;
			if (xcvr_wanted != -1)
				ecmd.transceiver = xcvr_wanted;
			/* XXX If the user specified speed or duplex
			 * then we should mask the advertised modes
			 * accordingly.  For now, warn that we aren't
			 * doing that.
			 */
			if ((speed_wanted != -1 || duplex_wanted != -1) &&
			    ecmd.autoneg && advertising_wanted == 0) {
				fprintf(stderr, "Cannot advertise");
				if (speed_wanted >= 0)
					fprintf(stderr, " speed %d",
						speed_wanted);
				if (duplex_wanted >= 0)
					fprintf(stderr, " duplex %s",
						duplex_wanted ? 
						"full" : "half");
				fprintf(stderr,	"\n");
			}
			if (autoneg_wanted == AUTONEG_ENABLE &&
			    advertising_wanted == 0) {
				/* Auto negotiation enabled, but with
				 * unspecified speed and duplex: enable all
				 * supported speeds and duplexes.
				 */
				ecmd.advertising =
					(ecmd.advertising &
					 ~ALL_ADVERTISED_MODES) |
					(ALL_ADVERTISED_MODES & ecmd.supported);

				/* If driver supports unknown flags, we cannot
				 * be sure that we enable all link modes.
				 */
				if ((ecmd.supported & ALL_ADVERTISED_FLAGS) !=
				    ecmd.supported) {
					fprintf(stderr, "Driver supports one "
					        "or more unknown flags\n");
				}
			} else if (advertising_wanted > 0) {
				/* Enable all requested modes */
				ecmd.advertising =
					(ecmd.advertising &
					 ~ALL_ADVERTISED_MODES) |
					advertising_wanted;
			} else if (full_advertising_wanted > 0) {
				ecmd.advertising = full_advertising_wanted;
			}

			/* Try to perform the update. */
			ecmd.cmd = ETHTOOL_SSET;
			err = send_ioctl(ctx, &ecmd);
			if (err < 0)
				perror("Cannot set new settings");
		}
		if (err < 0) {
			if (speed_wanted != -1)
				fprintf(stderr, "  not setting speed\n");
			if (duplex_wanted != -1)
				fprintf(stderr, "  not setting duplex\n");
			if (port_wanted != -1)
				fprintf(stderr, "  not setting port\n");
			if (autoneg_wanted != -1)
				fprintf(stderr, "  not setting autoneg\n");
			if (phyad_wanted != -1)
				fprintf(stderr, "  not setting phy_address\n");
			if (xcvr_wanted != -1)
				fprintf(stderr, "  not setting transceiver\n");
			if (mdix_wanted != -1)
				fprintf(stderr, "  not setting mdix\n");
		}
	}

	if (gwol_changed) {
		struct ethtool_wolinfo wol;

		wol.cmd = ETHTOOL_GWOL;
		err = send_ioctl(ctx, &wol);
		if (err < 0) {
			perror("Cannot get current wake-on-lan settings");
		} else {
			/* Change everything the user specified. */
			if (wol_change) {
				wol.wolopts = wol_wanted;
			}
			if (sopass_change) {
				int j;
				for (j = 0; j < SOPASS_MAX; j++) {
					wol.sopass[j] = sopass_wanted[j];
				}
			}

			/* Try to perform the update. */
			wol.cmd = ETHTOOL_SWOL;
			err = send_ioctl(ctx, &wol);
			if (err < 0)
				perror("Cannot set new wake-on-lan settings");
		}
		if (err < 0) {
			if (wol_change)
				fprintf(stderr, "  not setting wol\n");
			if (sopass_change)
				fprintf(stderr, "  not setting sopass\n");
		}
	}

	if (msglvl_changed) {
		struct ethtool_value edata;

		edata.cmd = ETHTOOL_GMSGLVL;
		err = send_ioctl(ctx, &edata);
		if (err < 0) {
			perror("Cannot get msglvl");
		} else {
			edata.cmd = ETHTOOL_SMSGLVL;
			edata.data = ((edata.data & ~msglvl_mask) |
				      msglvl_wanted);
			err = send_ioctl(ctx, &edata);
			if (err < 0)
				perror("Cannot set new msglvl");
		}
	}

	return 0;
}

#ifndef TEST_ETHTOOL
int send_ioctl(struct cmd_context *ctx, void *cmd)
{
	ctx->ifr.ifr_data = cmd;
	return ioctl(ctx->fd, SIOCETHTOOL, &ctx->ifr);
}
#endif

static const struct option {
	const char *opts;
	int want_device;
	int (*func)(struct cmd_context *);
	char *help;
	char *opthelp;
} args[] = {
	{ "-s|--change", 1, do_sset, "Change generic options",
	  "		[ speed %d ]\n"
	  "		[ duplex half|full ]\n"
	  "		[ port tp|aui|bnc|mii|fibre ]\n"
	  "		[ mdix auto|on|off ]\n"
	  "		[ autoneg on|off ]\n"
	  "		[ advertise %x ]\n"
	  "		[ phyad %d ]\n"
	  "		[ xcvr internal|external ]\n"
	  "		[ wol p|u|m|b|a|g|s|d... ]\n"
	  "		[ sopass %x:%x:%x:%x:%x:%x ]\n"
	  "		[ msglvl %d | msglvl type on|off ... ]\n" },
//	{}
};

int eth_tool(int argc, char **argp)
{
	int (*func)(struct cmd_context *);
	int want_device;
	struct cmd_context ctx;
	int k;

	/* Skip command name */
	argp++;
	argc--;

	/* First argument must be either a valid option or a device
	 * name to get settings for (which we don't expect to begin
	 * with '-').
	 */
	if (argc == 0)
		exit_bad_args();
	for (k = 0; args[k].opts; k++) {
		const char *opt;
		size_t len;
		opt = args[k].opts;
		for (;;) {
			len = strcspn(opt, "|");
			if (strncmp(*argp, opt, len) == 0 &&
			    (*argp)[len] == 0) {
				argp++;
				argc--;
				func = args[k].func;
				want_device = args[k].want_device;
				goto opt_found;
			}
			if (opt[len] == 0)
				break;
			opt += len + 1;
		}
	}
	if ((*argp)[0] == '-')
		exit_bad_args();
	//func = do_gset;
	func = do_sset;
	want_device = 1;

opt_found:
	if (want_device) {
		ctx.devname = *argp++;
		argc--;

		if (ctx.devname == NULL)
			exit_bad_args();
		if (strlen(ctx.devname) >= IFNAMSIZ)
			exit_bad_args();

		/* Setup our control structures. */
		memset(&ctx.ifr, 0, sizeof(ctx.ifr));
		strcpy(ctx.ifr.ifr_name, ctx.devname);

		/* Open control socket. */
		ctx.fd = socket(AF_INET, SOCK_DGRAM, 0);
		if (ctx.fd < 0) {
			perror("Cannot get control socket");
			return 70;
		}
	} else {
		ctx.fd = -1;
	}

	ctx.argc = argc;
	ctx.argp = argp;

	return func(&ctx);
}

#ifdef STANDALONE
char *eth_argp[3][9] = {
	{ "./ethtool", "-s", "eth0", "speed", "10", "duplex", "full", "autoneg", "on" },
	{ "./ethtool", "-s", "eth0", "speed", "100", "duplex", "full", "autoneg", "on" },
	{ "./ethtool", "-s", "eth0", "speed", "1000", "duplex", "full", "autoneg", "on" },
};

int main(int argc, char *argv[])
{
	int i;
	int eth_argc;

	for (i = 0; i < argc; i++)
		printf("'%s', ", argv[i]);

	printf("\n");


	eth_argc = 9;

	eth_tool(eth_argc, eth_argp[2]);

	return 0;
}
#endif
