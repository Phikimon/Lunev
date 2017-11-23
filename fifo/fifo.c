#include "fifo.h"

const char DATA_FIFO  [] = "DATA_FIFO";
      char LOCK_PREFIX[] = "NAME_LOCK_";

void enter_region(int* fds, char** fifos_names)
{
    fds[LOCK]         = capture_lock(fifos_names);

    fds[RD_SYNC_FIFO] = open(fifos_names[RD_SYNC_FIFO], O_RDONLY|O_NONBLOCK);
    // O_RDWR is safe here because there is only 1 byte at a time in sync fifo
    fds[WR_SYNC_FIFO] = open(fifos_names[WR_SYNC_FIFO], O_RDWR); 
    //Signal that you were born:
    send_message(fds[WR_SYNC_FIFO], MSG_BEGIN);
    char msg = MSG_DEFAULT;
    char inbox_state = CM_DEFAULT;
    while ((inbox_state != CM_NEW_MESSAGE) ||
                   (msg == MSG_END)        )
    {
        inbox_state = check_mailbox(fds[RD_SYNC_FIFO], &msg);
        if (msg == MSG_END)
        {
            // If message from the process who is now aborting
            // arrived then resend previous message to new,
            // valid addressee
            send_message(fds[WR_SYNC_FIFO], MSG_BEGIN);
        }
    }
}

void producer_wait_for_consumer(int* fds)
{
    char msg = MSG_DEFAULT;
    //Signal that producer is ready to abort
    send_message(fds[WR_SYNC_FIFO], MSG_END);
    char inbox_state = CM_DEFAULT;
    while ( (inbox_state != CM_NEW_MESSAGE) &&
            (inbox_state != CM_BROKEN_PIPE) )
    {
        inbox_state = check_mailbox(fds[RD_SYNC_FIFO], &msg);
    }
}

int capture_lock(char** fifos_names)
{
    while (1)
    {
        usleep(500000);
        const char* cur_lock_name = get_lock_name();
        if (!is_fifo_dead(cur_lock_name))
        {
            continue;
        }
        int fifod = open(cur_lock_name, O_RDWR);
        if (fifod == -1)
        {
            if (errno == ENOENT)
            {
                continue;
            } else
            {
                perror("open");
                exit(1);
            }
        }
        int rename_ret = rename(cur_lock_name, fifos_names[LOCK]);
        if (rename_ret == 0)
        {
            //Successfully captured
            return fifod;
        } else
        if ((rename_ret == -1) && (errno == ENOENT))
        {
            //Not this time
            CLOSE(fifod);
            continue;
        } else
        {
            perror("rename");
            exit(1);
        }
    }
}

int is_fifo_dead(const char* fifo_name)
{
    int fifod = open(fifo_name, O_WRONLY|O_NONBLOCK);
    if (fifod > 0)
    {
        CLOSE(fifod);
        return 0;
    } else
    if (errno == ENXIO)
    {
        return 1;
    } else
    if (errno == ENOENT)
    {
        return 0;
    } else
    {
        perror("open");
        exit(1);
    }
}

char check_mailbox(int nonblock_rd_fd, char* msg)
{
    *msg = MSG_DEFAULT;
    char inbox = 0;
    int read_ret = read(nonblock_rd_fd, &inbox, sizeof(char));
    if (read_ret > 0)
    {
        *msg = inbox;
        return CM_NEW_MESSAGE;
    } else
    if (read_ret == 0)
    {
        return CM_BROKEN_PIPE;
    } else
    if ((read_ret == -1) && (errno == EAGAIN))
    {
        return CM_WOULD_BLOCK;
    } else
    {
        perror("read");
        exit(1);
    }
}

void send_message(int rdwr_fd, char msg)
{
    int write_ret = write(rdwr_fd, &msg, sizeof(char));
    if (write_ret == -1)
    {
        perror("write");
        exit(1);
    }
}

void pump_data(int* fifo_fds, int inp_fd, int out_fd, int is_consumer)
{
    char buffer[BUF_SIZE_READ + 1] = {0};
    buffer[BUF_SIZE_READ] = '\0';

    ssize_t read_ret = -1;
    if (is_consumer)
    {
        int can_leave = 0;
        while ((read_ret != 0) || (can_leave == 0))
        {
            read_ret = read(inp_fd, buffer, BUF_SIZE_READ);
            if ( ((read_ret == -1) && (errno == EAGAIN)) ||  //< Producer finished writing,
                             (read_ret == 0)             )   //< didn't open fifo yet or died
            {
                char msg = MSG_DEFAULT;
                int inbox_st = check_mailbox(fifo_fds[RD_SYNC_FIFO], &msg);
                can_leave = (inbox_st == CM_NEW_MESSAGE) || 
                            (inbox_st == CM_BROKEN_PIPE);
                if (inbox_st == CM_NEW_MESSAGE)
                {
                    send_message(fifo_fds[WR_SYNC_FIFO], MSG_END);
                }
            } else
            {
                write(out_fd, buffer, read_ret);
            }
        };
    } else
    {
        while (read_ret != 0)
        {
            read_ret = read(inp_fd, buffer, BUF_SIZE_READ);
            CHECK(read, read_ret);
            write(out_fd, buffer, read_ret);
        };
    }
}

// Filter to search for lock fifo using prefix
int lock_filter(const struct dirent* entry)
{
    if ((entry->d_type == DT_FIFO) &&
        (strstr(entry->d_name, LOCK_PREFIX) == entry->d_name))
    {
        return 1;
    } else
    {
        return 0;
    }
}

// Search for lock fifo using prefix
char* get_lock_name(void)
{
    struct dirent** namelist = NULL;

    int n = scandir(".", &namelist, lock_filter, NULL);
    if (n == -1)
    {
        perror("scandir");
        exit(1);
    } else if (n == 0)
    {
        fprintf(stderr, "Lock not found!\n");
        exit(1);
    } else if (n > 1)
    {
        fprintf(stderr, "Several locks found O_O\n");
        exit(1);
    }
    static char lock_name[MAX_STR_LEN] = {};
    strcpy(lock_name, namelist[0]->d_name);
    free(namelist[0]);
    free(namelist);
    return lock_name;
}

void make_fifos(void)
{
#define MKFIFO(name)                 \
do {                                 \
    mkfifo_ret = mkfifo(name, 0644); \
    if ((mkfifo_ret == -1) &&        \
        (errno != EEXIST)  )         \
    {                                \
        perror("mkfifo");            \
        exit(1);                     \
    }                                \
} while(0)
    int mkfifo_ret = -1;
    MKFIFO(DATA_FIFO);
    MKFIFO("SYNC_FIFO1");
    MKFIFO("SYNC_FIFO2"); 
#undef MKFIFO 
}

char** get_fifos_names(int is_consumer)
{
    static char lock        [MAX_STR_LEN] = {};
    static char wr_sync_fifo[MAX_STR_LEN] = {};
    static char rd_sync_fifo[MAX_STR_LEN] = {};
    if (is_consumer)
    {
        sprintf(LOCK_PREFIX,  "CONS");
        sprintf(lock,         "CONS_LOCK_%d", getpid());
        sprintf(wr_sync_fifo, "SYNC_FIFO1");
        sprintf(rd_sync_fifo, "SYNC_FIFO2");
    } else
    {
        sprintf(LOCK_PREFIX,  "PROD");
        sprintf(lock,         "PROD_LOCK_%d", getpid());
        sprintf(wr_sync_fifo, "SYNC_FIFO2");
        sprintf(rd_sync_fifo, "SYNC_FIFO1");
    }
    static char* fifos_names[] = { lock, wr_sync_fifo, rd_sync_fifo };
    return fifos_names;
}

void close_fds(int* fds)
{
    CLOSE(fds[INPUT_FD]);
    CLOSE(fds[OUTPUT_FD]);
    CLOSE(fds[RD_SYNC_FIFO]);
    CLOSE(fds[WR_SYNC_FIFO]);
    CLOSE(fds[LOCK]);
}
