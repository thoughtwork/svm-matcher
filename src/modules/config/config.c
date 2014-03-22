#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
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
#include "xml.h"
#include "matcher.h"

struct {
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

int parseMAC(char *str, u_int8_t *buf)
{
	if (strlen(str) != ETH_ALEN*3 - 1) return -1;
	for (int i=0; i<ETH_ALEN; i++) {
		buf[i] = 0;
		if (str[i*3] >= '0' && str[i*3] <= '9') {
			buf[i] += (str[i*3] - '0' + 0x00) * 0x10;
		}
		if (str[i*3+1] >= '0' && str[i*3+1] <= '9') {
			buf[i] += (str[i*3+1] - '0' + 0x00) * 0x01;
		}

		if (str[i*3] >= 'a' && str[i*3] <= 'z') {
			buf[i] += (str[i*3] - 'a' + 0x0a) * 0x10;
		}
		if (str[i*3+1] >= 'a' && str[i*3+1] <= 'z') {
			buf[i] += (str[i*3+1] - 'a' + 0x0a) * 0x01;
		}

		if (str[i*3] >= 'A' && str[i*3] <= 'Z') {
			buf[i] += (str[i*3] - 'A' + 0x0A) * 0x10;
		}
		if (str[i*3+1] >= 'A' && str[i*3+1] <= 'Z') {
			buf[i] += (str[i*3+1] - 'A' + 0x0A) * 0x01;
		}
	}

	return 0;
}

void load_config(void)
{
	xNODE _config, *config = &_config;
	if (load_xmlDocument("config.xml", &_config) != 0) {
		printf("Fail to load configuration!\n");
		exit(1);
	}

	memset(&APP_CONFIG, 0, sizeof(APP_CONFIG));
	for (list_node * p_node = NULL; config->nodes && (p_node = list_foreach(config->nodes, p_node));) {
		xNODE *pitem = *(xNODE**)node2item(p_node, xnode_node_item, node, item);
		if (pitem->name && !strcmp(pitem->name, "server")) {
			xNODE *p_sever = pitem;
			for (list_node * p_node = NULL; p_sever->nodes && (p_node = list_foreach(p_sever->nodes, p_node));) {
				xNODE *pitem = *(xNODE**)node2item(p_node, xnode_node_item, node, item);
				if (pitem->name && !strcmp(pitem->name, "ifsig")) {
					xNODE *p_ifsig = pitem;
					if (p_ifsig->text) {
						APP_CONFIG.server.ifsig = strdup(p_ifsig->text);
					}
				}
				if (pitem->name && !strcmp(pitem->name, "ifvoc")) {
					xNODE *p_ifvoc = pitem;
					if (p_ifvoc->text) {
						APP_CONFIG.server.ifvoc = strdup(p_ifvoc->text);
					}
				}
			}
		}
		if (pitem->name && !strcmp(pitem->name, "device")) {
			xNODE *p_device = pitem;
			for (list_node * p_node = NULL; p_device->nodes && (p_node = list_foreach(p_device->nodes, p_node));) {
				xNODE *pitem = *(xNODE**)node2item(p_node, xnode_node_item, node, item);

				if (pitem->name && !strcmp(pitem->name, "sr155")) {
				u_int8_t sr155; u_int8_t mac[ETH_ALEN];
				xNODE *p_sr155 = pitem; char *sr155_id = NULL, *sr155_mac = NULL;
				for (list_node * p_node = NULL; p_sr155->attrs && (p_node = list_foreach(p_sr155->attrs, p_node));) {
					xATTR *pitem = *(xATTR**)node2item(p_node, xattr_node_item, node, item);
					if (pitem->name && !strcmp(pitem->name, "id")) { sr155_id = pitem->text; }
					if (pitem->name && !strcmp(pitem->name, "mac")) { sr155_mac = pitem->text; }
				}
				if (!sr155_id || ((sr155 = atoi(sr155_id)), (sr155 < 1 || sr155 > 8))) { continue; }
				if (!sr155_mac || parseMAC(sr155_mac, mac) != 0) { continue; }
				APP_CONFIG.device.sr155[sr155-1].present = 1; memcpy(APP_CONFIG.device.sr155[sr155-1].mac, mac, ETH_ALEN);
				for (list_node * p_node = NULL; p_sr155->nodes && (p_node = list_foreach(p_sr155->nodes, p_node));) {
				xNODE *pitem = *(xNODE**)node2item(p_node, xnode_node_item, node, item);

				if (pitem->name && !strcmp(pitem->name, "tug3")) {
				u_int8_t tug3; xNODE *p_tug3 = pitem; char *tug3_id = NULL;
				for (list_node * p_node = NULL; p_tug3->attrs && (p_node = list_foreach(p_tug3->attrs, p_node));) {
					xATTR *pitem = *(xATTR**)node2item(p_node, xattr_node_item, node, item);
					if (pitem->name && !strcmp(pitem->name, "id")) { tug3_id = pitem->text; }
				}
				if (!tug3_id || ((tug3 = atoi(tug3_id)), (tug3 < 1 || tug3 > 3))) { continue; }
				for (list_node * p_node = NULL; p_tug3->nodes && (p_node = list_foreach(p_tug3->nodes, p_node));) {
				xNODE *pitem = *(xNODE**)node2item(p_node, xnode_node_item, node, item);

				if (pitem->name && !strcmp(pitem->name, "tug2")) {
				u_int8_t tug2; xNODE *p_tug2 = pitem; char *tug2_id = NULL;
				for (list_node * p_node = NULL; p_tug2->attrs && (p_node = list_foreach(p_tug2->attrs, p_node));) {
					xATTR *pitem = *(xATTR**)node2item(p_node, xattr_node_item, node, item);
					if (pitem->name && !strcmp(pitem->name, "id")) { tug2_id = pitem->text; }
				}
				if (!tug2_id || ((tug2 = atoi(tug2_id)), (tug2 < 1 || tug2 > 7))) { continue; }
				for (list_node * p_node = NULL; p_tug2->nodes && (p_node = list_foreach(p_tug2->nodes, p_node));) {
				xNODE *pitem = *(xNODE**)node2item(p_node, xnode_node_item, node, item);

				if (pitem->name && !strcmp(pitem->name, "tu12")) {
				u_int8_t tu12; xNODE *p_tu12 = pitem; char *tu12_id = NULL;
				for (list_node * p_node = NULL; p_tu12->attrs && (p_node = list_foreach(p_tu12->attrs, p_node));) {
					xATTR *pitem = *(xATTR**)node2item(p_node, xattr_node_item, node, item);
					if (pitem->name && !strcmp(pitem->name, "id")) { tu12_id = pitem->text; }
				}
				if (!tu12_id || ((tu12 = atoi(tu12_id)), (tu12 < 1 || tu12 > 7))) { continue; }

				if (p_tu12->text && strlen(p_tu12->text) == 63) {
					for (u_int8_t ts=0; ts<32; ts++) {
						if (p_tu12->text[ts*2] == '1') {
							APP_CONFIG.device.sr155[sr155-1].ts[tug3-1][tug2-1][tu12-1][ts] = 1;
						}
					}
				}}}}}}}}
			}
		}
	}

	destroy_xNODE(config);
}
