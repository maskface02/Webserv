#include <stdio.h>
#include <sys/socket.h>
#include <fcntl.h>
int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int flags = fcntl(sockfd, F_GETFL, 0);
    
    printf("Socket fd: %d\n", sockfd);
    printf("Default flags: %d (0x%x)\n", flags, flags);
    
    if (flags == O_RDWR)
      printf("  - O_RDWR: SET\n");
    else printf("  - O_RDWR: NOT SET\n");
    if (flags == O_NONBLOCK)
      printf("  - O_NONBLOCK: SET\n");
    else printf("  - O_NONBLOCK: NOT SET\n");
    

    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    int flags2 = fcntl(sockfd, F_GETFL, 0);
    printf("Default flags: %d (0x%x)\n", flags2, flags2);

    if (flags2 == O_RDWR)
      printf("  - O_RDWR: SET\n");
    else printf("  - O_RDWR: NOT SET\n");
    
    if (flags2 == O_NONBLOCK)
      printf("  - O_NONBLOCK: SET\n");
    else printf("  - O_NONBLOCK: NOT SET\n");
    
    return 0;
}
