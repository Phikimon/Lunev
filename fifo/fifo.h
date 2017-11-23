#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

//---------------------------------------------------

#define CHECK(func_name, ret_val_name) \
do {                                   \
    if (ret_val_name == -1)            \
    {                                  \
        perror(#func_name);            \
        exit(1);                       \
    }                                  \
} while(0)

#define CLOSE(name)         \
do {                        \
    if (close(name) == -1)  \
    {                       \
        perror("close");    \
        exit(1);            \
    }                       \
    name = -1;              \
} while(0)

//---------------------------------------------------

enum { MAX_STR_LEN = 128 };
enum { BUF_SIZE_READ = (16 * 1024) };

//------------------------------------

extern const char DATA_FIFO  [];
extern       char LOCK_PREFIX[];

//------------------------------------

enum FDS_NAMES
{
    LOCK = 0,
    WR_SYNC_FIFO,
    RD_SYNC_FIFO,
    INPUT_FD,
    OUTPUT_FD,
    FDS_NUM
};

enum CHECK_MAILBOX_RETURN_CODES
{
    CM_DEFAULT = 0,
    CM_WOULD_BLOCK,
    CM_BROKEN_PIPE,
    CM_NEW_MESSAGE
};

enum MESSAGES
{
    MSG_DEFAULT = 0,
    MSG_BEGIN,
    MSG_END
};

//---------------------------------------------------

void enter_region(int* fds, char** fifos_names);

void leave_region(int* fds);

int capture_lock(char** fifos_names);

int is_fifo_dead(const char* fifo_name);

char check_mailbox(int nonblock_rd_fd, char* msg);

void send_message(int rdwr_fd, char msg);

void pump_data(int* fifo_fds, int inp_fd, int out_fd, int is_consumer);

// Filter to search for lock fifo using prefix
int lock_filter(const struct dirent* entry);

// Search for lock fifo using prefix
char* get_lock_name(void);

void make_fifos(void);

char** get_fifos_names(int is_consumer);

void close_fds(int* fds);

void producer_wait_for_consumer(int* fds);
