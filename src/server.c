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
#include <pthread.h>
#include <signal.h>

#include "errExit.h"
#include "SHA_256.h"
#include "msg.h"
#include "threadOp.h"

#define LEN 256
#define NUM_THREAD 2


char uuid_str[37];// Support variable for reading/writing UUIDs
char path2ServerFIFO [LEN];
char path2ThreadFIFO [LEN]; 
struct Message msgRead;
struct Message msgWrite;
uuid_t threadId;
struct ThreadData threadData;

typedef struct clientNode{
    uuid_t clientId;
    struct clientNode *next;
} clientNode;

typedef struct node {
    int ticketNumber;
    int status; // 0 = queued, 1 = in progress, 2 = completed
    clientNode *clientHead;
    
    long weight;
    char filePath[LEN];
    char hash[65];


    struct node *next;
} node;


void addClient(clientNode **clientHead, uuid_t clientId){
    if (*clientHead != NULL && uuid_compare((*clientHead)->clientId, clientId) == 0) { // Prevents adding the same client ID multiple times in the list
        return;
    }
    clientNode *newNode = malloc(sizeof(clientNode));
    if (!newNode) {
        perror("Error: failed allocating memory for clientNode");
        return;
    }
    uuid_copy(newNode->clientId, clientId);
    newNode->next = NULL;
    
    if (*clientHead == NULL) {
        *clientHead = newNode;
    } else {
        clientNode *current = *clientHead;
        while (current) {
            if(uuid_compare(current->clientId, clientId) == 0){ // Prevents adding the same client ID multiple times in the list
                fflush(stdout);
                free(newNode);
                return;
            }
            if (current->next == NULL) {
                current->next = newNode;
                return;
            }
            current = current->next;
        }
    }

}

void stampaClientList(clientNode **clientHead){
    clientNode *current = *clientHead;
    char uuid_str[37];
    printf("<Server> printing client list : \n");
    fflush(stdout);
    while (current) {
        uuid_unparse(current->clientId, uuid_str);
        printf(" - Client: %s\n", uuid_str);
        fflush(stdout);
        current = current->next;
    }
}

void printNode(node *node){
    printf("┌────────────────────────────────────────────────────────────────────────┐\n");
    printf("Printing Node: \nTicket Number : %d\nStatus : %d\n", node->ticketNumber, node->status);
    stampaClientList(&node->clientHead);
    printf("File Path : %s\nHash : %s\nWeight : %ld\n", node->filePath, node->hash, node->weight);
    printf("└────────────────────────────────────────────────────────────────────────┘\n");
}

void printTicketList(node *head){
    node *current = head;
    printf("<Server> printing ticket list : \n");
    while (current) {
        printNode(current);
        current = current->next;
    }
}

/*
 * main function managing requests and tickets
 * if first request: assign ticket -> create node -> return ticket
 * if file in queue: search node -> return original ticket
 * if file finished: search node -> return original ticket + hash
 */


int addFileInQueue(node **head, int *ticketCounterSystem, const char *filePath, uuid_t clientId, char *hash, int *ticket) {

    // [0] Check memory allocation
    node *newNode = malloc(sizeof(node));
    if (!newNode) {
        perror("Error: failed allocating memory for node");
        return -1;
    }

    // [1] Check filepath validity
    if (access(filePath, F_OK) != 0) {
        fprintf(stderr, "<Server> The selected file: %s does not exist\n", filePath);
        free(newNode);
        return -1;
    }

    // [2] Check if file is already in queue | TODO function searchByPath ?
    node *current = *head;

    while (current) {
        if (strcmp(current->filePath, filePath) == 0) {
            //printNode(current);
            free(newNode);
            if (current->status == 2) {
                printf("<Server> Hash for file %s has already been computed, returning it\n", filePath);
                strcpy(hash, current->hash);
                *ticket = current->ticketNumber;
                return -3; // already computed
            } else if(current->status == 1) {
                printf("<Server> Hash for file %s is being calculated\n", filePath);
                *ticket = current->ticketNumber;
                return -2; // already in queue or in progress
            } else {
                printf("<Server> error handling status\n");
                return -1;
            }
        }
        current = current->next;
    }

    // [3] Initialize new node
    (*ticketCounterSystem)++;
    newNode->ticketNumber = *ticketCounterSystem;   // ticket number
    newNode->status = 0;                            // 0 = queued
    newNode->weight = 0;                            // default weight
    // calculate the file weight
    struct stat fileStat;
    if (stat(filePath, &fileStat) == 0) {
        newNode->weight = fileStat.st_size;
    } else {
        perror("stat failed");
    }
    newNode->clientHead = NULL;
    addClient(&newNode->clientHead, clientId);
    strncpy(newNode->filePath, filePath, LEN);
    newNode->filePath[LEN - 1] = '\0';              // Ensure null-termination
    newNode->next = NULL;

    if (*head == NULL) {
        *head = newNode;
    } else {
        node *current = *head;
        while (current->next) {
            current = current->next;
        }
        current->next = newNode;
    }

    // return -1 if path is invalid, -2 if file is already in queue, 1 otherwise
    *ticket = *ticketCounterSystem;
    return 1;
}

int setFileInProgress(node *head, const int ticketNumber) {
    node *current = head;
    while (current) {
        if (current->ticketNumber == ticketNumber) {
            current->status = 1;
            return 0;
        }
        current = current->next;
    }
    return -1;
}

int setFileHash(node *head, const int ticketNumber, const char *hash) {
    node *current = head;
    while (current) {
        if (current->ticketNumber == ticketNumber) {
            current->status = 2;
            strncpy(current->hash, hash, 65);
            current->hash[64] = '\0';  // Ensure null-termination
            return 0;
        }
        current = current->next;
    }
    return -1;  // File not found
}

/*
 * dato ticket Number restituisce in toClone la copia del nodo con dato ticket 
 */
int getTicketById(int ticketNumber, node *head, node *toClone) {  // 0 se success , -1 se fail
    node *current = head;
    while (current) {
        if (current->ticketNumber == ticketNumber) {
            memcpy(toClone, current, sizeof(node));
            return 0;
        }
        current = current->next;
    }
    memset(toClone, 0, sizeof(node));  // Not found, return empty node
    return -1;
}

/**
 * Restituisce il ticket in attesa con il peso minore
 */
int getPendingTicket(node *head,int *ticketNumber, char *filePath){
    node *current = head;
    long minWeight;
    int found = 0;
    while (current) {
        if (current->status == 0) {
            if(!found){
                minWeight = current->weight;
                found = 1;
            }
            if(current->weight <= minWeight){ // ("<=") is important to repeat less code
                minWeight = current->weight;
                *ticketNumber = current->ticketNumber;
                strcpy(filePath, current->filePath);
            }
        }
        current = current->next;
    }
    return found; // 0 if not found, 1 if found
}

void cleanup(){
    printf("<Server> removing FIFO...\n");
    if (unlink(path2ServerFIFO) != 0){
        errExit("unlink serverFIFO failed");
    }
    if (unlink(path2ThreadFIFO) != 0){
        errExit("unlink threadFIFO failed");
    }
}

void handle_sigint(int sig) {
    printf("\n[signal] Received SIGINT (Ctrl+C)\n");
    cleanup();
    exit(0);
}


int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
    printf("╔═══════════════════════════════════════╗\n");
    printf("║          Welcome to the Server        ║\n");
    printf("╚═══════════════════════════════════════╝\n");
    
    uuid_t serverId;
    char char_hash[65];
    
    node *head = NULL;
    node *clonedNode = malloc(sizeof(node));
    clientNode *currentClient;

    pthread_t threads[NUM_THREAD];
    int threadDisponibili = 0;
    int filesPending = 0;
    char filePath[LEN];

    int ticketCounterSystem;
    ticketCounterSystem = 0;

    memset(serverId, 0, sizeof(uuid_t));
    
    msgWrite.status = 0;
    msgWrite.ticketNumber = 0;
    strcpy(msgWrite.data, "");
    memcpy(msgWrite.senderId, serverId, sizeof(uuid_t));
    

    // [01] Crea fd per FIFO
    uuid_unparse(serverId, uuid_str);
    snprintf(path2ServerFIFO, sizeof(path2ServerFIFO), "%s%s", baseFIFOpath, uuid_str);

    if (mkfifo(path2ServerFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1) {
        if (errno != EEXIST){
            errExit("mkfifo failed");
        } else {
            printf("<Server> FIFO %s already exists, using it.\n", path2ServerFIFO);
        }
    } else {
        printf("<Server> server FIFO: %s created!\n", path2ServerFIFO);
    }
    

    // [02] inizializzazione dei thread
    uuid_generate(threadId);
    uuid_unparse(threadId, uuid_str);
    snprintf(path2ThreadFIFO, sizeof(path2ThreadFIFO), "%s%s", baseFIFOpath, uuid_str);

    if (mkfifo(path2ThreadFIFO, S_IRUSR | S_IWUSR | S_IWGRP) == -1) {
        if (errno != EEXIST){
            errExit("mkfifo failed");
        } else {
            printf("<Server> FIFO %s already exists, using it.\n", path2ThreadFIFO);
        }
    } else {
        printf("<Server> threads FIFO: %s created!\n", path2ThreadFIFO);
    }

    
    memcpy(threadData.threadId, threadId, sizeof(uuid_t));
    for (int i = 0; i < NUM_THREAD; i++) {
        if (pthread_create(&threads[i], NULL, threadOp, (void *)&threadData) != 0) {
            errExit("Error creating thread");
        }
        receive(serverId, &msgRead);
        if(msgRead.messageType == 201 && msgRead.status == 200){
            threadDisponibili++;
        }else{
            errExit("Error receiving message from thread");
        }
    }

    while(1) {
        receive(serverId, &msgRead);

        switch (msgRead.messageType) {

            // ---
            // MSG CLIENT -> SERVER
            // ---
            case 1:                                        // 1: Client filePathRequest        // assegna al server il path del file su cui calcolare SHA
                int ticketGiven;
                int result = addFileInQueue(&head, &ticketCounterSystem, msgRead.data, msgRead.senderId, char_hash, &ticketGiven);
                msgWrite.messageType = 101;
                memcpy(msgWrite.destinationId, msgRead.senderId, sizeof(uuid_t));
                switch (result){
                    case 1: // ticket added to queue
                        msgWrite.status = 200;
                        msgWrite.ticketNumber = ticketGiven;
                        strcpy(msgWrite.data, msgRead.data);
                        send(&msgWrite);        // server manda ack al client

                        // MANDO MESSAGGIO AD UN THREAD CHE COMINCIA AD ELABORARE
                        if (threadDisponibili > 0) {
                            msgWrite.messageType = 106;
                            msgWrite.status = 200;
                            msgWrite.ticketNumber = ticketGiven;
                            memcpy(msgWrite.destinationId, threadId, sizeof(uuid_t));
                            memcpy(msgWrite.data, msgRead.data, sizeof(msgRead.data));
                            send(&msgWrite);
                        }else{
                            filesPending++;
                            printf("<Server> added file %s to queue\n", msgRead.data);
                        }
                        break;
                    case -1: // error
                        msgWrite.status = 404;
                        msgWrite.ticketNumber = ticketGiven;
                        strcpy(msgWrite.data, "error searching file");
                        send(&msgWrite);
                        break;
                        break;
                    case -2: // ticket already in queue
                        msgWrite.status = 405;
                        msgWrite.ticketNumber = ticketGiven;
                        strcpy(msgWrite.data, "path received, added to request queue");
                        if (getTicketById(ticketGiven, head, clonedNode) == 0) {
                            addClient(&clonedNode->clientHead, msgRead.senderId);
                        }
                        send(&msgWrite);
                        break;   
                    case -3: // ticket already computed
                        msgWrite.status = 406;
                        msgWrite.ticketNumber = ticketGiven;
                        memcpy(msgWrite.data, &char_hash, sizeof(char_hash));
                        send(&msgWrite);
                        break;   
                }
                break;
            case 2:                                                                 // 2: Client Ticket Status Request  // requests the status or position in the queue
                // Restituisco lo status del ticket
                memcpy(msgWrite.senderId, serverId, sizeof(uuid_t));
                memcpy(msgWrite.destinationId, msgRead.senderId, sizeof(uuid_t));
                msgWrite.messageType = 102;
                msgWrite.ticketNumber = msgRead.ticketNumber;
                msgWrite.status = 404; // By default status is 404, if ticket is found it gets overwritten, if not found the ticket remains 404 and gives error "Ticket not found"

                for (node *curr = head; curr != NULL; curr = curr->next) {
                    if (curr->ticketNumber == msgRead.ticketNumber) {
                        msgWrite.status = curr->status;
                        break;
                    }
                }
                if (msgWrite.status == 0) {
                    msgWrite.status = 200;
                    strcpy(msgWrite.data, "Queued");
                } else if (msgWrite.status == 1) {
                    msgWrite.status = 200;
                    strcpy(msgWrite.data, "In progress");
                } else if (msgWrite.status == 2) {
                    msgWrite.status = 200;
                    strcpy(msgWrite.data, "Completed");
                } else {
                    msgWrite.status = 404;
                    strcpy(msgWrite.data, "Ticket not found");
                }

                send(&msgWrite);

                break;
            
            // ---
            // MSG THREAD -> SERVER
            // ---
            case 202:                                                                       // thread conferma presa in carico lavoro
                setFileInProgress(head, msgRead.ticketNumber);
                threadDisponibili--;
            break;

            case 203:                                                                       // thread consegna hash calcolato
                // THE THREAD HAS FINISHED THE SHA CALCULATION and the server is sending it to the client
                // salva il risultato nella lista
                setFileHash(head, msgRead.ticketNumber, msgRead.data);
                if (clonedNode) {
                    if (getTicketById(msgRead.ticketNumber, head, clonedNode) == 0) {        // copia in cloneNode il nodo della lista con il ticket corrispondente      
                        currentClient = clonedNode->clientHead;            
                        msgWrite.messageType = 103;
                        msgWrite.ticketNumber = clonedNode->ticketNumber;
                        memcpy(msgWrite.data, clonedNode->hash, sizeof(clonedNode->hash));
                        do{
                            memcpy(msgWrite.destinationId, currentClient->clientId, sizeof(uuid_t));
                            send(&msgWrite);                                                 // risponde a tutti i client interessati al risultato
                            currentClient = currentClient->next;
                        }while(currentClient != NULL);
                    }
                }
                threadDisponibili++;
                if(filesPending>0){ // If there are any files in queue waiting to be executed, I fetch the lightest file and send it directly to thread
                    if(getPendingTicket(head,&ticketGiven,filePath)){
                        msgWrite.messageType = 106;
                        msgWrite.status = 200;
                        msgWrite.ticketNumber = ticketGiven;
                        memcpy(msgWrite.destinationId, threadId, sizeof(uuid_t));
                        memcpy(msgWrite.data, filePath, sizeof(filePath));
                        send(&msgWrite);
                        filesPending--;
                    }else{
                        errExit("Error searching for minimum-weight file");
                    }
                }
            break;

        }
    }

    free(clonedNode);
    cleanup();
    exit(0);
}




