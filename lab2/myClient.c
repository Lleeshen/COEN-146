 /**************************
	Lyman Shen
	Winter 2019
 **************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <limits.h>

#define BUFFER_LENGTH 10

int main (int, char *[]);


/********************
 *	main
 ********************/
int main (int argc, char *argv[])
{
	int i;
	int sockfd = 0, n = 0;
	char buff[BUFFER_LENGTH];
	char *p;
	struct sockaddr_in serv_addr;

  if (argc != 5)
	{
		printf ("Usage: %s <server port number> <ip of server> <input file> <output file> \n",argv[0]);
		return 1;
	}

  char * end = NULL;
  int errno;
  long portNumLong = strtol(argv[1],&end,10);
  if(end == argv[1]) {
    printf("Couldn't read port number\n");
    return 1;
  }
  if((portNumLong == LONG_MAX || portNumLong == LONG_MIN) && errno == ERANGE) {
    printf("Failed to get port number\n");
    return 1;  
  }
  int portNum = (int) portNumLong;
  
  FILE *iFp = fopen(argv[3],"rb");
  

  if(iFp == NULL) {
    printf("Input file failed to open\n");
    return 1;  
  }

  int numCharsRead;

	// set up
	memset (buff, '0', sizeof (buff));
	memset (&serv_addr, '0', sizeof (serv_addr)); 

	// open socket
	if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf ("Error : Could not create socket \n");
		return 1;
	} 

	// set address
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons (portNum); 

	if (inet_pton (AF_INET, argv[2], &serv_addr.sin_addr) <= 0)
	{
		printf ("inet_pton error occured\n");
		return 1;
	} 

	// connect
	if (connect (sockfd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
	{
		printf ("Error : Connect Failed \n");
		return 1;
	} 
  
  //Send output fileName to server
  write (sockfd, argv[4], strlen(argv[4]) + 1);
  //Wait for acknoledgement from server
	read (sockfd, buff, sizeof (buff));

	// input from file
	while(numCharsRead = fread(buff,1,BUFFER_LENGTH,iFp)) {
    //Send input to server
		write (sockfd, buff, numCharsRead);
	}
 

	close (sockfd);
  fclose(iFp);
	return 0;

}
