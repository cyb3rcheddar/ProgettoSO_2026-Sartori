#ifndef _THREADOP_HH
#define _THREADOP_HH // not a typo - DO NOT TOUCH

void *threadOp(void *arg);// void *arg allows passing 0 or more arguments to the function (then check in code) 

struct ThreadData {
    uuid_t threadId; // Shared among all threads, is the FIFO ID on which threads do receive, must be defined by server before thread creation
};

#endif
