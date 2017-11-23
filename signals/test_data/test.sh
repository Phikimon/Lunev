#!bin/bash
# $1 = $(EXECUTE_WITH_STRACES)
rm ./test_data/output_data 2> /dev/null
if [ $1 -eq 1 ]
then
    strace -e trace=signal,write,read,getpid,getppid,nanosleep,fork -fv -ttt ./signals ./test_data/input_data  >> ./test_data/output_data 2> ./test_data/log.strace &
else
    ./signals >> ./test_data/output_data &
fi
