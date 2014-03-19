#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/udp.h>

#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include "list.h"
#include "matcher.h"
#include "svmsys.h"
#include "packet.h"

extern MATCHER matcher;

void *packet_rtn(void *arg)
{
	pthread_detach(pthread_self());

	char buf[ETH_FRAME_LEN];

	while (1) {
		int ret = recvfrom(matcher.sock, buf, ETH_FRAME_LEN, 0, NULL, NULL);
		if (ret < 0) {
			perror(NULL);
			break;
		}
		if (ret == 0) {
			printf("The peer has performed an orderly shutdown!\n");
			break;
		}

		proto_header *ph = (void *)buf + sizeof(struct ether_header);
		switch (ph->type)
		{
		case 0x01:
			update_sig((void *)ph + sizeof(proto_header), ntohs(ph->len) - sizeof(proto_header));
			pthread_mutex_lock(&matcher.packet_count_lock); matcher.sig_cnt++; pthread_mutex_unlock(&matcher.packet_count_lock);
			break;
		case 0x03:
			update_voc((void *)ph + sizeof(proto_header), ntohs(ph->len) - sizeof(proto_header));
			pthread_mutex_lock(&matcher.packet_count_lock); matcher.voc_cnt++; pthread_mutex_unlock(&matcher.packet_count_lock);
			break;
		}
	}

	return NULL;
}
