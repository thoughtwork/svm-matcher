#ifndef _h_SVMSYS
#define _h_SVMSYS

void init_sig(void);
void update_sig(const u_char *packet, u_int len);

void init_voc(void);
void update_voc(const u_char *packet, u_int len);

#endif /* _h_SVMSYS */
