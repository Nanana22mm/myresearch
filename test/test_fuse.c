#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    int fd = open("ho4/hello-world.txt", O_RDONLY);
    printf("open the 1st file: %d\n", fd);
    int fd2 = open("ho4/dead-beef.txt", O_RDONLY);
    printf("open the 2nd file: %d\n", fd2);
    int fd3 = open("ho4/abc-def.txt", O_RDONLY);
    printf("open the 3nd file: %d\n", fd3);

    int size = 100;
    char *buf1 = (char *)calloc(size, sizeof(char));
    int len1 = read(fd, buf1, size);
    printf("read the 1st file: %s\n", buf1);

    char *buf2 = (char *)calloc(size, sizeof(char));
    int len2 = read(fd2, buf2, size);
    printf("read the 2nd file: %s\n", buf2);

    char *buf3 = (char *)calloc(size, sizeof(char));
    int len3 = read(fd3, buf3, size);
    printf("read the 3nd file: %s\n", buf3);

    close(fd);
    printf("closed the 1st file.\n");
    close(fd2);
    printf("closed the 2nd file.\n");
    close(fd3);
    printf("closed the 3rd file.\n");
}
