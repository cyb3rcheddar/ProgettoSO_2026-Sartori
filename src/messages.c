#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "errExit.h"
#include "msg.h"


void send(struct Message *message){
    char fifoPath[LEN];
    char uuid_str[37];

    // [1] Recupera la fifo path corretta 
    uuid_unparse(message->destinationId, uuid_str);
    snprintf(fifoPath, sizeof(fifoPath), "%s%s", baseFIFOpath, uuid_str);

    // [2] Apro la FIFO in scrittura
    if(message->messageType !=5 && message->messageType !=105 && message->messageType != 201 && message->messageType != 106){
       //printf("opening fifo on write : %s \n", fifoPath);
    }
    
    int fifoPointer = open(fifoPath, O_WRONLY);
    if (fifoPointer == -1) {
        printf("[MSG ERROR] errore in openWrite fifo : %s", fifoPath);
        printf("[MSG ERROR] Probabile processo non avviato \n");
        errExit("[MSG ERROR] Send : open fifo failed\n");
    }

    // [3] Scrive nella FIFO se esiste
    if (write(fifoPointer, message, sizeof(struct Message)) != sizeof(struct Message)){
        if (errno == ENOENT) {
            printf("[MSG ERROR] ENOENT in Write fifo : %s", fifoPath);
            errExit("[MSG ERROR] Send : FIFO non esiste, assicurati che il client sia in esecuzione.\n");
        } else {
            printf("[MSG ERROR] errore in Write fifo : %s", fifoPath);
            errExit("[MSG ERROR] Send : write on fifo failed\n");
        }
    }

    
    // [4] Switch in base al tipo di messaggio
    switch (message->messageType) {
        case 0:
            //printf("[MSG DEBUG] Invio richiesta ID client a %s\n", fifoPath);
            break;
        case 1:
            //printf("[MSG DEBUG] Invio richiesta filePath a %s\n", fifoPath);
            break;
        case 2:
            //printf("[MSG DEBUG] Invio richiesta stato ticket a %s\n", fifoPath);
            break;
        case 3:
            printf("[MSG DEBUG] Invio richiesta di chiusura connessione a %s\n", fifoPath);
            break;
        case 5:
            //printf("[MSG DEBUG] SND msg Client <-> Client \n");
            break;
        case 101:
            //printf("[MSG DEBUG] Invio risposta dal server in merito alla richiesta del file\n" );
            break;
        case 102:
            //printf("[MSG DEBUG] Invio risposta dal server in merito al ticket status\n" );
            break;
        case 103:
            //printf("[MSG DEBUG] Invio risposta dal server con hash del file\n");
            break;
        case 104:
            //printf("[MSG DEBUG] Invio server kill thread\n");
            break;
        case 105:
            //printf("[MSG DEBUG] SND msg Server <-> Server \n");
            break;
        case 106: // messaggio che il server usa per comunicare con i thread
            //printf("[MSG DEBUG] SND msg Server <-> Server \n");
            break;
        case 201: // messagio thread ok
            break;
        case 202: // messagio thread confirm job
            break;
        case 203: // messagio thread hash done
            break;
        default:
            fprintf(stderr, "<Error> Send : Tipo di messaggio sconosciuto: %d\n", message->messageType);
            return;
    }
    
    // [5] Chiudo la FIFO (non distrugge)
    if (close(fifoPointer) != 0){
        printf("[MSG ERROR] errore in close Write fifo : %s", fifoPath);
        errExit("[MSG ERROR] Send : close fifo failed");
    }
        
}

// memset(my_uuid, 0, sizeof(uuid_t));
void receive(uuid_t idFifo, struct Message *msg){
    char fifoPath[LEN];
    char uuid_str[37];
    int fifoPointer;
    
    // [1] Retrieve the correct FIFO path 
    uuid_unparse(idFifo, uuid_str);
    snprintf(fifoPath, sizeof(fifoPath), "%s%s", baseFIFOpath, uuid_str);
    
    // [2] Open the FIFO for reading
    //printf("[MSG DEBUG] opening fifo on read : %s \n", fifoPath);
    fifoPointer = open(fifoPath, O_RDONLY);
    if (fifoPointer == -1) {
        printf("[MSG ERROR] Error in openRead FIFO : %s", fifoPath);
        errExit("[MSG ERROR] Receive : open fifo failed");
    }   
    
    usleep(100);   //IMPORTANT: Sometimes the open of FIFO in read (async) takes too long and then read on a FIFO not yet opened happens, ugly but this way we are sure the FIFO is open
     
    // [3] Read the FIFO and wait for messages
    if(read(fifoPointer, msg, sizeof(struct Message)) != sizeof(struct Message)){
        if (errno == ENOENT) {
            printf("[MSG ERROR] ENOENT in Read FIFO : %s\n", fifoPath);
            errExit("[MSG ERROR] Receive : FIFO does not exist, make sure the client is running.\n");
        } else {
            printf("[MSG ERROR] Error in Read FIFO : %s\n", fifoPath);
            errExit("[MSG ERROR] Receive : read on fifo failed");
        }
    }
    
    // [4] Switch based on message type for client and server
    if(msg->status == 404){
        printf("┌────────────────────────────────────────────────────────────────────────┐\n");
        printf("│ %-70s │\n", "Response Received:");
        printf("│ MESSAGE TYPE: %-56d │\n", msg->messageType);
        printf("│ STATUS: %-62d │\n", msg->status);
        (msg->ticketNumber > 0) ? printf("│ TICKET: %-62d │\n", msg->ticketNumber) : (void)0; // If ticket number > 0 prints it
        printf("│ DATA: %-64s │\n", msg->data);
        printf("└────────────────────────────────────────────────────────────────────────┘\n");
    }else{
        //printf("[DEBUG]>MSG %d\n", msg->messageType);
    }
    fflush(stdout);
   
    switch (msg->messageType) {
        case 1:
            //printf("[MSG DEBUG] Ricevuto richiesta filePath a %s\n", fifoPath);
            break;
        case 2:
            //printf("[MSG DEBUG] Ricevuto richiesta stato ticket a %s\n", fifoPath);
            break;
        case 3:
            printf("[MSG DEBUG] Ricevuto richiesta di chiusura connessione a %s\n", fifoPath);
            break;
        case 5:
            //printf("[MSG DEBUG] RCV msg Client <-> Client \n");
            break;
        case 101:
            //printf("[MSG DEBUG] Ricevuto risposta dal server in merito alla richiesta del file\n" );
            break;
        case 102:
            //printf("[MSG DEBUG] Ricevuto risposta dal server in merito al ticket status\n" );
            break;
        case 103:
            //printf("[MSG DEBUG] Ricevuto hash del file\n");
            break;
        case 104:
            //printf("[MSG DEBUG] Ricevuto server kill thread\n");
            break;
        case 105:
            //printf("[MSG DEBUG] Ricevuto msg Server <-> Server \n");
            break;
        case 106: // Message that the server uses to communicate with threads
            //printf("[MSG DEBUG] Ricevuto msg Server <-> Server \n");
            break;
        case 201: // Message from thread ok
            break;
        case 202: // Message from thread confirm job
            break;
        case 203: // Message from thread hash done
            break;
        default:
            printf("<Error> Receive : Unknown message type: %d\n", msg->messageType);
            break;
    }
        
        
    // [5] Close the FIFO (does not destroy)
    if (close(fifoPointer) != 0){
        printf("[MSG ERROR] Error in close Read FIFO : %s", fifoPath);
        errExit("[MSG ERROR] close fifo failed");
    }  
}