#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/times.h>

#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/udp.h>

#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include "list.h"

#include "matcher.h"
#include "packet.h"
#include "config.h"

extern MATCHER matcher;

void init_sig(void)
{
	matcher.sig.tick = get_app_tick(); matcher.sig.count = 0; init_list(&matcher.sig.dpcopc_list);
}

void update_sig(const u_char *packet, u_int len)
{
	u_int8_t		*cnt	= (void *)packet;

	for (proto_signal *ps = (void *)cnt + sizeof(u_int8_t);
		(*cnt)--; ps = (void *)ps + sizeof(proto_signal) + ntohs(ps->len)) {

		u_int8_t *data = (void *)ps + sizeof(proto_signal);

		u_int8_t dpc1, dpc2, dpc3, opc1, opc2, opc3, pcm, ts, code;
		switch (data[3])
		{
		case 0x05:
		{
			u_int32_t	SLS_DPC_OPC	=	data[4] + 0x100 * data[5] + 0x10000 * data[6] + 0x1000000 * data[7];
			u_int16_t	CIC			=	data[8] + 0x100 * data[9]; /* 共12位，低5位代表时隙号，高7位代表PCM系统号 */

			dpc1 = SLS_DPC_OPC>>25 & 0x07,	dpc2 = SLS_DPC_OPC>>17 & 0xff,	dpc3 = SLS_DPC_OPC>>14 & 0x07;
			opc1 = SLS_DPC_OPC>>11 & 0x07,	opc2 = SLS_DPC_OPC>>3 & 0xff,	opc3 = SLS_DPC_OPC>>0 & 0x07;
			pcm  = CIC>>5 & 0x7f, ts = CIC>>0 & 0x1f;
			code = data[10];
		}
			break;
		case 0x85:
			dpc1 = data[6];
			dpc2 = data[5];
			dpc3 = data[4];
			opc1 = data[9];
			opc2 = data[8];
			opc3 = data[7];
			pcm  = data[12];
			ts   = data[11];
			code = data[13];
			break;
		default:
			continue;
			break;
		}

		pthread_mutex_lock(&matcher.sig_lock);
		{
			sigdpcopc *p_sigdpcopc = NULL;
			for (list_node * p_node = NULL; (p_node = list_foreach(&matcher.sig.dpcopc_list, p_node));) {
				sigdpcopc *item = *(sigdpcopc**)node2item(p_node, sigdpcopc_node_item, node, item);
				if (item->dpc1 == dpc1 && item->dpc2 == dpc2 && item->dpc3 == dpc3 &&
					item->opc1 == opc1 && item->opc2 == opc2 && item->opc3 == opc3) {
					p_sigdpcopc = item; break;
				}
				if (item->dpc1 == opc1 && item->dpc2 == opc2 && item->dpc3 == opc3 &&
					item->opc1 == dpc1 && item->opc2 == dpc2 && item->opc3 == dpc3) {
					p_sigdpcopc = item; break;
				}
			}
			if (!p_sigdpcopc) {
				p_sigdpcopc = malloc(sizeof(sigdpcopc));
				p_sigdpcopc->dpc1 = dpc1; p_sigdpcopc->dpc2 = dpc2; p_sigdpcopc->dpc3 = dpc3;
				p_sigdpcopc->opc1 = opc1; p_sigdpcopc->opc2 = opc2; p_sigdpcopc->opc3 = opc3;
				p_sigdpcopc->tick = 0; p_sigdpcopc->count = 0; init_list(&p_sigdpcopc->pcm_list);

				sigdpcopc_node_item *p_node_item = malloc(sizeof(sigdpcopc_node_item));
				p_node_item->item = p_sigdpcopc; list_add(&matcher.sig.dpcopc_list, &p_node_item->node);
			}

			sigpcm *p_sigpcm = NULL;
			for (list_node * p_node = NULL; (p_node = list_foreach(&p_sigdpcopc->pcm_list, p_node));) {
				sigpcm *item = *(sigpcm**)node2item(p_node, sigpcm_node_item, node, item);
				if (item->pcm == pcm) {
					p_sigpcm = item; break;
				}
			}
			if (!p_sigpcm) {
				p_sigpcm = malloc(sizeof(sigpcm));
				p_sigpcm->pcm = pcm;
				p_sigpcm->tick = 0; p_sigpcm->count = 0; for (int ts=0; ts<32; ts++) {
					sigts *p_sigts = &p_sigpcm->ts_list[ts];
					p_sigts->vocts = NULL;
					p_sigts->state = 0; p_sigts->active.tv_sec = p_sigts->active.tv_usec = 0;
				}

				sigpcm_node_item *p_node_item = malloc(sizeof(sigpcm_node_item));
				p_node_item->item = p_sigpcm; list_add(&p_sigdpcopc->pcm_list, &p_node_item->node);
			}

			sigts *p_sigts = &p_sigpcm->ts_list[ts];
			switch (p_sigts->state)
			{
			case 1: //通话状态
				if (code == 0x0c) { //REL
					p_sigts->state = -1; gettimeofday(&p_sigts->active, NULL);
					p_sigpcm->tick = p_sigdpcopc->tick  = matcher.sig.tick = get_app_tick();
					p_sigpcm->count--; p_sigdpcopc->count--; matcher.sig.count--;
				}
				break;
			case -1: //闲置状态
				if (code == 0x09) { //ANM
					p_sigts->state = 1;
					p_sigpcm->count++; p_sigdpcopc->count++; matcher.sig.count++;
				}
				break;
			case 0: //未知状态
				if (code == 0x0c) { //REL
					p_sigts->state = -1;
				}
				if (code == 0x09) { //ANM
					p_sigts->state = 1;
					p_sigpcm->count++; p_sigdpcopc->count++; matcher.sig.count++;
				}
				break;
			}
		}
		pthread_mutex_unlock(&matcher.sig_lock);
	}
}
