/*
This is to test redirect with following inputs:
	1. string
    2. binary structure
	3. file
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

void reset() {
    int systemRet = system("> /tmp/redirectoutfile");
    if ( systemRet < 0 ) {
        fprintf(stderr, "Error: cleaning up existing out file, return status %d(%s)\n", errno, strerror(errno));
    }
}

FILE* run() {
    FILE* popenFp = popen( "../redirect", "w" );
	if ( !popenFp ) {
		fprintf(stderr, "Error: popen('../redirect') , return status %d(%s)\n", errno, strerror(errno) );
		exit(-1);
	}
    return popenFp;
}

void cleanup( FILE* popenFp, int outFd, FILE* outFp ) {
    if ( popenFp )
	    pclose( popenFp );
    if ( outFd )
        close( outFd );
    if ( outFp )
        fclose( outFp );

    popenFp = NULL;
    outFd = -1;
    outFp = NULL;

    printf("-----------------------------------------------------------------\n");
}

int main( void ) {

    reset();
    /******************************  Test Case 1 - Input String ******************************/
    printf("Test Case 1: String ...\n");
	FILE* popenFp = run();

    // give input to exe
    char input[] = "Hello World! Redirect: test newline \n test tab \t test carriage return \r\n END!";
	int ret = write( fileno( popenFp ), input, sizeof(input) );
	if ( ret < sizeof(input) ) {
		fprintf(stderr, "Error: write(), return status %d(%s)\n", errno, strerror(errno) );
        pclose( popenFp );
		return -1;
	}

    sleep(1);
    int outFd = open("/tmp/redirectoutfile", O_RDONLY, 0644 );
    if ( outFd < 0 ) {
        fprintf(stderr, "Failed to open file '/tmp/redirectoutfile', return status: %d(%s)\n", errno, strerror(errno));
        pclose( popenFp );
        return -1;
    }
    
    char output[sizeof(input)] = {0};
    ret = read( outFd, output, sizeof(input) );
	if ( ret < sizeof(input) ) {
		fprintf(stderr, "Error: read(), return status %d(%s)\n", errno, strerror(errno) );
        pclose( popenFp );
        close(outFd);
		return -1;
	}

    // compare text input and output
    printf("Given input: [%s], \nOutput: [%s]\n", input, output);
    if ( strcmp( input, output ) == 0 ) {
        printf("Test Case 1: String - PASSED\n");
    } else {
        printf("Test Case 1: String - FAILED\n");
    }
    cleanup( popenFp, outFd, NULL );



    /******************************  Test Case 2 - Binary Structure Input ******************************/
    printf("Test Case 2: Binary Structure ...\n");
    reset();
	popenFp = run();

    // give input to redirect exe
    struct empInfo {
        char name[20];
        char location[20];
        int id;
    };
    struct empInfo emp[2];
    strcpy(emp[0].name, "John"); strcpy(emp[0].location, "Europe"); emp[0].id = 1001;
    strcpy(emp[1].name, "Marry"); strcpy(emp[1].location, "India"); emp[1].id = 2001;

	ret = fwrite( emp, sizeof(struct empInfo), 2, popenFp );
	if ( ret < 2 ) {
		fprintf(stderr, "Error: fwrite(), return status %d(%s)\n", errno, strerror(errno) );
        pclose( popenFp );
		return -1;
	}
    fflush( popenFp );
    sleep(1);

    FILE* outFp = fopen("/tmp/redirectoutfile", "r" );
    if ( !outFp ) {
        fprintf(stderr, "Failed to open file '/tmp/redirectoutfile', return status: %d(%s)\n", errno, strerror(errno));
        pclose( popenFp );
        return -1;
    }
    
    struct empInfo empOut[2];
    ret = fread( empOut, sizeof(struct empInfo), 2, outFp );
	if ( ret < 2 ) {
		fprintf(stderr, "Error: fread(), return status %d(%s)\n", errno, strerror(errno) );
        pclose( popenFp );
        fclose( outFp );
		return -1;
	}

    // compare binary structure input and output
    printf("Given input: \n");
    for (int i = 0; i < 2; i++ )
        printf("\tname [%s] location [%s] id [%d]\n", emp[i].name, emp[i].location, emp[i].id );

    printf("Output: \n");
    for (int i = 0; i < 2; i++ )
        printf("\tname [%s] location [%s] id [%d]\n", empOut[i].name, empOut[i].location, empOut[i].id );

    int n = memcmp( &emp, &empOut, 2*sizeof(struct empInfo) );
    if ( n == 0 ) {
        printf("Test Case 2: Binary Structure - PASSED\n");
    } else {
        printf("Test Case 2: Binary Structure - FAILED\n");
    }
    cleanup( popenFp, 0, outFp );


    /******************************  Test Case 3 - File Input ******************************/
    printf("Test Case 3: File ...\n");
    reset();
    ret = system( "cat ./InputImage.jpeg | ../redirect" );
	if ( ret < 0 ) {
		fprintf(stderr, "Error: system('cat ./InputImage.jpeg | ../redirect') , return status %d(%s)\n", errno, strerror(errno) );
		exit(-1);
	}

    popenFp = popen( "diff ./InputImage.jpeg /tmp/redirectoutfile", "r" );
    if ( !popenFp ) {
        fprintf(stderr, "Error: popen('diff ./InputImage.jpeg /tmp/redirectoutfile'), return status %d(%s) \n", errno, strerror(errno));
        exit(-1);
    }
    
    int status = 0;
    printf("Given input: [./InputImage.jpeg], \nRedirected output: [/tmp/redirectoutfile]\n");
    read( fileno(popenFp), &status, sizeof(int) );
     if ( status == 0 ) {
        printf("Test Case 3: File 'Matched' - PASSED\n");
    } else {
        printf("Test Case 3: File 'Not Matched' - FAILED\n");
    }

    cleanup( popenFp, 0, NULL );
	return 0;
}
