#ifndef _h_CONFIG
#define _h_CONFIG

#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/udp.h>

#include <linux/if.h>
#include <linux/if_ether.h>

extern struct {
	struct {
		char *ifsig, *ifvoc;
	} server;
	struct {
		struct {
			u_int8_t present;
			u_int8_t mac[ETH_ALEN], ts[3][7][3][32];
		} sr155[8];
	} device;
} APP_CONFIG;

void load_config(void);

#endif /* _h_CONFIG */
