#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>

//==================================================

#define BUF_SIZE (1 << 17)
#define MASTER_BUF_SIZE (1 << 9)
#define FACTOR 3

#define ASSERT(MSG, EXPR)                     \
    do {                                      \
        if (!(EXPR))                          \
        {                                     \
            perror("Message = '"#MSG"'; "     \
                   "expression = ("#EXPR")"); \
            exit(EXIT_FAILURE);               \
        }                                     \
    } while(0)

//==================================================

struct transieve
{
    int miso[2];
    int mosi[2];
    char* buffer;
    char* curstart;
    int cursize;
    int size;
};

enum
{
    FD_RD = 0,
    FD_WR
};

//==================================================

    /*
             ===========================
             |      ***MASTER***       |
             ===========================
             ^                         |
             |    |  ^          |  ^   +-> stdout
       file -+    |  |          |  |
                  V  |    ...   V  |

                  ====          ====
                  |S1|          |Sn|
                  ====          ====
    */

//==================================================

struct transieve* slaves = NULL;
int N = 0; //< Number of slaves

//==================================================

__attribute__((noreturn))
void master(void);

__attribute__((noreturn))
void slave(int i);

//==================================================

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        errno = EINVAL;
        perror("command line arguments");
        return 1;
    }
    N = atoi(argv[2]) + 2;
    //Create pipes
    slaves = calloc(N, sizeof(*slaves));
    ASSERT("calloc", slaves);

    int fd = open(argv[1], O_RDONLY);
    ASSERT("open", fd != -1);

    int pipe_ret, fcntl_ret;
    for (int i = 1; i < N - 1; i++)
    {
        pipe_ret = pipe(slaves[i].mosi);
        ASSERT("pipe", pipe_ret != -1);
        fcntl_ret = fcntl(slaves[i].mosi[FD_WR], F_SETFL, O_WRONLY|O_NONBLOCK); //< Allow partial write
        ASSERT("fcntl", fcntl_ret != -1);
        pipe_ret = pipe(slaves[i].miso);
        ASSERT("pipe", pipe_ret != -1);
    }

    slaves[0].miso[FD_RD] = fd;
    slaves[0].miso[FD_WR] = -1;
    slaves[0].mosi[FD_RD] = -1;
    slaves[0].mosi[FD_WR] = -1;

    slaves[N - 1].miso[FD_RD] = -1;
    slaves[N - 1].miso[FD_WR] = -1;
    slaves[N - 1].mosi[FD_RD] = -1;
    slaves[N - 1].mosi[FD_WR] = STDOUT_FILENO;

    //-----------------------------------
    for (int i = 1; i < N - 1; i++)
    {
        int fork_ret = fork();
        ASSERT("fork", fork_ret != -1);
        if (fork_ret == 0)
        {
            slave(i);
        }
    }
    master();
    return 0;
}

int max(int val1, int val2)
{
    return (val1 > val2) ? val1 : val2;
}

long min(long val1, long val2)
{
    return (val1 < val2) ? val1 : val2;
}

__attribute__((noreturn))
void master(void)
{
    //Close unused pipes
    for (int j = 1; j < N - 1; j++)
    {
        close(slaves[j].mosi[FD_RD]);
        close(slaves[j].miso[FD_WR]);
    }
    //Allocate memory for buffers
    int mul = 1;
    for (int i = N - 1; i > 0; i--)
    {
        slaves[i].size = min(mul * MASTER_BUF_SIZE, (1 << 20));
        slaves[i].curstart = slaves[i].buffer = calloc(slaves[i].size, sizeof(char));
        ASSERT("calloc", slaves[i].buffer);
        slaves[i].cursize = 0;
        mul *= FACTOR;
    }
    //Transfer data
    int max_fd = -1;
    while (1)
    {
        fd_set r, w;
        FD_ZERO(&r);
        FD_ZERO(&w);
        int active_fd_counter = 0;
        for (int i = 0; i < N; i++)
        {
            max_fd = max(max(max_fd, slaves[i].miso[FD_RD]), slaves[i].mosi[FD_WR]);
            if ((slaves[i].miso[FD_RD] != -1) && (slaves[i + 1].cursize == 0))
            {
                FD_SET(slaves[i].miso[FD_RD], &r);
                active_fd_counter++;
            }
            if ((slaves[i].mosi[FD_WR] != -1) && (slaves[i].cursize != 0))
            {
                FD_SET(slaves[i].mosi[FD_WR], &w);
                active_fd_counter++;
            }
        }
        if (!active_fd_counter) break;
        int select_ret = select(max_fd + 1, &r, &w, NULL, NULL);
        ASSERT("select", select_ret != -1);
        if (!select_ret) break;
        for (int i = 0; i < N; i++)
        {
            if (FD_ISSET(slaves[i].miso[FD_RD], &r))
            {
                ssize_t read_ret = read(slaves[i].miso[FD_RD], slaves[i + 1].buffer, slaves[i + 1].size);
                ASSERT("read", read_ret != -1);
                slaves[i + 1].cursize += read_ret;
                if (read_ret == 0)
                {
                    close(slaves[i].miso[FD_RD]);
                    close(slaves[i + 1].mosi[FD_WR]);
                    slaves[i].miso[FD_RD] = slaves[i + 1].mosi[FD_WR] = -1;
                }
            }
            if (FD_ISSET(slaves[i].mosi[FD_WR], &w))
            {
                ssize_t write_ret = write(slaves[i].mosi[FD_WR], slaves[i].curstart, slaves[i].cursize);
                ASSERT("write", write_ret != -1);
                if (write_ret < slaves[i].cursize)
                {
                    slaves[i].curstart += write_ret;
                } else
                {
                    slaves[i].curstart = slaves[i].buffer;
                }
                slaves[i].cursize  -= write_ret;
            }
        }
    }

    for (int i = 1; i < N; i++)
    {
        free(slaves[i].buffer);
    }
    free(slaves);
    exit(EXIT_SUCCESS);
}

__attribute__((noreturn))
void slave(int i)
{
    //Close unused pipes
    for (int j = 1; j < N - 1; j++)
    {
        if (j != i)
        {
            close(slaves[j].mosi[FD_RD]);
            close(slaves[j].mosi[FD_WR]);
            close(slaves[j].miso[FD_RD]);
            close(slaves[j].miso[FD_WR]);
        } else
        {
            close(slaves[j].mosi[FD_WR]);
            close(slaves[j].miso[FD_RD]);
        }
    }
    int out = slaves[i].miso[FD_WR];
    int in  = slaves[i].mosi[FD_RD];
    free(slaves);

    char buffer[BUF_SIZE] = {0};
    ssize_t read_ret = -1, write_ret = -1;
    while (read_ret)
    {
        read_ret = read(in, buffer, BUF_SIZE);
        ASSERT("read", read_ret != -1);
        write_ret = write(out, buffer, read_ret);
        ASSERT("write", write_ret == read_ret);
    }
    close(out);
    close(in);
    exit(EXIT_SUCCESS);
}
