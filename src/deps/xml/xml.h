#ifndef _h_XML
#define _h_XML

#include "list.h"

typedef struct XML_t {
	char *name, *text;
	list_node *attrs, *nodes;
} xNODE;
typedef struct {
	list_node node; xNODE *item;
} xnode_node_item;

typedef struct ATTR_t {
	char *name, *text;
} xATTR;
typedef struct {
	list_node node; xATTR *item;
} xattr_node_item;

int destroy_xATTR(xATTR *p_xattr);
int parse_xATTR(FILE *fd, xATTR *p_xattr);
int print_xATTR(FILE *fd, xATTR *p_xattr);
list_node *parse_xATTR_list(FILE *fd);

int destroy_xNODE(xNODE *p_xnode);
int parse_xNODE(FILE *fd, xNODE *p_xnode);
int print_xNODE(FILE *fd, xNODE *p_xnode);
list_node *parse_xNODE_list(FILE *fd);

int load_xmlDocument(const char *filename, xNODE *p_xnode);
int save_xmlDocument(const char *filename, xNODE *p_xnode);

#endif /* _h_XML */
