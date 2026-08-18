#ifndef _REQUEST_RESPONSE_HH
#define _REQUEST_RESPONSE_HH // DO NOT TOUCH

#define LEN 256
#define baseFIFOpath "./obj/FIFO-"

#include <sys/types.h>
#include <uuid/uuid.h>

struct Message {
    int messageType;
    uuid_t senderId;      // Defined with UUID
    uuid_t destinationId; // Defined with UUID
    int ticketNumber;
    int status;
    char data[LEN];
};

/*
 * message client types :
 * 1: Client filePathRequest        // Assigns to server the path of the file for SHA calculation
 * 2: Client Ticket Status Request  // Requests the status or position in queue
 * 3: Client FINACK                 // Closes the connection gracefully
 * 5: Same Client to Same Client    // When child talks to parent process
 *
 * message server types :
 * 101: Server filePath confirm     // ACK confirms receipt of path, now (not?) calculating SHA (not finished yet) and returns ticket of SHA process
 * 102: Server Ticket Status        // Returns the status of the thread that is calculating the SHA of the requesting client
 * 103: Server fileFirm response    // When server finishes calculating SHA sends the fingerprint to client, in status the ticket number
 * 104: EasterEgg                   // TODO:
 * 105: Server to Server            // When child talks to parent process
 * 106: Server assigns to thread a filepath for SHA calculation
 * 
 * message thread types : 
 * 201: Thread confirms its existence to server
 * 202: Thread responds with confirmation of work assignment
 * 203: Thread delivers to server the calculated hash
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
 * Yes, you can generate a UUID in C. The simplest way on Linux is to use libuuid,
 * which provides functions to create and manage UUIDs.
 * Example usage:
 *
 * #include <uuid/uuid.h>
 * #include <stdio.h>
 *
 * int main() {
 *     uuid_t uuid;
 *     char uuid_str[37];
 *
 *     uuid_generate(uuid);
 *     uuid_unparse(uuid, uuid_str);
 *
 *     printf("UUID: %s\n", uuid_str);
 *     return 0;
 * }
 *
 * To compile, link with -luuid:
 * gcc your_file.c -luuid -o your_program
 */