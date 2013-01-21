#ifndef __MTSERVER_H__
#define __MTSERVER_H__

#define BUFSIZE 256
#define INVALID -1

#define CMD_UPTIME 0
#define CMD_LOAD 1
#define CMD_EXIT 2
#define CMD_INVALID -1
#define CMD_INCOMPLETE -2
#define CMD_COMPLETE 3
#define CMD_CONTROL_C 4

#define MAXREMOTEERRORS 3
// specification of behavoir is inconsistent
// so control them here
#define SENDERRORON3RD_ERROR_SEP_PACKETS 1
#define SENDERRORON3RD_ERROR_SAME_PAKCETS 1
// specification 0 wants not to see -1 on sender , just disconnect
#define SENDERRORONCONTROL_C 0

#endif
