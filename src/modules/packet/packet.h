#ifndef PACKET_H_
#define PACKET_H_

typedef struct proto_header_t
{
	u_int16_t	len;
	u_int8_t	type;
	u_int16_t	id;
	u_int8_t	slice;
} __attribute__ ((__packed__)) proto_header;

typedef struct proto_signal_t
{
	u_int16_t	len;
	u_int8_t	src;
	u_int8_t	time[8];
}  __attribute__ ((__packed__)) proto_signal;

typedef struct proto_record_t
{
	u_int8_t	server;
	u_int8_t	action;
	u_int8_t	session_id[8];
	u_int8_t	dir;
	u_int8_t	case_id;
	u_int8_t	code;
	u_int8_t	time[8];
	u_int16_t	cic;
}  __attribute__ ((__packed__)) proto_record;

typedef struct proto_voice_t
{
	u_int8_t	session_id[8]; /* 标识线路编号，依次为：SR155设备编号（1-8），STM-1编号（0，1），TUG3（1-3），TUG2（1-7），TU12（1-3），时隙编号（0-31） */
	u_int8_t	dir;
	u_int8_t	case_id;
	u_int8_t	code;
	u_int8_t	reserved1_server;
	u_int8_t	reserved2;
	u_int8_t	data_flag;
	u_int8_t	data_sn;
	u_int16_t	data_len;
	u_int8_t	data[1024];
	u_int8_t	cnt[8];
}  __attribute__ ((__packed__)) proto_voice;

void *packet_rtn(void *arg);

#endif /* PACKET_H_ */
