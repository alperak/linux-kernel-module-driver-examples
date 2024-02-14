#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <signal.h>

#define SIGTX 60
#define REGISTER_USERAPP _IO('R', 'g')

void SignalHandler(int sig)
{
	printf("Button pressed!\n");
}

int main() {
	int fd;
	signal(SIGTX, SignalHandler);

	printf("PID: %d\n", getpid());

	/* Open the device file */
	fd = open("/dev/gpio_irq_signal", O_RDONLY);
	if(fd < 0) {
		perror("Could not open device file");
		return -1;
	}

	/* Register app to kernel module */
	if(ioctl(fd, REGISTER_USERAPP, NULL)) {
		perror("Error registering app");
		close(fd);
		return -1;
	}

	/* Wait for Signal */
	printf("Wait for signal...\n");
	while(1)
		sleep(1);

	return 0;
}

