/*
This test has to be run on top of linux machine.
This is to test reading arbitrary data and writing it to a file with following inputs:
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

void cleanup( FILE* popenFp, int outFd, FILE* outFp, char* output ) {
    if ( popenFp )
	    pclose( popenFp );
    if ( outFd )
        close( outFd );
    if ( outFp )
        fclose( outFp );

    if ( output )
        free( output );

    popenFp = NULL;
    outFd = -1;
    outFp = NULL;
    output = NULL;

    printf("-----------------------------------------------------------------\n");
}

char* runTest( const char* input, int size, int count ) {
    
    reset();

    FILE* popenFp = run();

    int ret = fwrite( input, size, count, popenFp );
	if ( ret < 2 ) {
		fprintf(stderr, "Error: fwrite(), return status %d(%s)\n", errno, strerror(errno) );
        pclose( popenFp );
		return NULL;
	}
    fflush( popenFp );
    sleep(1);

    FILE* outFp = fopen("/tmp/redirectoutfile", "r" );
    if ( !outFp ) {
        fprintf(stderr, "Failed to open file '/tmp/redirectoutfile', return status: %d(%s)\n", errno, strerror(errno));
        pclose( popenFp );
        return NULL;
    }
    
    char *output = (char*)malloc(size*count);
    if ( !output ) {
        fprintf(stderr, "Error: malloc failed, %d(%s)\n", errno, strerror(errno));
        exit(-1);
    }

    // struct empInfo empOut[2];
    ret = fread( output, size, count, outFp );
	if ( ret < 2 ) {
		fprintf(stderr, "Error: fread(), return status %d(%s)\n", errno, strerror(errno) );
        cleanup( popenFp, 0, outFp, NULL );
		return NULL;
	}

    cleanup( popenFp, 0, outFp, NULL );
    return output;
}

int main( void ) {

    /******************************  Test Case 1 - Input String ******************************/
    printf("Test Case 1: String ...\n");
    char input[] = "Hello World! Redirect: test newline \n test tab \t test carriage return \r\n END!";
    char* output = runTest( input, 1, sizeof(input) );
    if ( !output ) {
        fprintf(stderr, "Error: running test 1\n");
    } else {
        // compare text input and output
        printf("Given input: [%s], \nOutput: [%s]\n", input, output);
        if ( strcmp( input, output ) == 0 ) {
            printf("Test Case 1: String - PASSED\n");
        } else {
            printf("Test Case 1: String - FAILED\n");
        }
        cleanup( NULL, 0, NULL, output );
    }


    /******************************  Test Case 2 - Binary Structure Input ******************************/
    printf("Test Case 2: Binary Structure ...\n");
    // give input to redirect exe
    struct empInfo {
        char name[20];
        char location[20];
        int id;
    };
    struct empInfo emp[2];
    strcpy(emp[0].name, "John"); strcpy(emp[0].location, "Europe"); emp[0].id = 1001;
    strcpy(emp[1].name, "Marry"); strcpy(emp[1].location, "India"); emp[1].id = 2001;

    struct empInfo *empOut = ( struct empInfo* ) runTest( (char*)emp, sizeof(struct empInfo), 2 );
    if ( !empOut ) {
        fprintf(stderr, "Error: running test 2\n");
    } else {
        // compare binary structure input and output
        printf("Given input: \n");
        for (int i = 0; i < 2; i++ )
            printf("\tname [%s] location [%s] id [%d]\n", emp[i].name, emp[i].location, emp[i].id );

        printf("Output: \n");
        for (int i = 0; i < 2; i++ )
            printf("\tname [%s] location [%s] id [%d]\n", empOut[i].name, empOut[i].location, empOut[i].id );

        int n = memcmp( emp, empOut, 2*sizeof(struct empInfo) );
        if ( n == 0 ) {
            printf("Test Case 2: Binary Structure - PASSED\n");
        } else {
            printf("Test Case 2: Binary Structure - FAILED\n");
        }
        cleanup( NULL, 0, NULL, (char*)empOut );
    }


    /******************************  Test Case 3 - File Input ******************************/
    printf("Test Case 3: File ...\n");
    reset();
    int ret = system( "cat ./InputImage.jpeg | ../redirect" );
	if ( ret < 0 ) {
		fprintf(stderr, "Error: system('cat ./InputImage.jpeg | ../redirect') , return status %d(%s)\n", errno, strerror(errno) );
		exit(-1);
	}

    FILE* popenFp = popen( "diff ./InputImage.jpeg /tmp/redirectoutfile", "r" );
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

    cleanup( popenFp, 0, NULL, NULL );
	return 0;
}
