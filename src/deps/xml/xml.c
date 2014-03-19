#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>

#include "xml.h"
#include "mpr.h"
#include "list.h"

int destroy_xATTR(xATTR *p_xattr)
{
	assert(p_xattr);

	if (p_xattr->name) free(p_xattr->name);
	if (p_xattr->text) free(p_xattr->text);

	return 0;
}
int parse_xATTR(FILE *fd, xATTR *p_xattr)
{
	assert(fd); assert(p_xattr);

	xATTR xattr = { .name = NULL, .text = NULL };

	for (int c, cnt=0; (c=fgetc(fd)),(c!=EOF && c!='\0'); cnt++) {
		if ((char)c == ' ') {
			printf("%d Parse format error!\n", __LINE__);
			goto FAIL;
		}
		if ((char)c != '=') {
			continue;
		}
		if (cnt == 0) {
			printf("%d Parse format error!\n", __LINE__);
			goto FAIL;
		}
		fseek(fd, -1 * (cnt+1), SEEK_CUR);

		xattr.name = malloc(cnt+1);
		for (int i=0; i<cnt; i++) {
			xattr.name[i] = (char)fgetc(fd);
		} xattr.name[cnt] = '\0'; fgetc(fd);
		break;
	}
	if (!xattr.name) {
		printf("%d Parse format error!\n", __LINE__);
		goto FAIL;
	}
	if ((char)fgetc(fd) != '"') {
		printf("%d Parse format error!\n", __LINE__);
		goto FAIL;
	}
	for (int c, cnt=0; (c=fgetc(fd)),(c!=EOF && c!='\0'); cnt++) {
		if ((char)c != '"') {
			continue;
		}
		if (cnt == 0) {
			printf("%d Parse format error!\n", __LINE__);
			goto FAIL;
		}
		fseek(fd, -1 * (cnt+1), SEEK_CUR);

		xattr.text = malloc(cnt+1);
		for (int i=0; i<cnt; i++) {
			xattr.text[i] = (char)fgetc(fd);
		} xattr.text[cnt] = '\0'; fgetc(fd);
		break;
	}
	if (!xattr.text) {
		printf("%d Parse format error!\n", __LINE__);
		goto FAIL;
	}

	*p_xattr = xattr;
	return 0;
FAIL:
	destroy_xATTR(&xattr);
	return -1;
}
int print_xATTR(FILE *fd, xATTR *p_xattr)
{
	assert(fd); assert(p_xattr);

	if (!p_xattr->name || !p_xattr->text) {
		return -1;
	}
	fprintf(fd, " %s=\"%s\"", p_xattr->name, p_xattr->text);

	return 0;
}
list_node *parse_xATTR_list(FILE *fd)
{
	assert(fd);

	list_node *xattr_list = malloc(sizeof(list_node)); init_list(xattr_list);
	for (int c; (c=fgetc(fd)),(c!=EOF && c!='\0');) {
		if ((char)c == '>') {
			fseek(fd, -1, SEEK_CUR);
			break;
		}
		if ((char)c == '/') {
			fseek(fd, -1, SEEK_CUR);
			break;
		}
		if ((char)c == ' ') {
			continue;
		}

		fseek(fd, -1, SEEK_CUR);
		xATTR xattr; if (parse_xATTR(fd, &xattr) != 0) {
			printf("%d Parse format error!\n", __LINE__);
			goto FAIL;
		}
		xATTR *p_item = malloc(sizeof(xATTR)); *p_item = xattr;

		xattr_node_item *p_node_item = malloc(sizeof(xattr_node_item));
		p_node_item->item = p_item; list_add(xattr_list, &p_node_item->node);
	}

	return xattr_list;
FAIL:
	for (list_node * p_node = NULL; (p_node = list_get(xattr_list));) {
		xATTR *p_item = *(xATTR**)node2item(p_node, xattr_node_item, node, item);
		destroy_xATTR(p_item); free(p_item); free(node2enty(p_node, xattr_node_item, node));
	} free(xattr_list);
	return NULL;
}

int destroy_xNODE(xNODE *p_xnode)
{
	assert(p_xnode);

	if (p_xnode->name) free(p_xnode->name);
	if (p_xnode->text) free(p_xnode->text);

	if (p_xnode->nodes) {
		for (list_node *p_node = NULL; (p_node = list_get(p_xnode->nodes));) {
			xNODE *p_item = *(xNODE**)node2item(p_node, xnode_node_item, node, item);
			destroy_xNODE(p_item); free(p_item); free(node2enty(p_node, xnode_node_item, node));
		} free(p_xnode->nodes);
	}
	if (p_xnode->attrs) {
		for (list_node *p_node = NULL; (p_node = list_get(p_xnode->attrs));) {
			xATTR *p_item = *(xATTR**)node2item(p_node, xattr_node_item, node, item);
			destroy_xATTR(p_item); free(p_item); free(node2enty(p_node, xattr_node_item, node));
		} free(p_xnode->attrs);
	}

	return 0;
}
int parse_xNODE(FILE *fd, xNODE *p_xnode)
{
	assert(fd); assert(p_xnode);

	xNODE xnode = { .name = NULL, .text = NULL, .attrs = NULL, .nodes = NULL };

	/* 开始标签开始 */
	for (int c; (c=fgetc(fd)),(c!=EOF && c!='\0');) {
		if ((char)c == ' ' || (char)c == '\t' || (char)c == '\r' || (char)c == '\n') {
			continue;
		}
		if ((char)c != '<') {
			printf("%d Parse format error!\n", __LINE__);
			goto FAIL;
		}
		break;
	}

	/* 节点名称 */
	for (int c, cnt=0; (c=fgetc(fd)),(c!=EOF && c!='\0'); cnt++) {
		if ((char)c != ' ' && (char)c != '>') {
			continue;
		}
		if (cnt == 0) {
			printf("%d Parse format error!\n", __LINE__);
			goto FAIL;
		}
		fseek(fd, -1 * (cnt+1), SEEK_CUR);

		xnode.name = malloc(cnt+1);
		for (int i=0; i<cnt; i++) {
			xnode.name[i] = (char)fgetc(fd);
		} xnode.name[cnt] = '\0';
		break;
	}
	if (!xnode.name) {
		printf("%d Parse format error!\n", __LINE__);
		goto FAIL;
	}

	/* 节点属性 */
	xnode.attrs = parse_xATTR_list(fd);
	if (!xnode.attrs) {
		printf("%d Parse format error!\n", __LINE__);
		goto FAIL;
	}
	if (list_empty(xnode.attrs)) {
		free(xnode.attrs); xnode.attrs = NULL;
	}

	/* 开始标签结束 */
	for (int c; (c=fgetc(fd)),(c!=EOF && c!='\0');) {
		if ((char)c == ' ') {
			continue;
		}
		if ((char)c == '>') {
			break;
		}
		if ((char)c == '/') {
			if ((char) fgetc(fd) == '>') {
				goto SUCS;
			} else {
				printf("%d Parse format error!\n", __LINE__);
				goto FAIL;
			}
			break;
		}
	}

	/* 节点文本 */
	for (int c, cnt=0; (c=fgetc(fd)),(c!=EOF && c!='\0'); cnt++) {
		if ((char)c == ' ' || (char)c == '\t' || (char)c == '\r' || (char)c == '\n') {
			continue;
		}
		if ((char)c == '<') {
			fseek(fd, -1, SEEK_CUR);
			break;
		}

		fseek(fd, -1, SEEK_CUR);
		for (int cnt=0, _cnt=0; (c=fgetc(fd)),(c!=EOF && c!='\0'); cnt++) {
			if ((char)c != '<') {
				if ((char)c == ' ' || (char)c == '\t' || (char)c == '\r' || (char)c == '\n')
				{ _cnt++; } else { _cnt = 0; }
				continue;
			}
			fseek(fd, -1 * (cnt+1), SEEK_CUR);

			xnode.text = malloc(cnt-_cnt+1);
			for (int i=0; i<cnt; i++) {
				if (i<cnt-_cnt) {
					xnode.text[i] = (char)fgetc(fd);
				} else {
					fgetc(fd);
				}
			} xnode.text[cnt-_cnt] = '\0';
			break;
		}
		break;
	}

	/* 节点分支 */
	xnode.nodes = parse_xNODE_list(fd);
	if (!xnode.nodes) {
		printf("%d Parse format error!\n", __LINE__);
		goto FAIL;
	}
	if (list_empty(xnode.nodes)) {
		free(xnode.nodes); xnode.nodes = NULL;
	}

	/* 结束标签开始 */
	for (int c; (c=fgetc(fd)),(c!=EOF && c!='\0');) {
		if ((char)c == ' ' || (char)c == '\t' || (char)c == '\r' || (char)c == '\n') {
			continue;
		}
		if ((char)c != '<') {
			printf("%d Parse format error!\n", __LINE__);
			goto FAIL;
		}
		if ((char) fgetc(fd) != '/') {
			printf("%d Parse format error!\n", __LINE__);
			goto FAIL;
		}
		break;
	}

	/* 节点名称 */
	for (int c, cnt=0; (c=fgetc(fd)),(c!=EOF && c!='\0'); cnt++) {
		if ((char)c != ' ' && (char)c != '>') {
			continue;
		}
		if (cnt == 0) {
			printf("%d Parse format error!\n", __LINE__);
			goto FAIL;
		}
		fseek(fd, -1 * (cnt+1), SEEK_CUR);

		for (int i=0; i<cnt; i++) {
			if (xnode.name[i] != (char)fgetc(fd)) {
				printf("%d Parse format error!\n", __LINE__);
				goto FAIL;
			}
		}
		break;
	}
	if (!xnode.name) {
		printf("%d Parse format error!\n", __LINE__);
		goto FAIL;
	}

	/* 结束标签结束 */
	for (int c; (c=fgetc(fd)),(c!=EOF && c!='\0');) {
		if ((char)c == ' ') {
			continue;
		}
		if ((char)c != '>') {
			printf("%d Parse format error!\n", __LINE__);
			goto FAIL;
		}
		break;
	}

SUCS:
	*p_xnode = xnode;
	return 0;
FAIL:
	destroy_xNODE(&xnode);
	return -1;
}
int print_xNODE(FILE *fd, xNODE *p_xnode)
{
	assert(fd); assert(p_xnode);

	if (!p_xnode->name) {
		return -1;
	}
	fprintf(fd, "<%s", p_xnode->name);
	if (p_xnode->attrs) {
		for (list_node * p_node = NULL; p_xnode->attrs && (p_node = list_foreach(p_xnode->attrs, p_node));) {
			xATTR *p_item = *(xATTR**)node2item(p_node, xattr_node_item, node, item);
			if (print_xATTR(fd, p_item) != 0) {
				return -1;
			}
		}
	}
	if (p_xnode->text || p_xnode->nodes) {
		fprintf(fd, ">");
		if (p_xnode->text) {
			fprintf(fd, "%s", p_xnode->text);
		}
		if (p_xnode->nodes) {
			for (list_node * p_node = NULL; p_xnode->nodes && (p_node = list_foreach(p_xnode->nodes, p_node));) {
				xNODE *p_item = *(xNODE**)node2item(p_node, xnode_node_item, node, item);
				if (print_xNODE(fd, p_item) != 0) {
					return -1;
				}
			}
		}
		fprintf(fd, "</%s>", p_xnode->name);
	} else {
		fprintf(fd, " />");
	}

	return 0;
}
list_node *parse_xNODE_list(FILE *fd)
{
	assert(fd);

	list_node *xnode_list = malloc(sizeof(list_node)); init_list(xnode_list);
	for (int c, cnt=0; (c=fgetc(fd)),(c!=EOF && c!='\0'); cnt++) {
		if ((char)c != '<') {
			continue;
		}
		if ((char)fgetc(fd) == '/') {
			fseek(fd, -2, SEEK_CUR);
			break;
		} else {
			fseek(fd, -2, SEEK_CUR);
			xNODE xnode; if (parse_xNODE(fd, &xnode) != 0) {
				printf("%d Parse format error!\n", __LINE__);
				goto FAIL;
			}
			xNODE *p_node = malloc(sizeof(xNODE)); *p_node = xnode;

			xnode_node_item *p_node_item = malloc(sizeof(xnode_node_item));
			p_node_item->item = p_node; list_add(xnode_list, &p_node_item->node);
		}
	}

	return xnode_list;
FAIL:
	for (list_node * p_node = NULL; (p_node = list_get(xnode_list));) {
		xNODE *p_item = *(xNODE**)node2item(p_node, xnode_node_item, node, item);
		destroy_xNODE(p_item); free(p_item); free(node2enty(p_node, xnode_node_item, node));
	} free(xnode_list);
	return NULL;
}

int load_xmlDocument(const char *filename, xNODE *p_xnode)
{
	assert(filename); assert(p_xnode);

	FILE *fd = fopen(filename, "r");
	if (!fd) {
		printf("Cannot open file \"%s\": ", filename); perror(NULL);
		return -1;
	}

	for (int c; (c=fgetc(fd)),(c!=EOF && c!='\0');) {
		if ((char)c == '<') {
			if ((char)fgetc(fd) == '?') {
				for (int c; (c=fgetc(fd)),(c!=EOF && c!='\0');) {
					if ((char)c == '?') {
						if ((char)fgetc(fd) == '>') {
							break;
						}
					}
				}
			} else {
				fseek(fd, -2, SEEK_CUR);
				break;
			}
		}
	}

	int ret;
	xNODE xnode; if ((ret = parse_xNODE(fd, &xnode))  == 0) {
		*p_xnode = xnode;
	} fclose(fd);

	return ret;
}
int save_xmlDocument(const char *filename, xNODE *p_xnode)
{
	assert(filename); assert(p_xnode);

	FILE *fd = fopen(filename, "w");
	if (!fd) {
		printf("Cannot open file \"%s\": ", filename); perror(NULL);
		return -1;
	}

	fprintf(fd, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");
	int ret = print_xNODE(fd, p_xnode); fclose(fd);

	return ret;
}
