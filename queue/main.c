#include "stdio.h"
#include "unistd.h"
#include "stdlib.h"
#include "sys/types.h"
#include "sys/msg.h"
#include "sys/ipc.h"
#include "errno.h"
#include "assert.h"

#define ASSERT(MSG, EXPR)                     \
    do {                                      \
        if (!(EXPR))                          \
        {                                     \
            perror("Message = '"#MSG"'; "     \
                   "expression = ("#EXPR")"); \
            exit(EXIT_FAILURE);               \
        }                                     \
    } while(0)

struct msgbuf
{
    long mtype;
};

int main(int argc, char* argv[])
{
    //Parse argument
    if (argc != 2)
    {
        errno = EINVAL;
        perror("command line arguments");
        return 1;
    }
    long num = atoi(argv[1]);
    //Create queue
    int qid = msgget(IPC_PRIVATE, 0600);
    ASSERT(msgget, qid != -1);
    //Create childs
    pid_t pid = -1;
    pid_t i;
    for (i = 1; i <= num; i++)
    {
        pid = fork();
        ASSERT(fork, pid != -1);
        if (pid == 0) break;
    }
    if (pid == 0) //< Child
    {
        struct msgbuf msg = {0};
        int rcv_ret = msgrcv(qid, &msg, 0, i - 1, 0);
        ASSERT(msgrcv, rcv_ret != -1);
        printf("%d\n", i);
        fflush(stdout);
    }
    if (i != num)
    {
        struct msgbuf msg = { .mtype = i };
        int snd_ret = msgsnd(qid, &msg, 0, 0);
        ASSERT(msgsnd, snd_ret != -1);
    } else
    {
        msgctl(qid, IPC_RMID, NULL);
    }
    return 0;
}
