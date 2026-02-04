#ifndef _THREADOP_HH
#define _THREADOP_HH

void *threadOp(void *arg);  // void *arg passes arguments to the function (to be checked in code)

struct ThreadData {
    uuid_t threadId; // common thread data for the fifo reciever, defined by server at thread creation
};

#endif