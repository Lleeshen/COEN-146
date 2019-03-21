#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#define BUFFER_LENGTH 10
#define NDEBUG

int main (int argc, char * argv[]) {
  //Arguments for source and destination files
  assert(argc > 2);
  //Open file pointers for files
  FILE * fps = fopen(argv[1], "rb");
  FILE * fpd = fopen(argv[2], "wb");
  //Set p buffer
  char * buffer;
  buffer = malloc(sizeof(char) * BUFFER_LENGTH);
  //Keep track of characters read
  int readChars;
  //Read input when there are characters to be read
  while(readChars = fread(buffer,1,BUFFER_LENGTH,fps)) {
    //Write the same ouput when there input is read successfully
    fwrite(buffer,1,readChars,fpd);
  }
  fclose(fps);
  fclose(fpd);
  return 0;
}
