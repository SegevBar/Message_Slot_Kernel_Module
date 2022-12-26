#include "message_slot.h"

#include <fcntl.h>     
#include <unistd.h>     
#include <sys/ioctl.h>  
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char const *argv[]) {
    const char* file_path;
    unsigned int channel_id;
    char message[BUF_LEN];
    int fd;
    char* p = NULL; 
    int length = 0;

    if (argc != 3) {
        perror("Invalid number of arguments");
        exit(1);
    }

    file_path = argv[1];
    channel_id = strtol(argv[2], &ptr, 10);

    fd = open(file_path, O_RDWR);
    if (fd < 0) {
        perror("An error occurred while opening the file");
        exit(1);
    }
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < -1) {
        perror("An error occurred in ioctl function");
        exit(1);
    }
    length = read(fd, message, BUF_LEN);
    if (length < 0) {
        perror("An error occurred while reading the message");
        exit(1);
    }
    if (close(fd) < -1) {
        perror("An error occurred while closing file discriptor");
        exit(1);
    }
    if (write(1, message, length) < 0) {
        perror("An error occurred while writing the message");
        exit(1);
    }
    exit(0);
    return 0;
}
