/**************************
 *  UDP reliable transfer client
 *  Lyman Shen
 *  Winter 2019
 *
 *  Uncomment fprintf statements for status messages
 *  Uncomment lines with randNum to test errors
 **************************/

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <limits.h>
#include <time.h>
#include "tfv2.h"
#define BUFFER_LENGTH 10
#define RAND_MOD 8

/********************
 * main
 ********************/
int main(int argc, char **argv) {
  int sock;
  struct sockaddr_in server_addr;
  struct hostent *host;
  socklen_t addr_len;
  srand(time(NULL));
  //Usage of function
  if (argc != 5) {
		fprintf (stderr,"Usage: %s <server port number> <ip of server> <input file> <output file> \n",argv[0]);
		exit(EXIT_FAILURE);
	}

  //Get port number safely
  char * end = NULL;
  int errno;
  long portNumLong = strtol(argv[1],&end,10);
  if(end == argv[1]) {
    fprintf(stderr,"Couldn't read port number\n");
    exit(EXIT_FAILURE);
  }
  if((portNumLong == LONG_MAX || portNumLong == LONG_MIN) && errno == ERANGE) {
    fprintf(stderr,"Failed to get port number\n");
    exit(EXIT_FAILURE);  
  }
  int portNum = (int) portNumLong;

  host= (struct hostent *) gethostbyname(argv[2]);
  
  // open socket
  if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  //Set up fcntl
  fcntl(sock,F_SETFL,O_NONBLOCK);

  // set address
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portNum);
  server_addr.sin_addr = *((struct in_addr *)host->h_addr);
 
  //Set up input file
  FILE *iFp = fopen(argv[3],"rb");
  if(iFp == NULL) {
    printf("Input file failed to open\n");
    return 1;  
  }

  //Set up necessary variables for rft 3.0
  PACKET * packet_ptr = malloc(sizeof(PACKET));
  int seq;
  int cSum;
  int numCharsRead;
  int resent = 0;
  int randNum;
  char buff[BUFFER_LENGTH];
  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  int rv;
  fd_set readfds;
  int err = 1;

  //Set up constants
  int pSize = sizeof(PACKET);
  int addrSize = sizeof(struct sockaddr);
  int headerSize = sizeof(HEADER);

  //Send the filename
  seq = 0;
  //Prepare packet information
  packet_ptr->header.seq_ack = seq;                 //Sequence number set
  packet_ptr->header.length = strlen(argv[4]) + 1;  //Length of file name set
  memcpy(packet_ptr->data,argv[4],10);              //Data set to file name
  //Calculate Checksum
  packet_ptr->header.checksum = 0;                                          //Set checksum to 0
  cSum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);  //Calculate checksum
  randNum = rand() % RAND_MOD;
  //if(randNum != 0)
  packet_ptr->header.checksum = cSum;                                       //Set checksum
  //Send packet
  sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&server_addr, addrSize);
  fprintf(stderr,"Sent packet fname\tSeq %d\n",seq);
  resent = 0;
  //Make sure good ACK
  while(1) {
    FD_ZERO(&readfds);
    FD_SET(sock,&readfds);
    rv = select(sock + 1, &readfds, NULL, NULL, &tv);
    if(rv == -1) { //Error
      exit(EXIT_FAILURE);
    } else if (rv == 0) { //Timeout
      fprintf(stderr,"Packet fname loss\n");
      if(resent > 2)
        exit(EXIT_FAILURE);
      //Remake the packet and send if not 3 straight bad ACKs
      ++resent;
      packet_ptr->header.seq_ack = seq;
      memcpy(packet_ptr->data,argv[4],10);
      packet_ptr->header.length = strlen(argv[4]) + 1;
      packet_ptr->header.checksum = 0;
      cSum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
      randNum = rand() % RAND_MOD;
      //if(randNum != 0)
      packet_ptr->header.checksum = cSum; 
      sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&server_addr, addrSize); //Resend packet with right information 
      fprintf(stderr,"Resent fname packet\tSeq %d\n",seq);
      continue;
    } else {  //Received value
      numCharsRead = recvfrom(sock,packet_ptr,sizeof(PACKET),0,(struct sockaddr *) &server_addr, &addrSize);
      if(numCharsRead > 0) {
        //Wait for good ACK
        cSum = packet_ptr->header.checksum;
        packet_ptr->header.checksum = 0;
        packet_ptr->header.checksum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
        if(seq != packet_ptr->header.seq_ack 
          //|| cSum != packet_ptr->header.checksum
          ) {
          fprintf(stderr,"Bad ACK fname seq %d %d cSum %d %d \n",seq, packet_ptr->header.seq_ack, cSum, packet_ptr->header.checksum);
          //Exit if 3 straight bad ACKs
          if(resent > 2) {
            exit(EXIT_FAILURE);
          } else {
            //Remake the packet and send if not 3 straight bad ACKs
            ++resent;
            packet_ptr->header.seq_ack = seq;
            memcpy(packet_ptr->data,argv[4],10);
            packet_ptr->header.length = strlen(argv[4]) + 1;
            packet_ptr->header.checksum = 0;
            cSum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
            randNum = rand() % RAND_MOD;
            //if(randNum != 0)
            packet_ptr->header.checksum = cSum; 
            sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&server_addr, addrSize); //Resend packet with right information 
            fprintf(stderr,"Resent fname packet\tSeq %d\n",seq);
            continue;
          }
        }
        //Good ACK, change seq number and reset bad ACK
        resent = 0; //Reset the bad ACK counter
        fprintf(stderr,"Good ACK fname\n");
        break;
      }
    }
  }
  seq = 1; //Update sequence number

  
  //Read from file and send
  //Keep sending while there are still input
  while(1) {
    //Get new data from file and send packet if previous is successful
    if(err == 1) {
      numCharsRead = fread(buff,1,BUFFER_LENGTH,iFp);
      if(numCharsRead == 0)
        break;
      //Prepare packet information
      packet_ptr->header.seq_ack = seq;
      packet_ptr->header.length = numCharsRead;
      memcpy(packet_ptr->data,buff,numCharsRead);
      //Calculate checksum
      packet_ptr->header.checksum = 0;
      cSum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
      randNum = rand() % RAND_MOD;
      //if(randNum != 0)
      packet_ptr->header.checksum = cSum;
      //Send packet
      sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&server_addr, addrSize);  //Send packet with new data
      fprintf(stderr,"Sent packet fdata\tseq %d\n",seq);    
    }
    //Make sure good ACK
    while(1) {
      FD_ZERO(&readfds);
      FD_SET(sock,&readfds);
      rv = select(sock + 1, &readfds, NULL, NULL, &tv);
      if(rv == -1) { //Error
        exit(EXIT_FAILURE);
      } else if (rv == 0) { //Timeout
        fprintf(stderr,"Packet loss fdata\n");
        if(resent > 2)
          exit(EXIT_FAILURE);
        //Remake the packet and send if not 3 straight bad ACKs
        ++resent;
        //Prepare packet information
        packet_ptr->header.seq_ack = seq;
        packet_ptr->header.length = numCharsRead;
        memcpy(packet_ptr->data,buff,numCharsRead);
        //Calculate checksum
        packet_ptr->header.checksum = 0;
        cSum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
        randNum = rand() % RAND_MOD;
        //if(randNum != 0)
        packet_ptr->header.checksum = cSum;
        sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&server_addr, addrSize); //Resend packet with right information 
        fprintf(stderr,"Resent packet fdata\tseq %d\n",seq);
        err = 0;
        continue;
      } else {  //Received value
        numCharsRead = recvfrom(sock,packet_ptr,sizeof(PACKET),0,(struct sockaddr *) &server_addr, &addrSize);
        if(numCharsRead > 0) {
          //Wait for good ACK
          cSum = packet_ptr->header.checksum;
          packet_ptr->header.checksum = 0;
          packet_ptr->header.checksum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
          if(seq != packet_ptr->header.seq_ack 
            //|| cSum != packet_ptr->header.checksum
            ) {
            fprintf(stderr,"Bad ACK fdata seq %d %d cSum %d %d \n",seq, packet_ptr->header.seq_ack, cSum, packet_ptr->header.checksum);
            //Exit if 3 straight bad ACKs
            if(resent > 2) {
              exit(EXIT_FAILURE);
            } else {
              //Remake the packet and send if not 3 straight bad ACKs
              ++resent;
              packet_ptr->header.seq_ack = seq;
              memcpy(packet_ptr->data,buff,numCharsRead);
              packet_ptr->header.length = numCharsRead;
              packet_ptr->header.checksum = 0;
              cSum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
              randNum = rand() % RAND_MOD;
              //if(randNum != 0)
              packet_ptr->header.checksum = cSum; 
              sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&server_addr, addrSize); //Resend packet with right information 
              fprintf(stderr,"Resent packet fdata\tseq %d\n",seq);
              err = 0;
              continue;
            }
          }
          //Good ACK, change seq number and reset bad ACK
          resent = 0; //Reset the bad ACK counter
          fprintf(stderr,"Good ACK fdata\n");
          seq = (seq == 0) ? 1 : 0; //Update sequence number
          err = 1;
          break;
        }
      }
    }

  }

  //Send EOF packet
  //Prepare packet
  packet_ptr->header.seq_ack = seq;
  packet_ptr->header.length = 0;
  memset(packet_ptr->data,0,10);
  //Calculate checksum
  packet_ptr->header.checksum = 0;
  cSum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
  randNum = rand() % RAND_MOD;
  //if(randNum != 0)
  packet_ptr->header.checksum = cSum;
  //Send packet
  sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&server_addr, addrSize);
  fprintf(stderr,"Sent packet eof\tSeq %d\n",seq);
  resent = 0;
  //Make sure good ACK
  while(1) {
    FD_ZERO(&readfds);
    FD_SET(sock,&readfds);
    rv = select(sock + 1, &readfds, NULL, NULL, &tv);
    if(rv == -1) { //Error
      exit(EXIT_FAILURE);
    } else if (rv == 0) { //Timeout
      fprintf(stderr,"Packet loss eof\n");
      if(resent > 2)
        exit(EXIT_FAILURE);
      //Remake the packet and send if not 3 straight bad ACKs
      ++resent;
      packet_ptr->header.seq_ack = seq;
      packet_ptr->header.length = 0;
      memset(packet_ptr->data,0,10);
      packet_ptr->header.checksum = 0;
      cSum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
      randNum = rand() % RAND_MOD;
      //if(randNum != 0)
      packet_ptr->header.checksum = cSum; 
      sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&server_addr, addrSize); //Resend packet with right information 
      fprintf(stderr,"Resent packet eof\tSeq \n",seq);
      continue;
    } else {  //Received value
      numCharsRead = recvfrom(sock,packet_ptr,sizeof(PACKET),0,(struct sockaddr *) &server_addr, &addrSize);
      if(numCharsRead > 0) {
        //Wait for good ACK
        cSum = packet_ptr->header.checksum;
        packet_ptr->header.checksum = 0;
        packet_ptr->header.checksum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
        if(seq != packet_ptr->header.seq_ack
          //|| cSum != packet_ptr->header.checksum
          ) {
          fprintf(stderr,"Bad ACK eof\n");
          //Exit if 3 straight bad ACKs
          if(resent > 2) {
            exit(EXIT_FAILURE);
          } else {
            //Remake the packet and send if not 3 straight bad ACKs
            ++resent;
            packet_ptr->header.seq_ack = seq;
            packet_ptr->header.length = 0;
            memset(packet_ptr->data,0,10);
            packet_ptr->header.checksum = 0;
            cSum = calc_checksum(packet_ptr,headerSize + packet_ptr->header.length);
            randNum = rand() % RAND_MOD;
            //if(randNum != 0)
            packet_ptr->header.checksum = cSum; 
            sendto(sock, packet_ptr, pSize, 0, (struct sockaddr *)&server_addr, addrSize); //Resend packet with right information 
            fprintf(stderr,"Resent packet eof\n");
            continue;
          }
        }
        //Good ACK, change seq number and reset bad ACK
        resent = 0; //Reset the bad ACK counter
        fprintf(stderr,"Good ACK eof\n");
        seq = (seq == 0) ? 1 : 0; //Update sequence number
        break;
      }
    }
  }

  fclose(iFp);
  
  close(sock);
  exit(EXIT_SUCCESS);
}

