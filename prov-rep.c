#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>


int main(){
  //Below are defs for file size and user inputs.
    struct stat fileInfo; // info about file in buffer.
    int resourceType;
    int resourceAmount;
  //------------------------------------------------
  //---------Below are semaphore defs------------
    key_t Key = 6969;
    int semFlag = IPC_CREAT | 0666;
    /* the above create a new semaphore;

    0666 is the usual access permision in linux
    in rwx octal format and having the sequence
  */
    int newSem = 1; //1 new semaphore created
    int semID = semget(Key, newSem, semFlag); //semaphore ID

    //setting semaphore to 1:
    semctl(semID, 0, SETVAL, 1);
    //------------------------------------------------


    //----Defining incrementing and decrementing operations------
      struct sembuf incrementing;
      incrementing.sem_num = 0;
      incrementing.sem_op = 1;
      incrementing.sem_flg = SEM_UNDO;//automatically undones processes on termination.

      struct sembuf decrementing;
      decrementing.sem_num = 0;
      decrementing.sem_op = -1;
      decrementing.sem_flg = SEM_UNDO;
    //----------------------------------------------------------
      int nsops = 1;

    /*Open a text file for update
      r+ deletes the content nor
      creates a new file if it doesn't exist */
      FILE *resourceFile = fopen("res.txt", "r+");

      fstat(fileno(resourceFile), &fileInfo);
      int dataLength = fileInfo.st_size;
      int fileNum = fileno(resourceFile);
    //Above 3 lines get the file information

      //mapping to memory region..
      char *region = mmap(NULL, dataLength, PROT_WRITE | PROT_READ, MAP_SHARED, fileNum, 0);

      pid_t childpid; //for forking

      if((childpid = fork()) == -1){
        perror("Unable to Fork.\n");
        exit(0);
      }
      if(childpid == 0){
        while (1) {

        //entering critical section..
        semop(semID, &decrementing, nsops);

        printf("\n\nREPORTER (Child Process)\n");
        int pageSize = getpagesize();
        const size_t pSize = sysconf(_SC_PAGESIZE); //Size of a page in bytes.
        char *curr;

        //using mincore
        curr = (char*)calloc(1, (dataLength + pSize -1) / pSize);
        mincore(region, dataLength, curr);

        int i = 0;
        printf("Page Size: %d\n", pageSize);
        printf("Current State of Pages: %d\n", (curr[i] & 1));
        printf("Current State of Resources:\n%s",region);
        i++;
        printf("\n-------------------------------------------------------\n");

        //leaving critical section
        semop(semID, &incrementing, nsops);
        sleep(10); // Defined in spec
      }
    }
    else{
      sleep(2);//this way they will not print at the same time.
      printf("PROVIDER (Parent Process)\n");
      while (1) {
        printf("%s", region);

        printf("Resource type being provided: ");
        scanf("%d", &resourceType);
        printf("Amount of resource being provided: ");
        scanf("%d", &resourceAmount);

        //entering critical section..
        semop(semID, &decrementing, nsops);

        //providing resources...
        printf("\nCurrent Resource State:\n%s", region);
        char a = region[resourceType*4+2];
        char b[2];
        b[0] = a;
        b[1] = '\0';
        // printf("HELLO%c ; %c \n", a,b[0]);
        int availableResources = atoi(b);
        // printf("WORLD%d\n", availableResources);
        printf("\nResource Type: %d\n",resourceType);
        printf("Amount available: %d\n",availableResources);

        if ((availableResources + resourceAmount) < 10){
          int newAmount = availableResources + resourceAmount;
          char nA;
          sprintf(&nA, "%d", newAmount);

          region[resourceType*4+2] = nA;
          printf("\nResource Type: %d\n", resourceType);
          printf("Amount allocated: %d\n", resourceAmount);

          //syncing...
          msync(region, dataLength, MS_SYNC);
          /* msync():
    call to msync() flushes back to disk any changes made to
    a file mapped via mmap(), synchronizing the mapped file with the mapping.
    MS_SYNC:
    Specifies that synchronization should occur synchronously*/
        }
        //Resource units stricly less than 10
      else{
        printf("Too man resources......\n Cannot...Add\n");
      }
      //leaving critical section..
      semop(semID, &incrementing, nsops);
      }
    }
    return 0;
}
