/*
    Test cases:
        1. string + huge file (~83 MB)  
        2. image
        3. exe
        4. audio file
        5. video file
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>
#include <stdbool.h>

#define BUFFER_SIZE 1024

int main( int argc, char* argv[] ) {

    const char *outFilePath;
    if ( argc == 2 ) {
        outFilePath = argv[1];
    } else {
        outFilePath = "/tmp/redirectoutfile";   //default file path
    }

    // open output file
    int outFd = open(outFilePath, O_CREAT|O_WRONLY|O_APPEND, 0644 );
    if ( outFd < 0 ) {
        fprintf(stderr, "Failed to open file(%s), return status: %d(%s)\n", outFilePath, errno, strerror(errno));
        return -1;
    }

    // read fd non-blocking
    if ( fcntl( STDIN_FILENO, F_SETFL, O_NONBLOCK) < 0 ) {
        printf("Error: fcntl, return status: %d(%s)", errno, strerror(errno));
        close( outFd );
        return -1;
    }


    // setup epoll
    int epollFd = epoll_create( 1 );
    if (  epollFd < 0 ) {
        fprintf(stderr, "Error epoll_create(), return status: %d(%s)\n", errno, strerror(errno));
        close( outFd );
        return -1;
    }

    struct epoll_event event;
    memset(&event, sizeof(struct epoll_event), 0);
    event.data.fd = STDIN_FILENO;
    event.events = EPOLLIN;
    int epollRet = epoll_ctl( epollFd, EPOLL_CTL_ADD, STDIN_FILENO, &event );
    if (  epollRet < 0 ) {
        fprintf(stderr, "Error: Adding stdin fd to epoll for EPOLLIN, return status: %d(%s)\n", errno, strerror(errno));
        close( outFd );
        close( epollFd );
        return -1;
    }


    char buffer[BUFFER_SIZE];
    struct epoll_event events;
    memset(&events, sizeof(struct epoll_event), 0);

    while( 1 ) {
        int ret = epoll_wait( epollFd, &events, 1, -1 );
        if ( ret == 0 ) {
            fprintf(stdout, "Info: Epoll wait no file descriptor is ready, return status: %d(%s)\n", errno, strerror(errno));
            continue;
        } else if ( ret < 0 ) {
            if ( ret == EINTR ) {
                fprintf(stdout, "Info: Epoll wait interrupted, return status: %d(%s)\n", errno, strerror(errno));
                continue;
            } else {
                fprintf(stderr, "Error: Epoll wait error, return status: %d(%s)\n", errno, strerror(errno));
                close( outFd );
                close( epollFd );
                return -1;
            }
        } else {
            if ( events.data.fd != STDIN_FILENO ) {
                fprintf(stderr, "Error: Invalid file descriptor(%d)\n", events.data.fd );
                continue;
            }

            //read
            int bytes = fread( buffer, 1, BUFFER_SIZE, stdin );

            int offset = 0;
            //write
            while ( bytes ) {
                ret = write( outFd, buffer+offset, bytes );
                if ( ret < 0 ) {
                    if ( errno == EINTR ) {
                        fprintf(stdout, "Info: Write interrupted(%s), return status: %d(%s)\n", outFilePath, errno, strerror(errno));
                    } else {
                        fprintf(stderr, "Error: Write errors(%s), return status: %d(%s)\n", outFilePath, errno, strerror(errno));
                        close( outFd );
                        close( epollFd );
                        return -1;
                    }
                } else {
                    bytes -= ret;
                    offset += ret;
                    if ( bytes == 0 ) {
                        // fprintf(stdout, "Success: Written bytes %d to file(%s), return status: %d(%s)\n", ret, outFilePath, errno, strerror(errno));
                    } else {
                        fprintf(stdout, "Partial write %d bytes to file(%s), return status: %d(%s)\n", ret, outFilePath, errno, strerror(errno));
                    }
                }
            }

            if ( feof( stdin ) ) {
                // fprintf(stderr, "Info: EOF occured\n");
                break;
            }
        }
    }

    close( outFd );
    close( epollFd );

    return 0;
}
