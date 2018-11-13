/* *************************************************************
 *   Global Configuration
 * *************************************************************/

#define VERSION_STRING	"20150708"




/* *************************************************************
 *   GMAC Configuration
 * *************************************************************/

#define	SRC_IP_ADDR				"192.168.1.10"
#define	DST_IP_ADDR				"192.168.1.20"

#define DATA_SIZE				13
#define DATA_STRING				"Loopback Test"

#define PORT					4950

#define MAC_ADDR				"00:50:56:c1:11:09"
#define INTERFACE_NAME			"eth0"

#define DEF_REPEAT				(1)
#define DEF_PHYID				7	/* ASB realtek phy id */
#define PROG_NAME				"gmac_test"


/* Define some constants. */
#define IP4_HDRLEN				20	/* IPv4 header length */
#define UDP_HDRLEN				8	/* UDP header length, excludes data */

//#define USING_PROMISC			/* XXX: just for test */
#define RECV_UDP				/* FIXME: RECV_UDP is only supported */

/* *************************************************************
 *   TEST Configuration
 * *************************************************************/

#define TEST_RESTRIES			7

