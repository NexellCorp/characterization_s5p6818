/* *************************************************************
 *   Global Configuration
 * *************************************************************/

#define VERSION_STRING	"20151120"




/* *************************************************************
 *   GMAC Configuration
 * *************************************************************/

#define	SRC_IP_ADDR				"192.168.1.10"
#define	DST_IP_ADDR				"192.168.1.20"

#define DATA_SIZE				13
#define DATA_STRING				"Loopback Test"

#define PORT					4950

#define MAC_ADDR				"00:22:33:44:55:66"
#define INTERFACE_NAME			"eth0"

#define DEF_SPEED				100
#define DEF_REPEAT				(3)
#define PROG_NAME				"gmac_test"


/* Define some constants. */
#define IP4_HDRLEN				20	/* IPv4 header length */
#define UDP_HDRLEN				8	/* UDP header length, excludes data */

//#define USING_PROMISC			/* XXX: just for test */
#define RECV_UDP				/* FIXME: RECV_UDP is only supported */

#define GMAC_SYS_DEV			"/sys/bus/platform/devices/stmmaceth"
#define GMAC_SYS_DRV			"/sys/bus/platform/drivers/stmmaceth"
#define GMAC_SYS_PHY			"/sys/bus/mdio_bus/drivers/RTL8211E Gigabit Ethernet/stmmac-0:07"

#define PHY_SYS_REG				"/sys/devices/platform/stmmaceth/mii_phy/phyreg"
#define PHY_W					1
#define PHY_R					0
