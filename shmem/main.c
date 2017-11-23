#define _GNU_SOURCE
#include "assert.h"
#include "errno.h"
#include "stdio.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "sys/ipc.h"
#include "sys/shm.h"
#include "sys/sem.h"

//------------------------------------------

#define ASSERT(MSG, EXPR)                 \
do {                                      \
    if (!(EXPR))                          \
    {                                     \
        perror("\nMessage = '"MSG"'; "    \
               "expression = ("#EXPR")"); \
        exit(EXIT_FAILURE);               \
    }                                     \
} while(0)

//------------------------------------------

enum { BUFFER_SIZE = (1 << 0) };

struct shmem {
    int   bytes_read;
    char  data[BUFFER_SIZE];
};

const int SHARED_KEY = 0xDEADBEEF;
enum { SEMBUF_SIZE = 16 };
enum { SHMEM_SIZE = sizeof(struct shmem) };

enum {
    WAIT = 0,
    CHECK = IPC_NOWAIT
};

//------------------------------------------

int semop_push(short val, short flag);
int semop_flush(int errno_ok);

void consumer(void);
void producer(int fd);

//------------------------------------------

// More global variables!
struct sembuf sbuf[SEMBUF_SIZE] = {0};
int sbuf_counter = 0;
int semid = -1, shmid = -1;
int semop_ret = -1;
struct shmem* shm = (void*) -1;

//------------------------------------------

int main(int argc, char* argv[])
{
    if (argc > 2) {
        errno = EINVAL;
        perror("cmd line arguments");
        return 1;
    }
    int is_consumer = (argc == 1);

    semid = semget(SHARED_KEY, 1, IPC_CREAT|0666);
    ASSERT("semget", semid != -1);

    shmid = shmget(SHARED_KEY, SHMEM_SIZE, IPC_CREAT|0666);
    ASSERT("shmget", shmid != -1);

    shm = shmat(shmid, NULL, 0);
    ASSERT("shmat", shm != (void*) -1);

    if (is_consumer) {
        consumer();
    } else {
        int fd = open(argv[1], O_RDONLY);
        ASSERT("open", fd != -1);
        producer(fd);
    }

    return 0;
}

int semop_push(short val, short flag)
{
    sbuf[sbuf_counter].sem_op  = val;
    sbuf[sbuf_counter].sem_flg = flag;
    sbuf[sbuf_counter].sem_num = 0;
    sbuf_counter++;
}

int semop_flush(int errno_ok)
{
    semop_ret = semop(semid, sbuf, sbuf_counter);
    errno = (semop_ret == -1) ? errno : 0;
    ASSERT("semop", (semop_ret != -1) || (errno == errno_ok));
    sbuf_counter = 0;
}

#define NOT_LESS_THAN(VAL, FLAG) \
do {                             \
    semop_push(-(VAL), FLAG);    \
    semop_push(+(VAL), 0);       \
} while (0)

#define EQUAL_TO(VAL, FLAG)      \
do {                             \
    semop_push(-(VAL), 0);       \
    semop_push(     0, FLAG);    \
    semop_push(+(VAL), 0);       \
} while (0)

void consumer(void)
{
    //============ENTER REGION============================================
    EQUAL_TO(1, WAIT);
    semop_push(+1, SEM_UNDO);
    semop_flush(0); // Val = 2 (p_adj = 1; c_adj = 1)
    //Wait for producer
    NOT_LESS_THAN(2, CHECK);
    EQUAL_TO(4, WAIT);
    semop_push(+1, SEM_UNDO);
    semop_flush(0); // Val = 5 (p_adj = 3; c_adj = 2)
    //============TRANSFER DATA===========================================
    ssize_t write_ret = -1;
    while (1) {
        NOT_LESS_THAN(4, CHECK);
        EQUAL_TO(4, WAIT);
        semop_push(-1, SEM_UNDO);
        semop_flush(0); // Val = 3 (p_adj = 2; c_adj = 1)

        NOT_LESS_THAN(3, CHECK);
        EQUAL_TO(4, WAIT);
        semop_flush(0); // Val = 4 (p_adj = 3; c_adj = 1)

        if (shm->bytes_read == 0)
            break;

        write_ret = write(STDOUT_FILENO, shm->data, shm->bytes_read);
        ASSERT("write", write_ret == shm->bytes_read);
        shm->bytes_read = 0;

        NOT_LESS_THAN(4, CHECK);
        semop_push(+1, SEM_UNDO);
        semop_flush(0); // Val = 5 (p_adj = 3; c_adj = 2)
    }
}

void producer(int fd)
{
    //============ENTER REGION============================================
    EQUAL_TO(0, WAIT);
    semop_push(+1, SEM_UNDO);
    semop_flush(0); // Val = 1 (p_adj = 1; c_adj = 0)
    //Wait for consumer
    EQUAL_TO(2, WAIT);
    semop_push(+2, SEM_UNDO);
    semop_flush(0); // Val = 4 (p_adj = 3; c_adj = 1)
    //============TRANSFER DATA===========================================
    shm->bytes_read = 0;

    ssize_t read_ret = -1;
    while (read_ret) {
        NOT_LESS_THAN(4, CHECK);
        EQUAL_TO(5, WAIT);
        semop_push(-1, SEM_UNDO);
        semop_flush(0); // Val = 4 (p_adj = 2; c_adj = 2)

        NOT_LESS_THAN(3, CHECK);
        EQUAL_TO(3, WAIT);
        semop_flush(0); // Val = 3 (p_adj = 2; c_adj = 1)

        read_ret = read(fd, shm->data, BUFFER_SIZE);
        ASSERT("read", read_ret >= 0);
        shm->bytes_read = read_ret;

        NOT_LESS_THAN(3, CHECK);
        semop_push(+1, SEM_UNDO);
        semop_flush(0); // Val = 4 (p_adj = 3; c_adj = 1)
    }
    NOT_LESS_THAN(4, CHECK);
    semop_push(0, 0);
    semop_flush(EAGAIN);
}
#undef NOT_LESS_THAN
#undef EQUAL_TO
