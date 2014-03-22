#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
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

u_int8_t spe_link_to_mvd_index_e1[6][21][2] =  {
	{{1, 1}, {1, 5}, {1, 9}, {1, 13}, {1, 2}, {1, 6}, {1, 10}, {1, 14}, {1, 3}, {1, 7}, {1, 11}, {1, 15}, {1, 4}, {1, 8}, {1, 12}, {1, 16}, {2, 1}, {2, 5}, {2, 9}, {2, 13}, {2, 2}},
	{{2, 4}, {2, 8}, {2, 12}, {2, 16}, {3, 1}, {3, 5}, {3, 9}, {3, 13}, {3, 2}, {3, 6}, {3, 10}, {3, 14}, {3, 3}, {3, 7}, {3, 11}, {3, 15}, {3, 4}, {3, 8}, {3, 12}, {3, 16}, {4, 1}},
	{{4, 3}, {4, 7}, {4, 11}, {4, 15}, {4, 4}, {4, 8}, {4, 12}, {4, 16}, {5, 1}, {5, 5}, {5, 9}, {5, 13}, {5, 2}, {5, 6}, {5, 10}, {5, 14}, {5, 3}, {5, 7}, {5, 11}, {5, 15}, {5, 4}},
	{{6, 2}, {6, 6}, {6, 10}, {6, 14}, {6, 3}, {6, 7}, {6, 11}, {6, 15}, {6, 4}, {6, 8}, {6, 12}, {6, 16}, {7, 1}, {7, 5}, {7, 9}, {7, 13}, {7, 2}, {7, 6}, {7, 10}, {7, 14}, {7, 3}},
	{{8, 1}, {8, 5}, {8, 9}, {8, 13}, {8, 2}, {8, 6}, {8, 10}, {8, 14}, {8, 3}, {8, 7}, {8, 11}, {8, 15}, {8, 4}, {8, 8}, {8, 12}, {8, 16}, {9, 1}, {9, 5}, {9, 9}, {9, 13}, {9, 2}},
	{{9, 4}, {9, 8}, {9, 12}, {9, 16}, {10, 1}, {10, 5}, {10, 9}, {10, 13}, {10, 2}, {10, 6}, {10, 10}, {10, 14}, {10, 3}, {10, 7}, {10, 11}, {10, 15}, {10, 4}, {10, 8}, {10, 12}, {10, 16}, {11, 1}}
};
int record(u_int8_t sr155, u_int8_t stm1, u_int8_t tug3, u_int8_t tug2, u_int8_t tu12, u_int8_t ts, u_int8_t action)
{
	if (sr155 < 1 || sr155 > 8) { return (2); }
	if (stm1 != 0 && stm1 != 1) { return (2); }
	if (tug3 < 1 || tug3 > 3) { return (2); }
	if (tug2 < 1 || tug2 > 7) { return (2); }
	if (tu12 < 1 || tu12 > 3) { return (2); }
	if (ts < 0 || ts > 31) { return (2); }
	if (action != 1 && action != 2) { return (2); }

	vocsr155 *p_vocsr155 = matcher.voc.sr155_list[sr155-1];

	int tx_len = 0;
	unsigned char sendbuf[1024] = { 0 };

	/* Ethernet header */
	*(struct ether_header *)(sendbuf + tx_len) = p_vocsr155->head;
	tx_len += sizeof(struct ether_header);

	/* Protocol header */
	proto_header header = {
			.len = htons(sizeof(proto_header) + sizeof(u_int8_t) + sizeof(proto_record)),
			.type = 0x02,
			.id = htons(get_pkt_tick()),
			.slice = 0x00
	};
	*(proto_header *)(sendbuf + tx_len) = header;
	tx_len += sizeof(proto_header);

	/* record command */
	*(u_int8_t *)(sendbuf + tx_len) = 1;
	tx_len += sizeof(u_int8_t);

	u_int8_t spe = (stm1 * 3) + tug3;
	u_int8_t link = (tu12 - 1) * 7 + tug2;

	u_int8_t mvd = spe_link_to_mvd_index_e1[spe - 1][link - 1][0];
	u_int8_t e1 = spe_link_to_mvd_index_e1[spe - 1][link - 1][1];

	proto_record record = {
			.server = 0,
			.action = action,
			.session_id = { sr155, stm1, tug3, tug2, tu12, ts, 0x00, 0x00 },
			.dir = ((stm1 == 0) ? 1 : 2),
			.case_id = 0,
			.code = 0x01,
			.time = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
			.cic = htons((mvd-1) * 0x0200 + (e1-1) * 0x20 + ts)
	};
	*(proto_record *)(sendbuf + tx_len) = record;
	tx_len += sizeof(proto_record);

	/* Send packet */
	pthread_mutex_lock(&matcher.packet_count_lock);
	if (sendto(matcher.socksig, sendbuf, tx_len, 0, (struct sockaddr*) &p_vocsr155->addr, sizeof(p_vocsr155->addr)) < 0) {
		printf("Send failed\n");
	}
	matcher.rec_cnt++;
	pthread_mutex_unlock(&matcher.packet_count_lock);

	return (0);
}

void *record_rtn(void *arg)
{
	pthread_detach(pthread_self());

	while (1) {
		sleep(60);

		pthread_mutex_lock(&matcher.voc_lock);
		u_int32_t count = 0; for(int sr155=1; (count < 8*32) && (sr155<=8); sr155++) {
			vocsr155 *p_vocsr155 = matcher.voc.sr155_list[sr155-1];
			if (!p_vocsr155) { continue; }

			//printf("checking...\n");
			for (list_node * p_node = NULL; (p_node = list_foreach(&p_vocsr155->rec_list, p_node));) {
				vocts *p_vocts = *(vocts**)node2item(p_node, vocts_node_item, node, item);

				//printf("p_vocts->state: %d, p_vocts->tick: %d, p_vocts->timeout.tv_sec: %d\n", p_vocts->state, p_vocts->tick, p_vocts->timeout.tv_sec);
				if (p_vocts->state == -1) {
					if (p_vocts->tick != 0) {
						continue;
					}
					struct timeval tv; gettimeofday(&tv, NULL);
					if (tv.tv_sec - p_vocts->timeout.tv_sec < 300) {
						continue;
					}
				}

				list_node *del_node = p_node; p_node = del_node->prev; list_rmv(del_node);
				if (p_vocts->state == -1) {
					p_vocts->state = 0;
					record(p_vocts->sr155, 0, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, 2);
					record(p_vocts->sr155, 1, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, 2);

					vocts_node_item *p_node_item = node2enty(del_node, vocts_node_item, node);
					list_add(&p_vocsr155->rec_list1, &p_node_item->node);

					for (u_int8_t sr155=1; sr155<=8; sr155++) {
						vocsr155 *p_vocsr155 = matcher.voc.sr155_list[sr155-1];
						if (!p_vocsr155) { continue; }

						vocts *p_vocts = NULL;
						for (list_node * p_node = NULL; !p_vocts && (p_node = list_get(&p_vocsr155->rec_list0));) {
							p_vocts = *(vocts**)node2item(p_node, vocts_node_item, node, item);
							//free(node2enty(p_node, vocts_node_item, node));
							vocts_node_item *p_node_item = node2enty(p_node, vocts_node_item, node);
							list_add(&p_vocsr155->rec_list, &p_node_item->node);
							break;
						}
						for (list_node * p_node = NULL; !p_vocts && (p_node = list_get(&p_vocsr155->rec_list1));) {
							p_vocts = *(vocts**)node2item(p_node, vocts_node_item, node, item);
							//free(node2enty(p_node, vocts_node_item, node));
							vocts_node_item *p_node_item = node2enty(p_node, vocts_node_item, node);
							list_add(&p_vocsr155->rec_list, &p_node_item->node);
							break;
						}
						if (p_vocts) {
							p_vocts->state = -1; p_vocts->tick = 0; gettimeofday(&p_vocts->timeout, NULL);
							record(p_vocts->sr155, 0, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, 1);
							record(p_vocts->sr155, 1, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, 1);
							break;
						}
					}
				} else {
					free(node2enty(del_node, vocts_node_item, node));
				}
			}
		}
		pthread_mutex_unlock(&matcher.voc_lock);
	}

	return NULL;
}

int init_voc(void)
{
	int sock;
	struct ifreq req_idx;

	if ((sock = socket(AF_PACKET, SOCK_RAW, htons(0x8052))) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	matcher.sockvoc = sock;
	memset(&req_idx, 0, sizeof(req_idx));
	strncpy(req_idx.ifr_name, APP_CONFIG.server.ifvoc, IFNAMSIZ - 1);
	if (ioctl(sock, SIOCGIFINDEX, &req_idx) < 0) {
		perror("SIOCGIFINDEX");
		return (2);
	}

	if ((sock = socket(AF_PACKET, SOCK_RAW, htons(0x8052))) == -1) {
		perror("socket");
		exit(EXIT_FAILURE);
	}
	matcher.socksig = sock;
	memset(&req_idx, 0, sizeof(req_idx));
	strncpy(req_idx.ifr_name, APP_CONFIG.server.ifsig, IFNAMSIZ - 1);
	if (ioctl(sock, SIOCGIFINDEX, &req_idx) < 0) {
		perror("SIOCGIFINDEX");
		return (2);
	}

	struct ifreq req_mac; memset(&req_mac, 0, sizeof(req_mac));
	strncpy(req_mac.ifr_name, APP_CONFIG.server.ifvoc, IFNAMSIZ - 1);
	if (ioctl(sock, SIOCGIFHWADDR, &req_mac) < 0) {
		perror("SIOCGIFHWADDR");
		return (2);
	}
	memcpy(matcher.host_addr, req_mac.ifr_hwaddr.sa_data, ETH_ALEN);

	for(int sr155=1; sr155<=8; sr155++) {
		if (!APP_CONFIG.device.sr155[sr155-1].present) {
			matcher.voc.sr155_list[sr155-1] = NULL;
			continue;
		}

		vocsr155 *p_vocsr155 = malloc(sizeof(vocsr155));
		{
			struct sockaddr_ll addr;
			addr.sll_ifindex = req_idx.ifr_ifindex;
			addr.sll_halen = ETH_ALEN;
			memcpy(addr.sll_addr, APP_CONFIG.device.sr155[sr155-1].mac, ETH_ALEN);
			p_vocsr155->addr = addr;

			struct ether_header head;
			memcpy(head.ether_dhost, APP_CONFIG.device.sr155[sr155-1].mac, ETH_ALEN);
			memcpy(head.ether_shost, req_mac.ifr_hwaddr.sa_data, ETH_ALEN);
			head.ether_type = htons(0x8052);
			p_vocsr155->head = head;
		}
		{
			p_vocsr155->count = 0;
			init_list(&p_vocsr155->rec_list);
			init_list(&p_vocsr155->rec_list0);
			init_list(&p_vocsr155->rec_list1);
			for(u_int8_t tug3=1; tug3<=3; tug3++) {
			for(u_int8_t tug2=1; tug2<=7; tug2++) {
			for(u_int8_t tu12=1; tu12<=3; tu12++) {
			for(u_int8_t ts=0; ts<32; ts++) {
				if (!APP_CONFIG.device.sr155[sr155-1].ts[tug3-1][tug2-1][tu12-1][ts]) {
					p_vocsr155->ts_list[tug3-1][tug2-1][tu12-1][ts] = NULL;
					continue;
				}
				vocts *p_vocts = &p_vocsr155->_ts_list[tug3-1][tug2-1][tu12-1][ts];
				p_vocts->sr155 = sr155; p_vocts->tug3 = tug3; p_vocts->tug2 = tug2; p_vocts->tu12 = tu12; p_vocts->ts = ts;
				p_vocts->state = 0; p_vocts->tick = 0; init_list(&p_vocts->dpcopc_list);

				p_vocsr155->ts_list[tug3-1][tug2-1][tu12-1][ts] = p_vocts;

				vocts_node_item *p_node_item = malloc(sizeof(vocts_node_item));
				p_node_item->item = p_vocts; list_add(&p_vocsr155->rec_list0, &p_node_item->node);
			}}}}
		}
		matcher.voc.sr155_list[sr155-1] = p_vocsr155;

		for(u_int8_t tug3=1; tug3<=3; tug3++) {
		for(u_int8_t tug2=1; tug2<=7; tug2++) {
		for(u_int8_t tu12=1; tu12<=3; tu12++) {
		for(u_int8_t ts=0; ts<32; ts++) {
			record(sr155, 0, tug3, tug2, tu12, ts, 2);
			record(sr155, 1, tug3, tug2, tu12, ts, 2);
		}}}}
	}

	sleep(1);
	u_int32_t count = 0; for(int sr155=1; (count < 8*32) && (sr155<=8); sr155++) {
		vocsr155 *p_vocsr155 = matcher.voc.sr155_list[sr155-1];
		if (!p_vocsr155) { continue; }

		for (list_node * p_node = NULL; (count < 8*32) && (p_node = list_get(&p_vocsr155->rec_list0));) {
			vocts *p_vocts = *(vocts**)node2item(p_node, vocts_node_item, node, item);
			//free(node2enty(p_node, vocts_node_item, node));
			vocts_node_item *p_node_item = node2enty(p_node, vocts_node_item, node);
			list_add(&p_vocsr155->rec_list, &p_node_item->node);

			p_vocts->state = -1; p_vocts->tick = 0; gettimeofday(&p_vocts->timeout, NULL);
			record(p_vocts->sr155, 0, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, 1);
			record(p_vocts->sr155, 1, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, 1);
			count++;
		}
	}

	sleep(1);
	pthread_t pid;
	pthread_create(&pid, NULL, record_rtn, NULL);

	return 0;
}

void initial_voice(vocts *p_vocts)
{
	/* 删除现存的结果集合 */
	for (list_node * p_node = NULL; (p_node = list_get(&p_vocts->dpcopc_list));) {
		vocdpcopc *p_vocdpcopc = *(vocdpcopc**)node2item(p_node, vocdpcopc_node_item, node, item);
		for (list_node * p_node = NULL; (p_node = list_get(&p_vocdpcopc->pcm_list));) {
			vocpcm *p_vocpcm = *(vocpcm**)node2item(p_node, vocpcm_node_item, node, item);
			free(p_vocpcm); free(node2enty(p_node, vocpcm_node_item, node));
		}
		free(p_vocdpcopc); free(node2enty(p_node, vocdpcopc_node_item, node));
	}

	pthread_mutex_lock(&matcher.sig_lock);
	p_vocts->state = -1; p_vocts->tick = get_app_tick();
	if (matcher.sig.count != 0) {
		for (list_node * p_node = NULL; (p_node = list_foreach(&matcher.sig.dpcopc_list, p_node));) {
			sigdpcopc *p_sigdpcopc = *(sigdpcopc**)node2item(p_node, sigdpcopc_node_item, node, item);
			/* count为零的（p_sigdpcopc）比较集合可以跳过遍历过程 */
			if (p_sigdpcopc->count == 0) continue;

			vocdpcopc *p_vocdpcopc = malloc(sizeof(vocdpcopc));
			p_vocdpcopc->signal = p_sigdpcopc; init_list(&p_vocdpcopc->pcm_list);
			for (list_node * p_node = NULL; (p_node = list_foreach(&p_sigdpcopc->pcm_list, p_node));) {
				sigpcm *p_sigpcm = *(sigpcm**)node2item(p_node, sigpcm_node_item, node, item);
				/* count为零的（p_sigpcm）比较集合可以跳过遍历过程 */
				if (p_sigpcm->count == 0) continue;

				vocpcm *p_vocpcm = malloc(sizeof(vocpcm));
				p_vocpcm->signal = p_sigpcm; for (int ts = 0; ts < 32; ts++) {
					if (p_vocts->ts != ts) { //TODO CIC中的时隙号必定和电路编号中的时隙号相同？
						p_vocpcm->ts_list[ts] = NULL;
						continue;
					}
					sigts *p_sigts = &p_sigpcm->ts_list[ts];
					if (p_sigts->state == 1) {
						p_vocpcm->ts_list[ts] = p_sigts;
					} else {
						p_vocpcm->ts_list[ts] = NULL;
					}
				}

				/* 检查（p_vocpcm）结果集合是否为空 */
				u_int8_t count = 0;
				for (int ts = 0; ts < 32; ts++) {
					if (p_vocpcm->ts_list[ts]) {
						count++;
					}
				}
				if (count) {
					vocpcm_node_item *p_node_item = malloc(sizeof(vocpcm_node_item));
					p_node_item->item = p_vocpcm; list_add(&p_vocdpcopc->pcm_list, &p_node_item->node);
				} else {
					free(p_vocpcm);
				}
			}

			/* 检查（p_vocdpcopc）结果集合是否为空 */
			if (!list_empty(&p_vocdpcopc->pcm_list)) {
				vocdpcopc_node_item *p_node_item = malloc(sizeof(vocdpcopc_node_item));
				p_node_item->item = p_vocdpcopc; list_add(&p_vocts->dpcopc_list, &p_node_item->node);
			} else {
				free(p_vocdpcopc);
			}
		}
	}
	pthread_mutex_unlock(&matcher.sig_lock);
}
void compare_voice(vocts *p_vocts)
{
	pthread_mutex_lock(&matcher.sig_lock);
	if (matcher.sig.tick > p_vocts->tick) {
		for (list_node *p_node = NULL; (p_node = list_foreach(&p_vocts->dpcopc_list, p_node));) {
			vocdpcopc *p_vocdpcopc = *(vocdpcopc**)node2item(p_node, vocdpcopc_node_item, node, item);
			/* 时间戳早于结果集合的（p_vocdpcopc->signal）比较集合可以跳过比较过程 */
			if (p_vocdpcopc->signal->tick < p_vocts->tick) {
				continue;
			}

			for (list_node *p_node = NULL; (p_node = list_foreach(&p_vocdpcopc->pcm_list, p_node));) {
				vocpcm *p_vocpcm = *(vocpcm**)node2item(p_node, vocpcm_node_item, node, item);
				/* 时间戳早于结果集合的（p_vocpcm->signal）比较集合可以跳过比较过程 */
				if (p_vocpcm->signal->tick < p_vocts->tick) {
					continue;
				}

				/* 从结果集合中删除可排除的单元 */
				for (int ts = 0; ts < 32; ts++) {
					if (p_vocpcm->ts_list[ts]) {
						sigts *p_sigts = p_vocpcm->ts_list[ts];
						if (p_sigts->state != 1) {
							int64_t diff_timeval_us(struct timeval *tv1, struct timeval *tv2)
							{
								return (tv2->tv_sec - tv1->tv_sec) * 1000 * 1000 + (tv2->tv_usec - tv1->tv_usec);
							}
							/* 容许一定的延迟时间（1000 * 1000us） */
							struct timeval cur; gettimeofday(&cur, NULL);
							if (diff_timeval_us(&p_sigts->active, &cur) > 1000 * 1000) {
								p_vocpcm->ts_list[ts] = NULL;
							}
						}
					}
				}

				/* 检查（p_vocpcm）并删除空的结果集合 */
				u_int8_t count = 0;
				for (int ts = 0; ts < 32; ts++) {
					if (p_vocpcm->ts_list[ts]) {
						count++;
					}
				}
				if (count == 0) {
					list_node *del_node = p_node; p_node = del_node->prev; list_rmv(del_node);
					free(p_vocpcm); free(node2enty(del_node, vocpcm_node_item, node));
				}
			}

			/* 检查（p_vocdpcopc）并删除空的结果集合 */
			if (list_empty(&p_vocdpcopc->pcm_list)) {
				list_node *del_node = p_node; p_node = del_node->prev; list_rmv(del_node);
				free(p_vocdpcopc); free(node2enty(del_node, vocdpcopc_node_item, node));
			}

		}
	}
	pthread_mutex_unlock(&matcher.sig_lock);

	/* 检查整个结果集合是否为空 */
	if (list_empty(&p_vocts->dpcopc_list)) {
		/* 初始化结果集合 */
		initial_voice(p_vocts);
	} else {
		/* 标记结果集合 */
		p_vocts->tick = get_app_tick();
	}

	/* 结果集合（dpcopc_list）是否有唯一的（vocdpcopc） */
	if (!list_alone(&p_vocts->dpcopc_list)) {
		return;
	}
	vocdpcopc *p_vocdpcopc = *(vocdpcopc**)node2item(p_vocts->dpcopc_list.next, vocdpcopc_node_item, node, item);

	/* 结果集合（pcm_list）是否有唯一的（vocpcm） */
	if (!list_alone(&p_vocdpcopc->pcm_list)) {
		return;
	}
	vocpcm *p_vocpcm = *(vocpcm**)node2item(p_vocdpcopc->pcm_list.next, vocpcm_node_item, node, item);

	/* 结果集合（ts_list）是否有唯一的（sigts） */
	sigts *p_sigts = NULL;
	u_int8_t count = 0;
	for (int ts = 0; ts < 32; ts++) {
		if (p_vocpcm->ts_list[ts]) {
			p_sigts = p_vocpcm->ts_list[ts];
			count++;
		}
	}
	if (count != 1) {
		return;
	}

	/* 设置状态为“已确定” */
	p_vocts->state = 1;

	pthread_mutex_lock(&matcher.sig_lock);
	/* 检查是否会产生覆盖 */
	if (p_sigts->vocts) {
		vocts *p_vocts = p_sigts->vocts;
		/* 重置被覆盖的项 */
		p_vocts->state = 0; p_vocts->tick = 0; for (list_node * p_node = NULL; (p_node = list_get(&p_vocts->dpcopc_list));) {
			vocdpcopc *p_vocdpcopc = *(vocdpcopc**)node2item(p_node, vocdpcopc_node_item, node, item);
			for (list_node * p_node = NULL; (p_node = list_get(&p_vocdpcopc->pcm_list));) {
				vocpcm *p_vocpcm = *(vocpcm**)node2item(p_node, vocpcm_node_item, node, item);
				free(p_vocpcm); free(node2enty(p_node, vocpcm_node_item, node));
			}
			free(p_vocdpcopc); free(node2enty(p_node, sigdpcopc_node_item, node));
		}

		vocsr155 *p_vocsr155 = matcher.voc.sr155_list[p_vocts->sr155-1]; p_vocsr155->count--;
		vocts_node_item *p_node_item = malloc(sizeof(vocts_node_item));
		p_node_item->item = p_vocts; list_add(&p_vocsr155->rec_list0, &p_node_item->node);
	}
	vocsr155 *p_vocsr155 = matcher.voc.sr155_list[p_vocts->sr155-1]; p_vocsr155->count++;

	p_sigts->vocts = p_vocts;
	pthread_mutex_unlock(&matcher.sig_lock);

	/* 关闭对应的录音，开始一些其他录音 */
	record(p_vocts->sr155, 0, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, 2);
	record(p_vocts->sr155, 1, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, 2);
	for (u_int8_t sr155=1; sr155<=8; sr155++) {
		vocsr155 *p_vocsr155 = matcher.voc.sr155_list[sr155-1];
		if (!p_vocsr155) { continue; }

		vocts *p_vocts = NULL;
		for (list_node * p_node = NULL; !p_vocts && (p_node = list_get(&p_vocsr155->rec_list0));) {
			p_vocts = *(vocts**)node2item(p_node, vocts_node_item, node, item);
			//free(node2enty(p_node, vocts_node_item, node));
			vocts_node_item *p_node_item = node2enty(p_node, vocts_node_item, node);
			list_add(&p_vocsr155->rec_list, &p_node_item->node);
			break;
		}
		for (list_node * p_node = NULL; !p_vocts && (p_node = list_get(&p_vocsr155->rec_list1));) {
			p_vocts = *(vocts**)node2item(p_node, vocts_node_item, node, item);
			//free(node2enty(p_node, vocts_node_item, node));
			vocts_node_item *p_node_item = node2enty(p_node, vocts_node_item, node);
			list_add(&p_vocsr155->rec_list, &p_node_item->node);
			break;
		}
		if (p_vocts) {
			p_vocts->state = -1; p_vocts->tick = 0; gettimeofday(&p_vocts->timeout, NULL);
			record(p_vocts->sr155, 0, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, 1);
			record(p_vocts->sr155, 1, p_vocts->tug3, p_vocts->tug2, p_vocts->tu12, p_vocts->ts, 1);
			break;
		}
	}
}
void update_voc(const u_char *packet, u_int len)
{
	proto_voice	*pv	    = (void *)packet;

	u_int8_t	sr155	= pv->session_id[0];
	u_int8_t	stm1	= pv->session_id[1];
	u_int8_t	tug3	= pv->session_id[2];
	u_int8_t	tug2	= pv->session_id[3];
	u_int8_t	tu12	= pv->session_id[4];
	u_int8_t	ts		= pv->session_id[5];
	u_int16_t	datalen	= ntohs(pv->data_len);
	u_int8_t	*data	= pv->data;
//	printf("sr155 %d, tug3 %d, tug2 %d, tu12 %d, ts %d, len %d\n", sr155, tug3, tug2, tu12, ts, datalen);
	if (sr155 < 1 || sr155 > 8) return;
	if (stm1 !=0 && stm1 != 1) return;
	if (tug3 < 1 || tug3 > 3) return;
	if (tug2 < 1 || tug2 > 7) return;
	if (tu12 < 1 || tu12 > 3) return;
	if (ts < 0 || ts > 31) return;
	if (datalen < 1 || datalen > 1024) return;

	if (!matcher.voc.sr155_list[sr155-1]) return;

	pthread_mutex_lock(&matcher.voc_lock);
	vocts *p_vocts = matcher.voc.sr155_list[sr155-1]->ts_list[tug3-1][tug2-1][tu12-1][ts];
	if (p_vocts && p_vocts->state == -1) {
		if (p_vocts->tick < matcher.sig.tick) {
			for (int i=1; i<datalen; i++) {
				if (data[0] != data[i]) {
					compare_voice(p_vocts);
					break;
				}
			}
		}
	}
	pthread_mutex_unlock(&matcher.voc_lock);
}
