#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/sem.h>
#include <errno.h>

#include "errExit.h"


// @semid id

int waitSem(int semid) {
    struct sembuf operation;
    
    operation.sem_num = 0;          
    operation.sem_op = -1;          
    operation.sem_flg = SEM_UNDO;   

    if (semop(semid, &operation, 1) == -1) {
        errExit("<Semaforo> Wait Fallita");
        return -1;
    }
    return 0;
}


// @semid id

int signalSem(int semid) {
    struct sembuf operation;
    
    operation.sem_num = 0;          
    operation.sem_op = 1;           
    operation.sem_flg = SEM_UNDO;   

    if (semop(semid, &operation, 1) == -1) {
        errExit("<Semaforo> Signal Fallita");
        return -1;
    }
    return 0;
}


int sem_init(key_t key, int nsems) {
    int semid;
    // Create set
    semid = semget(key, nsems, IPC_CREAT | IPC_EXCL | 0666);
    
    if (semid == -1) {
        if (errno == EEXIST) {
            semid = semget(key, nsems, 0666);       // Get id
        } else {
            errExit("<Semaforo> SemGet Fallita");
            return -1;
        }
    } else {
        // Value initialization
        for (int i = 0; i < nsems; i++) {
            if (semctl(semid, i, SETVAL, 1) == -1) {
                errExit("<Semaforo> SemCtl Fallita");
                return -1;
            }
        }
    }
    
    return semid;
}



