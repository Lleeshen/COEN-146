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
#include <pthread.h>

#define INF 10000

typedef struct {
  char name[50];
  char ip[50];
  int port;
} Machine;

//Cost matrix, machines array, lock, machineId, and numMachines are global
int cost_matrix[4][4];
Machine machines[4];

pthread_mutex_t lock;

int machineId;
int numMachines;

void* receiveInfo(void * arg);
void* linkState(void * arg);

void setNumMachines(char * number);
void setMachineId(char * id);
void initializeCostTable(char * name);
void initializeMachineTable(char * name);
void printTable();
void changeCost();
void updateCost(int * edge);

int main (int argc, char ** argv) {
  //Make sure right amount of arguments
  if(argc != 5) {
    fprintf(stderr,"Usage: %s machineID totalMachines costFile machineFile\n",argv[0]);
    exit(EXIT_FAILURE);  
  }
  //Setup for randomization
  srand(time(NULL));
  //Set up machineId, numMachines, and machines array
  setNumMachines(argv[2]);
  setMachineId(argv[1]);
  initializeMachineTable(argv[4]);
  //Set up cost table array
  initializeCostTable(argv[3]);
  pthread_t thr1, thr2;
  //Run Receive Info thread
  pthread_create(&thr1,NULL,receiveInfo,NULL);
  //Run Link State thread
  pthread_create(&thr2,NULL,linkState,NULL);
  //Start locking mechanism
  pthread_mutex_init(&lock,NULL);
  //Function that allows user to change cost to an edge
  changeCost();
}

void setNumMachines(char * number) {
  //Get numMachines from argument safely
  char * end = NULL;
  int errno;
  long longNumMachines = strtol(number,&end,10);
  if(end == number) {
    fprintf(stderr,"Couldn't read number of machines\n");
    exit(EXIT_FAILURE);
  }
  if(errno == ERANGE && (longNumMachines == LONG_MAX || longNumMachines == LONG_MIN)) {
    fprintf(stderr,"Failed to get number of machines\n");
    exit(EXIT_FAILURE);
  }
  //Initialize variable
  numMachines = (int) longNumMachines;
}

void setMachineId(char * id) {
  //Get machineId from argument safely
  char * end = NULL;
  int errno;
  long longID = strtol(id,&end,10);
  if(end == id) {
    fprintf(stderr,"Couldn't read machine number\n");
    exit(EXIT_FAILURE);
  }
  if(errno == ERANGE && (longID == LONG_MAX || longID == LONG_MIN)) {
    fprintf(stderr,"Failed to get machine number\n");
    exit(EXIT_FAILURE);
  }
  //Initialize variable
  machineId =  (int) longID;
}

void initializeMachineTable(char * name) {
  //Open file
  FILE * machineFile;
  int i = 0;
  machineFile = fopen(name, "r");
  if(machineFile != NULL) {
    while(1) {
      //Store file contents into machine array 
      if(fscanf(machineFile,"%s %s %d", machines[i].name, machines[i].ip, &machines[i].port) == 3) {
        ++i;
      } else {
        //Done loading contents, either close file and return error based on number of lines read
        if(i == numMachines) {
          fclose(machineFile);
          break;
        }
        else {
          fprintf(stderr,"Did not scan all lines of machine file\n");
          exit(EXIT_FAILURE);
        }
      }
    }
  } else {
    //Exit if cannot open file
    fprintf(stderr,"Could not open machines file\n");
    exit(EXIT_FAILURE);
  }
}

void initializeCostTable(char * name) {
  //Open file
  FILE * costFile;
  int i = 0;
  costFile = fopen(name,"r");
  if(costFile != NULL) {
    while(1) {
      //Load contents into cost matrix
      if(fscanf(costFile,"%d %d %d %d",&cost_matrix[i][0],&cost_matrix[i][1],&cost_matrix[i][2],&cost_matrix[i][3]) == 4)
        ++i;
      else {
        //Done loading contents, either close file or return error based on amount of data read
        if(i == numMachines) {
          fclose(costFile);
          break;
        }
        else {
          fprintf(stderr,"Did not scan all lines of cost file\n");
          exit(EXIT_FAILURE);
        }
      }
    }
  } else {
    //Exit if cannot open file
    fprintf(stderr,"Could not open costs file\n");
    exit(EXIT_FAILURE);
  }
}

void printTable() {
  int i, j;
  //Action of printing table should be locked
  pthread_mutex_lock(&lock);
  //Loop through each machine
  for(i = 0; i < numMachines; ++i) {
    printf("Machine %d:\t",i);
    //For each machinie, print the cost to every other machine
    for(j = 0; j < numMachines; ++j) {
      printf("%d\t",cost_matrix[i][j]);
    }
    printf("\n");
  }
  pthread_mutex_unlock(&lock);
}

void * receiveInfo(void * arg) {
  //printf("Testing receiveInfo thread\n");

  int edge[3];
  //Set up Socket to receive
  int sock;
  int portNum = machines[machineId].port;
  struct sockaddr_in server_addr, client_addr;
  struct hostent *host;
  int addrSize = sizeof(struct sockaddr);

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(portNum);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    fprintf(stderr,"Socket Error\n");
    exit(EXIT_FAILURE);
  }

  if (bind(sock,(struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
    fprintf(stderr,"Socket bind error\n");
    exit(EXIT_FAILURE);
  }

  while(1) {
    printf("Waiting to receive cost change\n");
    //Wait to receive cost change
    if(recvfrom(sock,edge,sizeof(edge),0,(struct sockaddr *) &client_addr,&addrSize) > 0) {
        printf("Received new cost\n");
        //Change cost_matrix
        updateCost(edge);
        //Print new cost_matrix
        printf("Updated cost\n");
        printTable();
    }
  }
}

void * linkState(void * arg) {
  
  //Declare variables for Dijkstra's alg
  int leastDistance[4];
  int visited[4];
  int wait;
  int i, j;
  int curCost;
  int minCost;
  int minIndex;
  
  while(1) {
    //Print current table
    printf("Current cost table:\n");
    printTable();
    //Running the algorithm should be locked
    pthread_mutex_lock(&lock);

    //Initialize current edges as least distance and mark all machines as not visited
    for(i = 0; i < numMachines; ++i) {
      leastDistance[i] = cost_matrix[machineId][i];
      visited[i] = 0;
    }
    //Don't need to visit current machine
    visited[machineId] = 1;

    //Visit the remaining machines
    for(i = 1; i < numMachines; ++i) {
        //Find Closest unvisited machine, save machine Id
        minCost = INF;        
        for(j = 0; j < numMachines; ++j) {
          if(visited[j] != 1 && minCost > leastDistance[j]) {
            minCost = leastDistance[j];
            minIndex = j;
          }        
        }
        visited[minIndex] = 1;
        //Update Least distances to every other node based on edges of the machine last visited
        for(j = 0; j < numMachines; ++j) {
          if(visited[j] != 1 && leastDistance[minIndex] + cost_matrix[minIndex][j] < leastDistance[j]) {
            leastDistance[j] = leastDistance[minIndex] + cost_matrix[minIndex][j];
          }
        }    
    }
    //Finished algorithm, can unlock
    pthread_mutex_unlock(&lock);
    //Print stored least costs
    printf("\nLeast Costs from Machine %d:\n",machineId);
    for(i = 0; i < numMachines; ++i) {
      printf("Least cost to Machine %d is %d\n",i,leastDistance[i]);    
    }
    //Wait for 10 to 20 seconds
    wait = rand() % 11 + 10;
    sleep(wait);
  }
}

void changeCost() {


  int edge[3];
  edge[0] = machineId;
  
  //Set up some of the socket information
  
  int sock;
  int portNum;
  struct sockaddr_in server_addr;
  struct hostent *host;
  int addrSize = sizeof(struct sockaddr);

  int i;
  while(1) {
    sleep(10);
    printf("Wait for input: MachineId Cost\n");
    //Scan new cost to machine
    scanf("%d %d", &edge[1], &edge[2]);
    //Cost to self cannot be changed; Don't use invalid Machine Id
    if(edge[1] >= numMachines || edge[0] == edge[1]) {
      printf("Invalid input, try again\n");
      continue;
    }
    //Change cost matrix of this machine
    updateCost(edge);
    //Send the cost to every machine 
    for(i = 0; i < numMachines; ++i) {
      if(i != edge[0]) {
        //Use that macines's port number
        portNum = machines[i].port;
        host = (struct hostent *) gethostbyname(machines[i].ip);
        //Make a new socket
        if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
          fprintf(stderr,"Socket error\n");
          exit(EXIT_FAILURE);        
        }
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(portNum);
        server_addr.sin_addr = *((struct in_addr *)host->h_addr);
        //Send updated edge to other machines
        printf("Sending to Machine %d on port %d\n",i,portNum);
        sendto(sock, edge, sizeof(edge), 0, (struct sockaddr *)&server_addr , addrSize);
      }
    }
    printTable();
  }
}

void updateCost(int * edge) {
  //Lock cost_matrix while it is updated
  pthread_mutex_lock(&lock);
  //Change the cost given
  cost_matrix[edge[0]][edge[1]] = edge[2];
  //Change cost of the same edge reversed
  cost_matrix[edge[1]][edge[0]] = edge[2];
  pthread_mutex_unlock(&lock);
}
