#!bin/bash
cd ../
if [ $1 -eq 1 ]
then
    for (( i=1; i <= $2; i++ ))
    do
    strace -e trace=getpid,ipc,write,read -fv -ttt ./semaphores >> ./test_data/output_data 2> ./test_data/log_cons_$i.strace &
    done
else
    for (( i=1; i <= $2; i++ ))
    do
    ./semaphores >> ./test_data/output_data &
    done
fi
