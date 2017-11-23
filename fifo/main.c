#include "fifo.h"

int main(int argc, char* argv[])
{
    make_fifos();
    if ((argc < 1) || (argc > 2)) 
    {
        errno = EINVAL;
        perror("Wrong number of arguments");
        return 1;
    }
    int is_consumer = (argc == 1);
    char** fifos_names = get_fifos_names(is_consumer);
    int fds[FDS_NUM] = {0};
    enter_region(fds, fifos_names);
    //Open fds
    if (is_consumer)
    {
        fds[INPUT_FD] = open(DATA_FIFO, O_RDONLY|O_NONBLOCK);
        CHECK(open, fds[INPUT_FD]);
        fds[OUTPUT_FD] = STDOUT_FILENO;
    } else
    {
        fds[INPUT_FD]  = open(argv[1], O_RDONLY);
        fds[OUTPUT_FD] = open(DATA_FIFO, O_WRONLY|O_NONBLOCK);
        while (fds[OUTPUT_FD] == -1) 
        {
            if (errno != ENXIO)
            {
                perror("open");
                exit(1);
            }
            char msg = MSG_DEFAULT;
            int inbox_state = check_mailbox(fds[RD_SYNC_FIFO], &msg);
            if ((inbox_state == CM_NEW_MESSAGE) ||
                (inbox_state == CM_BROKEN_PIPE) )
            {
                send_message(fds[WR_SYNC_FIFO], MSG_END);
                exit(0);
            }
            fds[OUTPUT_FD] = open(DATA_FIFO, O_WRONLY|O_NONBLOCK);
        }
        int fcntl_ret = fcntl(fds[OUTPUT_FD], F_SETFL, O_WRONLY); //< Clear O_NONBLOCK flag
        CHECK(fcntl, fcntl_ret);
    }
    
    pump_data(fds, fds[INPUT_FD], fds[OUTPUT_FD], is_consumer);

    if (!is_consumer)
    {
        producer_wait_for_consumer(fds);
    }
    close_fds(fds);

    return 0;
}
