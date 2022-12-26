#include "message_slot.h"

#include <fcntl.h>       
#include <unistd.h>    
#include <sys/ioctl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char const *argv[]) {
    const char* file_path;  /*message slot file path*/
    unsigned int channel_id;  /*target message channel id*/
    const char* message;  /*the message to pass*/
    int fd;
    char* p = NULL; 
    int len = 0;

    if (argc != 4) {
        perror("Invalid number of arguments");
        exit(1);
    } 
    
    /* get CMD arguments */
    file_path = argv[1];
    channel_id = strtol(argv[2], &p, 10);
    message = argv[3];
    len = strlen(message);  

    /* Open the specified message slot device file */
    fd = open(file_path, O_RDWR);
    if (fd < 0) {
        perror("An error occurred while opening the file");
        exit(1);
    }
    /* Set the channel id to the id specified on the command line */
    if (ioctl(fd, MSG_SLOT_CHANNEL, channel_id) < -1) {
        perror("An error occurred in ioctl function");
        exit(1);
    }
    /* Write the specified message to the message slot file */
    if (write(fd, message, len) < 0) {
        perror("An error occurred while writing the message");
        exit(1);
    }
    /* Close the device */
    if (close(fd) < -1) {
        perror("An error occurred while closing file discriptor");
        exit(1);
    }
    /* Exit the program with exit value 0 */
    exit(0);
    return 0;
}
