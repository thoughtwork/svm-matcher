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
#include "svmsys.h"

MATCHER matcher;

u_int64_t get_app_tick()
{
	static u_int64_t tick = 0;
	return ++tick;
}
u_int16_t get_pkt_tick()
{
	static u_int16_t tick = 0;
	return ++tick;
}

void list_dpcopc(void)
{
	u_int8_t id = 0;
	pthread_mutex_lock(&matcher.sig_lock);
	for (list_node * p_node = NULL; (p_node = list_foreach(&matcher.sig.dpcopc_list, p_node));) {
		sigdpcopc *p_sigdpcopc = *(sigdpcopc**)node2item(p_node, sigdpcopc_node_item, node, item);

		printf("%4d    %d.%d.%d %d.%d.%d\n", ++id,
				p_sigdpcopc->dpc1, p_sigdpcopc->dpc2, p_sigdpcopc->dpc3,
				p_sigdpcopc->opc1, p_sigdpcopc->opc2, p_sigdpcopc->opc3);
	}
	pthread_mutex_unlock(&matcher.sig_lock);
}
void show_dpcopc(u_int8_t id)
{
	sigdpcopc *p_sigdpcopc = NULL;

	pthread_mutex_lock(&matcher.sig_lock);
	for (list_node * p_node = NULL; (id>=1) && (p_node = list_foreach(&matcher.sig.dpcopc_list, p_node));) {
		if (id != 1) { id--; continue; }
		p_sigdpcopc = *(sigdpcopc**)node2item(p_node, sigdpcopc_node_item, node, item);
	}
	if (p_sigdpcopc) {
		u_int32_t matched_cnt = 0, session_cnt = 0;

		int8_t sigts_list[128][32];
		for(u_int8_t pcm=0; pcm<128; pcm++) {
			for(u_int8_t ts=0; ts<32; ts++) { sigts_list[pcm][ts] = 0; }
		}
		for (list_node * p_node = NULL; (p_node = list_foreach(&p_sigdpcopc->pcm_list, p_node));) {
			sigpcm *p_sigpcm = *(sigpcm**)node2item(p_node, sigpcm_node_item, node, item);
			for (int ts = 0; ts < 32; ts++) {
				sigts *p_sigts = &p_sigpcm->ts_list[ts];
				if (!p_sigts->vocts) {
					sigts_list[p_sigpcm->pcm][ts] = p_sigts->state;
				} else {
					sigts_list[p_sigpcm->pcm][ts] = p_sigts->state * 10;
					matched_cnt++;
				}
				if (p_sigts->state == 1) {
					session_cnt++;
				}
			}
		}
		printf("\033[0;0H\033[2J");
		printf("PCM 0-127:\n");
		for(u_int8_t pcm=0; pcm<128; pcm++) {
			for(u_int8_t ts=0; ts<32; ts++) {
				switch(sigts_list[pcm][ts])
				{
				case 1:
					printf("\033[31m@");
					break;
				case -1:
					printf("\033[31m-");
					break;
				case 10:
					printf("\033[32m@");
					break;
				case -10:
					printf("\033[32m-");
					break;
				case 0:
					printf("\033[37m-");
					break;
				default:
					printf(" ");
					break;
				}
			}
			printf("\033[0m   ");
			if ((pcm+1)%4 == 0) {
				printf("\n");
			}
		}
		printf("matched_cnt: %d, session_cnt: %d\n", matched_cnt, session_cnt);
	} else {
		printf("dpcopc with specified id not found!\n");
	}
	pthread_mutex_unlock(&matcher.sig_lock);
}
void info_dpcopc(u_int8_t id)
{
	sigdpcopc *p_sigdpcopc = NULL;

	pthread_mutex_lock(&matcher.sig_lock);
	for (list_node * p_node = NULL; (id>=1) && (p_node = list_foreach(&matcher.sig.dpcopc_list, p_node));) {
		if (id != 1) { id--; continue; }
		p_sigdpcopc = *(sigdpcopc**)node2item(p_node, sigdpcopc_node_item, node, item);
	}
	if (p_sigdpcopc) {
		for (list_node * p_node = NULL; (p_node = list_foreach(&p_sigdpcopc->pcm_list, p_node));) {
			sigpcm *p_sigpcm = *(sigpcm**)node2item(p_node, sigpcm_node_item, node, item);
			for (int ts = 0; ts < 32; ts++) {
				sigts *p_sigts = &p_sigpcm->ts_list[ts];
				vocts *p_vocts = p_sigts->vocts;
				if (p_vocts) {
					printf("%d.%d.%d-%d.%d.%d %d.%d -> %d.%d.%d.%d.%d\n",
							p_sigdpcopc->dpc1, p_sigdpcopc->dpc2, p_sigdpcopc->dpc3, p_sigdpcopc->opc1, p_sigdpcopc->opc2, p_sigdpcopc->opc3, p_sigpcm->pcm, ts, p_vocts->sr155, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts);
				}
			}
		}
	} else {
		printf("dpcopc with specified id not found!\n");
	}
	pthread_mutex_unlock(&matcher.sig_lock);
}
void save_dpcopc(u_int8_t id)
{
	char filename[256] = { '\0' };
	snprintf(filename, 255, "dpcopc_%hhu.info", id);

	FILE *fd = fopen(filename, "w");
	if (!fd) {
		printf("Cannot open file \"%s\": ", filename); perror(NULL);
		return;
	}

	sigdpcopc *p_sigdpcopc = NULL;
	pthread_mutex_lock(&matcher.sig_lock);
	for (list_node * p_node = NULL; (id>=1) && (p_node = list_foreach(&matcher.sig.dpcopc_list, p_node));) {
		if (id != 1) { id--; continue; }
		p_sigdpcopc = *(sigdpcopc**)node2item(p_node, sigdpcopc_node_item, node, item);
	}
	if (p_sigdpcopc) {
		for (list_node * p_node = NULL; (p_node = list_foreach(&p_sigdpcopc->pcm_list, p_node));) {
			sigpcm *p_sigpcm = *(sigpcm**)node2item(p_node, sigpcm_node_item, node, item);
			for (int ts = 0; ts < 32; ts++) {
				sigts *p_sigts = &p_sigpcm->ts_list[ts];
				vocts *p_vocts = p_sigts->vocts;
				if (p_vocts) {
					fprintf(fd, "%d.%d.%d-%d.%d.%d %d.%d -> %d.%d.%d.%d.%d\n",
							p_sigdpcopc->dpc1, p_sigdpcopc->dpc2, p_sigdpcopc->dpc3, p_sigdpcopc->opc1, p_sigdpcopc->opc2, p_sigdpcopc->opc3, p_sigpcm->pcm, ts, p_vocts->sr155, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts);
				}
			}
		}
	} else {
		printf("dpcopc with specified id not found!\n");
	}
	pthread_mutex_unlock(&matcher.sig_lock);

	fclose(fd);
}

void list_device(void)
{
	u_int8_t id = 0;
	pthread_mutex_lock(&matcher.voc_lock);
	while (id < 8) {
		vocsr155 *p_vocsr155 = matcher.voc.sr155_list[id];
		if (!p_vocsr155) { ++id; continue; }

		printf("%4d    %02X:%02X:%02X:%02X:%02X:%02X\n", ++id,
				p_vocsr155->addr.sll_addr[0], p_vocsr155->addr.sll_addr[1], p_vocsr155->addr.sll_addr[2],
				p_vocsr155->addr.sll_addr[3], p_vocsr155->addr.sll_addr[4], p_vocsr155->addr.sll_addr[5]);
	}
	pthread_mutex_unlock(&matcher.voc_lock);
}
void show_device(u_int8_t id)
{
	vocsr155 *p_vocsr155 = NULL;

	pthread_mutex_lock(&matcher.voc_lock);
	if (id >= 1 && id <= 8) {
		p_vocsr155 = matcher.voc.sr155_list[id-1];
	}
	if (p_vocsr155) {
		printf("\033[0;0H\033[2J");
		u_int32_t matched_cnt = 0, monitor_cnt = 0;
		for (u_int8_t tug3=1; tug3<=3; tug3++) {
			printf("TUG3 %d, TUG2 1-7, TU12 1-3:\n", tug3);
			for (u_int8_t tug2=1; tug2<=7; tug2++) {
				for (u_int8_t tu12=1; tu12<=3; tu12++) {
					for (u_int8_t ts=0; ts<32; ts++) {
						vocts *p_vocts = p_vocsr155->ts_list[tug3-1][tug2-1][tu12-1][ts];
						if (p_vocts) {
							switch (p_vocts->state)
							{
							case 1:
								printf("\033[32m-");
								matched_cnt++;
								break;
							case -1:
								printf("\033[31m@");
								monitor_cnt++;
								break;
							case 0:
								printf("\033[31m-");
								break;
							}
						} else {
							printf("\033[37m-");
						}
					}
					printf("\033[0m   ");
				}
				printf("\n");
			}
			printf("\n");
		}
		printf("matched_cnt: %d, monitor_cnt: %d\n", matched_cnt, monitor_cnt);
	} else {
		printf("device with specified id not found!\n");
	}
	pthread_mutex_unlock(&matcher.voc_lock);
}
void info_device(u_int8_t id)
{
	vocsr155 *p_vocsr155 = NULL;

	pthread_mutex_lock(&matcher.voc_lock);
	if (id >= 1 && id <= 8) {
		p_vocsr155 = matcher.voc.sr155_list[id-1];
	}
	if (p_vocsr155) {
		for (u_int8_t tug3=1; tug3<=3; tug3++) {
		for (u_int8_t tug2=1; tug2<=7; tug2++) {
		for (u_int8_t tu12=1; tu12<=3; tu12++) {
		for (u_int8_t ts=0; ts<32; ts++) {
			vocts *p_vocts = p_vocsr155->ts_list[tug3-1][tug2-1][tu12-1][ts];
			if (p_vocts && (p_vocts->state == 1)) {
				if (!list_alone(&p_vocts->dpcopc_list)) {
					continue;
				}
				vocdpcopc *p_vocdpcopc = *(vocdpcopc**)node2item(p_vocts->dpcopc_list.next, vocdpcopc_node_item, node, item);

				if (!list_alone(&p_vocdpcopc->pcm_list)) {
					continue;
				}
				vocpcm *p_vocpcm = *(vocpcm**)node2item(p_vocdpcopc->pcm_list.next, vocpcm_node_item, node, item);

				for (u_int8_t ts = 0; ts < 32; ts++) {
					if (p_vocpcm->ts_list[ts]) {
						sigdpcopc *p_sigdpcopc = p_vocdpcopc->signal; sigpcm *p_sigpcm = p_vocpcm->signal;
						printf("%d.%d.%d.%d.%d -> %d.%d.%d-%d.%d.%d %d.%d\n",
								p_vocts->sr155, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, p_sigdpcopc->dpc1, p_sigdpcopc->dpc2, p_sigdpcopc->dpc3, p_sigdpcopc->opc1, p_sigdpcopc->opc2, p_sigdpcopc->opc3, p_sigpcm->pcm, ts);
						break;
					}
				}
			}
		}}}}
	} else {
		printf("device with specified id not found!\n");
	}
	pthread_mutex_unlock(&matcher.voc_lock);
}
void save_device(u_int8_t id)
{
	char filename[256] = { '\0' };
	snprintf(filename, 255, "device_%hhu.info", id);

	FILE *fd = fopen(filename, "w");
	if (!fd) {
		printf("Cannot open file \"%s\": ", filename); perror(NULL);
		return;
	}

	vocsr155 *p_vocsr155 = NULL;
	pthread_mutex_lock(&matcher.voc_lock);
	if (id >= 1 && id <= 8) {
		p_vocsr155 = matcher.voc.sr155_list[id-1];
	}
	if (p_vocsr155) {
		for (u_int8_t tug3=1; tug3<=3; tug3++) {
		for (u_int8_t tug2=1; tug2<=7; tug2++) {
		for (u_int8_t tu12=1; tu12<=3; tu12++) {
		for (u_int8_t ts=0; ts<32; ts++) {
			vocts *p_vocts = p_vocsr155->ts_list[tug3-1][tug2-1][tu12-1][ts];
			if (p_vocts && (p_vocts->state == 1)) {
				if (!list_alone(&p_vocts->dpcopc_list)) {
					continue;
				}
				vocdpcopc *p_vocdpcopc = *(vocdpcopc**)node2item(p_vocts->dpcopc_list.next, vocdpcopc_node_item, node, item);

				if (!list_alone(&p_vocdpcopc->pcm_list)) {
					continue;
				}
				vocpcm *p_vocpcm = *(vocpcm**)node2item(p_vocdpcopc->pcm_list.next, vocpcm_node_item, node, item);

				for (u_int8_t ts = 0; ts < 32; ts++) {
					if (p_vocpcm->ts_list[ts]) {
						sigdpcopc *p_sigdpcopc = p_vocdpcopc->signal; sigpcm *p_sigpcm = p_vocpcm->signal;
						fprintf(fd, "%d.%d.%d.%d.%d -> %d.%d.%d-%d.%d.%d %d.%d\n",
								p_vocts->sr155, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, p_sigdpcopc->dpc1, p_sigdpcopc->dpc2, p_sigdpcopc->dpc3, p_sigdpcopc->opc1, p_sigdpcopc->opc2, p_sigdpcopc->opc3, p_sigpcm->pcm, ts);
						break;
					}
				}
			}
		}}}}
	} else {
		printf("device with specified id not found!\n");
	}
	pthread_mutex_unlock(&matcher.voc_lock);

	fclose(fd);
}

void show_packet()
{
	pthread_mutex_lock(&matcher.packet_count_lock);
	printf("pkt_sig_cnt: %llu, pkt_rec_cnt: %llu, pkt_voc_cnt: %llu\n", matcher.sig_cnt, matcher.rec_cnt, matcher.voc_cnt);
	pthread_mutex_unlock(&matcher.packet_count_lock);
}

void show_help()
{
	printf("version 1.3\n");

	printf("list dpcopc         list dpcopc index\n");
	printf("show dpcopc <id>    show specified dpcopc\n");
	printf("info dpcopc <id>    show detailed information of specified dpcopc\n");
	printf("save dpcopc <id>    save detailed information of specified dpcopc\n");

	printf("list device         list dpcopc index\n");
	printf("show device <id>    show specified dpcopc\n");
	printf("info device <id>    show detailed information of specified dpcopc\n");
	printf("save device <id>    save detailed information of specified dpcopc\n");

	printf("show packet         show packet statistics\n");

	printf("quit                exit program\n");
}

int main(int argc, char **argv)
{
	load_config(); init_sig(); init_voc();

	pthread_t pid;
	pthread_create(&pid, NULL, packet_rtn, NULL);

	for (char cmd[64]; printf("command>"),fgets(cmd, 64, stdin);) {
		if (!strcmp(cmd, "list dpcopc\n")) {
			list_dpcopc();
			continue;
		}
		u_int8_t dpcopc_id;
		if (sscanf(cmd, "show dpcopc %hhu\n", &dpcopc_id) == 1) {
			show_dpcopc(dpcopc_id);
			continue;
		}
		if (sscanf(cmd, "info dpcopc %hhu\n", &dpcopc_id) == 1) {
			info_dpcopc(dpcopc_id);
			continue;
		}
		if (sscanf(cmd, "save dpcopc %hhu\n", &dpcopc_id) == 1) {
			save_dpcopc(dpcopc_id);
			continue;
		}

		if (!strcmp(cmd, "list device\n")) {
			list_device();
			continue;
		}
		u_int8_t device_id;
		if (sscanf(cmd, "show device %hhu\n", &device_id) == 1) {
			show_device(device_id);
			continue;
		}
		if (sscanf(cmd, "info device %hhu\n", &device_id) == 1) {
			info_device(device_id);
			continue;
		}
		if (sscanf(cmd, "save device %hhu\n", &device_id) == 1) {
			save_device(device_id);
			continue;
		}

		if (!strcmp(cmd, "show packet\n")) {
			show_packet();
			continue;
		}

		if (!strcmp(cmd, "quit\n")) {
			break;
		}

		show_help();
	}

	return (0);
}
