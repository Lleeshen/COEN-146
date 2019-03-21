#ifndef TFV2_H
#define TFV2_H

#define SIZE 10

typedef struct {
  int seq_ack;    //Will always be 0 or 1
  int length;     //How many bytes of data you have
  int checksum;   //Checksum calculated by XORing the bytes n packet
} HEADER;

typedef struct {
  HEADER header;
  char data[SIZE];
} PACKET;

int calc_checksum(PACKET *pkt, int nBytes) {
  int i = 0;
  char cs = 0;
  char *p = (char *) pkt;
  for(i = 0; i < nBytes; ++i) {
    cs = cs ^ *p;
    ++p;  
  }
  return (int) cs;
}

#endif
