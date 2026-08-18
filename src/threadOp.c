#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <uuid/uuid.h>
#include <sys/ipc.h>
#include <time.h>

#include "errExit.h"
#include "SHA_256.h"
#include "msg.h"
#include "semaforo.h"

// Structure for shared data among threads
struct ThreadData {
    uuid_t threadId; // Shared among all threads, is the FIFO ID on which threads do receive, must be defined by server before thread creation
};

void *threadOp(void *arg){
    struct Message msgRead;
    struct Message msgWrite;

    struct timespec start, end;
    double elapsed;

    uint8_t hash[32];
    char char_hash[65];

    struct ThreadData *threadData = (struct ThreadData *)arg;

    uuid_t serverId;
    memset(serverId, 0, sizeof(uuid_t)); // Server ID set to 0

    msgWrite.messageType = 201;
    msgWrite.ticketNumber = 0;
    msgWrite.status = 200;
    memcpy(msgWrite.senderId, threadData->threadId, sizeof(uuid_t));
    memcpy(msgWrite.destinationId, serverId, sizeof(uuid_t));
    memcpy(msgWrite.data, "thread created", strlen("thread created") + 1);
    send(&msgWrite);                // Writes to server that thread was created

    
    key_t key = 1000;       // Semaphore key for threads
    int semaforoId = sem_init(key,1);       // Request a semaphore given the key (on create: take or create)
    if(semaforoId == -1){
        errExit("<Thread> Failed to get semaphore");
    }
    do{
        waitSem(semaforoId);
        receive(threadData->threadId, &msgRead);
        signalSem(semaforoId);
        switch (msgRead.messageType) {
            case 106:                   // 106: Server assigns a filepath to a thread for SHA calculation
                printf("<Thread> file received: %s\n",msgRead.data);
                clock_gettime(CLOCK_MONOTONIC, &start);

                // Communicate to server that I am starting work
                msgWrite.messageType = 202;
                msgWrite.ticketNumber = msgRead.ticketNumber;
                memcpy(msgWrite.data, "thread ticket assign", strlen("thread ticket assign") + 1);
                msgWrite.status = 200;
                send(&msgWrite);                    // Confirmation of ticket assignment
            
                memset(hash, 0, sizeof(hash));
                memset(char_hash, 0, sizeof(char_hash));
                digest_file(msgRead.data, hash);
                
                for(int i = 0; i < 32; i++) {
                    sprintf(char_hash + (i * 2), "%02x", hash[i]);
                }
                // Send the result to the server which then forwards it to client
                msgWrite.messageType = 203;
                msgWrite.status = 200;
                msgWrite.ticketNumber = msgRead.ticketNumber;
                memcpy(msgWrite.destinationId, serverId, sizeof(uuid_t));
                memcpy(msgWrite.data, char_hash, sizeof(char_hash));
                sleep(10);      // Hashing is too fast, slowing it down a bit to see if scheduling works
                clock_gettime(CLOCK_MONOTONIC, &end);
                elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

                printf("<Thread> file: %s - DONE - Hash: %s | Processing time: %.3f seconds\n",msgRead.data,char_hash, elapsed);
                send(&msgWrite);
              
            break;
            default:
                errExit("<Thread> RCV received unrecognized message");
        }
        
    }while(msgRead.messageType != 104 );
}