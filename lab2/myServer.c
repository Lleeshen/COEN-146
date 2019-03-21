 /**************************
 *     Lyman Shen
 *     Winter 2019
 ***************************/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <limits.h>

#define BUFFER_LENGTH 10
#define BUFFER_READ_LENGTH 5

int main (int, char *[]); 


/*********************
 * main
 *********************/
int main (int argc, char *argv[])
{
	int n;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr; 
	char buff[BUFFER_LENGTH];
  
  if (argc != 2)
	{
		printf ("Usage: %s <server port number>\n",argv[0]);
		return 1;
	}
  
  char * p = NULL;
  int errno;
  long portNumLong = strtol(argv[1],&p,10);
  if(p == argv[1]) {
    printf("Couldn't read port number\n");
    return 1;
  }
  if((portNumLong == LONG_MAX || portNumLong == LONG_MIN) && errno == ERANGE) {
    printf("Failed to get port number\n");
    return 1;  
  }
  int portNum = (int) portNumLong;

  FILE *oFp;

	// set up
	memset (&serv_addr, '0', sizeof (serv_addr));
	memset (buff, '0', sizeof (buff)); 

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
	serv_addr.sin_port = htons (portNum); 

	// create socket, bind, and listen
	listenfd = socket (AF_INET, SOCK_STREAM, 0);
	bind (listenfd, (struct sockaddr*)&serv_addr, sizeof (serv_addr)); 
	listen (listenfd, BUFFER_LENGTH); 

	// Accept input file and interact
	while (1)
	{
    //Accept the output file name and open file
    connfd = accept (listenfd, (struct sockaddr*)NULL, NULL);
    
    oFp = NULL;

    //Read the output file name
    if((n = read(connfd, buff, sizeof(buff))) > 0) {
      //Open file
      oFp = fopen(buff,"wb");
      //Write message to acknoledge open file
      p = buff;
      //*p++ = '0';
      *p = '\0';    
      write(connfd, buff, 3);
    }

    if(oFp == NULL) {
      printf("Output file did not open successfully\n");
      close (connfd);
      continue;
    }

		// receive data
		while ((n = read (connfd, buff, sizeof(buff))) > 0)
		{
      //Write to file
			fwrite(buff,1,n,oFp);
		}
    //Close connection and file
    close (connfd);
    fclose(oFp);
	}
  
	return 0;
}
