#ifndef __UDP_H
#define __UDP_H

void udp_open (void);
void udp_close (void);
void udp_send (const char *cmd);
void udp_poll (void);
int video_receive (void);
void video_receive_start (void);

#endif /* __UDP_H */

