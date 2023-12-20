#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#define WR_VALUE _IOW('a', 'a', int32_t*)
#define RD_VALUE _IOR('a', 'b', int32_t*)

int main()
{
    int fd;
    int32_t value, number;

    fd = open("/dev/my_device", O_RDWR);
    if (fd < 0) {
        printf("Cannot open device file...\n");
        return -1;
    }

    printf("Enter value to send\n");
    scanf("%d", &number);

    printf("Writing value to driver\n");
    if (ioctl(fd, WR_VALUE, (int32_t*) &number) < 0) {
        printf("Cant write value to driver");
        return -1;
    }

    printf("Reading value from driver\n");
    if (ioctl(fd, RD_VALUE, (int32_t*) &value) < 0) {
        printf("Cant read value from driver");
        return -1;
    }

    printf("Value: %d\n", value);

    close(fd);
}
