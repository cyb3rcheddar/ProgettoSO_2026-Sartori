#ifndef _REQUEST_RESPONSE_HH
#define _REQUEST_RESPONSE_HH

#define LEN 256
#define baseFIFOpath "./obj/FIFO-"

#include <sys/types.h>
#include <uuid/uuid.h>

struct Message {
    int messageType;
    uuid_t senderId;      // uuid defined
    uuid_t destinationId; // uuid defined
    int ticketNumber;
    int status;
    char data[LEN];
};

/*
 * message client types :
 * 1: Client filePathRequest        // assigns to server the path of the file for SHA calculation
 * 2: Client Ticket Status Request  // requests the status or position in queue
 * 3: Client FINACK                 // closes the connection gracefully
 * 5: Same Client to Same Client    // when child talks to parent process
 *
 * message server types :
 * 101: Server filePath confirm     // ACK confirms receipt of path, now (not?) calculating SHA (not finished yet) and returns ticket of SHA process
 * 102: Server Ticket Status        // returns the status of the thread that is calculating the SHA of the requesting client
 * 103: Server fileFirm response    // when server finishes calculating SHA sends the fingerprint to client, status contains ticket number
 * 104: Undefined value             // TODO
 * 105: Server to Server            // when child talks to parent process
 * 106: Server assigns to thread a filepath for SHA calculation
 * 
 * message thread types : 
 * 201: thread confirms its existence to server
 * 202: thread responds with confirmation of work assignment
 * 203: thread delivers to server the calculated hash
 * 
 * Status types :
 * 200: ok
 * 201: ticket in processing
 * 404: file not found
 * 405: in processing
 * 406: in cache 
*/

void send(struct Message *message);
void receive(uuid_t idFifo, struct Message *msg);

#endif

/*
 
Yes, you can generate a UUID in C. The simplest way on Linux is to use the libuuid library,
which provides functions to create and manage UUIDs.
Usage example:

#include <uuid/uuid.h>
#include <stdio.h>

int main() {
    uuid_t uuid;
    char uuid_str[37];

    uuid_generate(uuid);
    uuid_unparse(uuid, uuid_str);

    printf("UUID: %s\n", uuid_str);
    return 0;
}

To compile, add -luuid:
gcc your_file.c -luuid -o your_program

*/