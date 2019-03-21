/**************************
 *  UDP reliable transfer server
 *  Lyman Shen
 *  Winter 2019
 *
 *  Uncomment lines with fprintf for status messages
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
#include <limits.h>
#include <time.h>
#include "tfv2.h"
#define RAND_MOD 8

/*********************
 * main
 *********************/
int main(int argc, char **argv) {
  int sock;
  int bytes_read;
  struct sockaddr_in server_addr , client_addr;
  socklen_t addr_len;
  srand(time(NULL));

  //Right number of arguments
  if (argc != 2) {
		fprintf(stderr,"Usage: %s <server port number>\n",argv[0]);
		exit(EXIT_FAILURE);
	}
  
  //Get port number safely
  char * p = NULL;
  int errno;
  long portNumLong = strtol(argv[1],&p,10);
  if(p == argv[1]) {
    fprintf(stderr,"Couldn't read port number\n");
    exit(EXIT_FAILURE);
  }
  if((portNumLong == LONG_MAX || portNumLong == LONG_MIN) && errno == ERANGE) {
    fprintf(stderr,"Failed to get port number\n");
    exit(EXIT_FAILURE);  
  }
  int portNum = (int) portNumLong;

  //Open socket
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("Socket");
    exit(EXIT_FAILURE);
  }
  // Set address
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portNum);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  //Bind socket to address
  if (bind(sock,(struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    perror("Bind");
    exit(EXIT_FAILURE);
  }
  addr_len = sizeof(struct sockaddr);
	
  //Set up necessary variables for rft 2.2
  PACKET * packet_ptr = malloc(sizeof(PACKET));
  int seq = 0;
  int cSum;
  int noData = 0;
  FILE *oFp = NULL;
  int randNum;

  //Set up constants
  int pSize = sizeof(PACKET);
  int addrSize = sizeof(struct sockaddr);
  int headerSize = sizeof(HEADER);

  while(recvfrom(sock,packet_ptr,sizeof(PACKET),0,(struct sockaddr *) &client_addr,&addrSize) > 0) {
    //Received EOF Packet    
    if(packet_ptr->header.length == 0) {
      fprintf(stderr,"EOF Packet\tSeq %d\n",seq);
      fclose(oFp);
      //Send ACK with sequence number
      packet_ptr->header.seq_ack = seq;
      packet_ptr->header.length = 0;
      packet_ptr->header.checksum = 0;
      packet_ptr->header.checksum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
      randNum = rand() % RAND_MOD;
      if(randNum != 0)
      sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&client_addr, addrSize);
      break;
    }
    //Calculate checksum
    cSum = packet_ptr->header.checksum;
    packet_ptr->header.checksum = 0;
    packet_ptr->header.checksum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
    //Send bad ACK if wrong sequence number or checksum
    if(seq != packet_ptr->header.seq_ack || cSum != packet_ptr->header.checksum) {
      fprintf(stderr,"Checksum %d %d Seq %d %d\n",cSum, packet_ptr->header.checksum, seq, packet_ptr->header.seq_ack);
      packet_ptr->header.seq_ack = seq;
      packet_ptr->header.checksum = 0;
      packet_ptr->header.checksum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
      randNum = rand() % RAND_MOD;
      if(randNum != 0)
      sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&client_addr, addrSize);
      continue;
    }
    randNum = rand() % RAND_MOD;
    printf("Randnum %d\n",randNum);
    if(randNum != 0) {
      //Send good acknowledge, update sequence number
      packet_ptr->header.seq_ack = seq;
      packet_ptr->header.checksum = 0;
      packet_ptr->header.checksum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
      sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&client_addr, addrSize);
      //Write to file
      if(oFp != NULL) {
        fwrite(packet_ptr->data,1,packet_ptr->header.length,oFp);
        fprintf(stderr,"Wrote to file\tSeq %d\n",seq);
      } else {
        //Open the file if not yet open
        oFp = fopen(packet_ptr->data,"wb");
        fprintf(stderr,"Opened file\tSeq %d\n",seq);
      }
      seq = (seq == 0) ? 1 : 0;
    }
  } 
  close(sock);
  exit(EXIT_SUCCESS);
}
