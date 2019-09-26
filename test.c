#include "udp.h"
#include <stdio.h>

int main (int argc, char *argv[]) {
	udp_open();
	video_init();

	printf("hello world\n");
	return 0;
}
