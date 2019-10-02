#include "udp.h"
#include <stdio.h>
#include <Windows.h>

int main (int argc, char *argv[]) {
	udp_open();
	video_init();

	udp_send("command");
	printf("Sent command\n");
	Sleep(1000);

	udp_send("streamon");
	printf("Sent streamon\n");
	Sleep(1000);

	printf("Start video\n");
	while(1) {
		if (video_receive() < 0) {
			break;
		}
	}

	return 0;
}
