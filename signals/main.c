#include <signal.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

//------------------------------------------

#define ASSERT(MSG, EXPR)                 \
do {                                      \
    if (!(EXPR))                          \
    {                                     \
        perror("Message = '"MSG"'; "      \
               "expression = ("#EXPR")"); \
        exit(EXIT_FAILURE);               \
    }                                     \
} while(0)

//------------------------------------------

enum { BUF_SIZE = (1 << 14) };

//------------------------------------------

int incoming_bit = 0;

//------------------------------------------

void parent(pid_t cpid);
void child(int fd, pid_t ppid);

void receive (pid_t where, void* buf, int size);
void transmit(pid_t where, void* buf, int size);

void incoming_bit_handler(int sig);
void dummy_handler(int sig);

//------------------------------------------

int main(int argc, char* argv[])
{
    if (argc != 2) {
        errno = EINVAL;
        perror("cmd line arguments");
        return 1;
    }
    int ses_ret, sas_ret, spm_ret, sac_ret;
    struct sigaction act = {0};
    sigset_t sigs;

    ses_ret = sigemptyset(&sigs);
    ASSERT("sigemptyset", ses_ret != -1);
    sas_ret = sigaddset(&sigs, SIGUSR1);
    ASSERT("sigaddset", sas_ret != -1);
    sas_ret = sigaddset(&sigs, SIGUSR2);
    ASSERT("sigaddset", sas_ret != -1);
    spm_ret = sigprocmask(SIG_SETMASK, &sigs, NULL);
    ASSERT("sigprocmask", spm_ret != -1);

    int fork_ret = fork();
    ASSERT("fork", fork_ret != -1);
    if (fork_ret > 0) {
        act.sa_handler = exit;
        sac_ret = sigaction(SIGCHLD, &act, NULL);
        ASSERT("sigaction", sac_ret != -1);
        act.sa_handler = incoming_bit_handler;
        sac_ret = sigaction(SIGUSR1, &act, NULL);
        ASSERT("sigaction", sac_ret != -1);
        act.sa_handler = incoming_bit_handler;
        sac_ret = sigaction(SIGUSR2, &act, NULL);
        ASSERT("sigaction", sac_ret != -1);

        parent(fork_ret);
    } else {
        act.sa_handler = dummy_handler;
        sac_ret = sigaction(SIGUSR1, &act, NULL);
        ASSERT("sigaction", sac_ret != -1);

        pid_t ppid = getppid();
        if (ppid == 1) {
            exit(1);
        }

        int fd = open(argv[1], O_RDONLY);
        child(fd, ppid);
    }
}

void incoming_bit_handler(int sig)
{
    incoming_bit = (sig == SIGUSR1) ? 0 : 1;
}

void dummy_handler(int sig) {}

void receive(pid_t where, void* buf, int size)
{
    sigset_t sigs;
    int ssp_ret, ses_ret, kill_ret;
    char* cbuf = buf;

    ses_ret = sigemptyset(&sigs);
    ASSERT("sigemptyset", ses_ret != -1);

    for (int j = 0; j < CHAR_BIT * size; j++) {
        ssp_ret = sigsuspend(&sigs);
        ASSERT("sigsuspend", errno == EINTR);
        if (incoming_bit) {
            cbuf[j / CHAR_BIT] |=   1 << (j % CHAR_BIT);
        } else {
            cbuf[j / CHAR_BIT] &= ~(1 << (j % CHAR_BIT));
        }
        kill_ret = kill(where, SIGUSR1);
        ASSERT("kill", kill_ret != -1);
    }
}

void transmit(pid_t where, void* buf, int size)
{
    sigset_t sigs;
    int ses_ret, ssp_ret, kill_ret, sig;
    char* cbuf = buf;

    ses_ret = sigemptyset(&sigs);
    ASSERT("sigemptyset", ses_ret != -1);
    for (int j = 0; j < CHAR_BIT * size; j++) {
        sig = (cbuf[j / CHAR_BIT] & (1 << (j % CHAR_BIT))) ?
              SIGUSR2 :
              SIGUSR1;
        kill_ret = kill(where, sig);
        ASSERT("kill", kill_ret != -1);
        alarm(1);
        ssp_ret = sigsuspend(&sigs);
        alarm(0);
        ASSERT("sigsuspend", errno == EINTR);
    }
}

void parent(pid_t cpid)
{
    ssize_t bytes_to_read = -1;
    char buffer[BUF_SIZE] = {0};

    while (bytes_to_read) {
        receive(cpid, &bytes_to_read, sizeof(bytes_to_read));
        receive(cpid, buffer, bytes_to_read);

        int write_ret = write(STDOUT_FILENO, buffer, bytes_to_read);
        ASSERT("write", write_ret == bytes_to_read);
    }
}

void child(int fd, pid_t ppid)
{
    char buffer[BUF_SIZE] = {0};
    ssize_t read_ret = -1;
    while (read_ret) {
        read_ret = read(fd, buffer, BUF_SIZE);
        ASSERT("read", (read_ret != -1));

        transmit(ppid, &read_ret, sizeof(read_ret));
        transmit(ppid, buffer, read_ret);
    }
}
