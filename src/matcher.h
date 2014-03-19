/*
    matcher.h -- Application header

    Generic application symbol definations.
    All function modules should include this header.
 */

#ifndef _h_MATCHER
#define _h_MATCHER

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>

#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#include <sys/socket.h>
#include <sys/select.h>

#include <netinet/in.h>
#include <netinet/ether.h>
#include <netinet/udp.h>

#include <linux/if.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>

#include "list.h"

/* signal对象定义 */
typedef struct sigts_t {
	void *vocts;
	int8_t state; struct timeval active;
} sigts;

typedef struct sigpcm_t {
	u_int8_t pcm;
	u_int64_t tick; u_int32_t count; sigts ts_list[32];
} sigpcm;
typedef struct {
	list_node node; sigpcm *item;
} sigpcm_node_item;

typedef struct sigdpcopc_t {
	u_int8_t dpc1, dpc2, dpc3, opc1, opc2, opc3;
	u_int64_t tick; u_int32_t count; list_node pcm_list;
} sigdpcopc;
typedef struct {
	list_node node; sigdpcopc *item;
} sigdpcopc_node_item;

typedef struct svmsig_t {
	u_int64_t tick; u_int32_t count; list_node dpcopc_list;
} SIG;

/* voice对象定义 */
typedef struct vocpcm_t {
	sigpcm *signal; sigts *ts_list[32];
} vocpcm;
typedef struct {
	list_node node; vocpcm *item;
} vocpcm_node_item;

typedef struct vocdpcopc_t {
	sigdpcopc *signal; list_node pcm_list;
} vocdpcopc;
typedef struct {
	list_node node; vocdpcopc *item;
} vocdpcopc_node_item;

typedef struct vocts_t {
	u_int8_t sr155, tug3, tug2, tu12, ts;
	int8_t state; u_int64_t tick; list_node dpcopc_list;
} vocts;
typedef struct {
	list_node node; vocts *item;
} vocts_node_item;

typedef struct vocsr155_t {
	struct sockaddr_ll addr; struct ether_header head;
	u_int32_t count; list_node rec_list; vocts *ts_list[3][7][3][32], _ts_list[3][7][3][32];
} vocsr155;

typedef struct svmvoc_t {
	vocsr155 *sr155_list[8];
} VOC;


typedef struct matcher_t {
	int sock; u_char host_addr[8];

	pthread_mutex_t packet_count_lock;
	u_int64_t sig_cnt, voc_cnt, rec_cnt;

	SIG sig; pthread_mutex_t sig_lock;
	VOC voc; pthread_mutex_t voc_lock;
} MATCHER;

u_int64_t get_app_tick();
u_int16_t get_pkt_tick();

#endif /* _h_MATCHER */
