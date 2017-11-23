#!bin/bash
cd ../
if [ $1 -eq 1 ]
then
    for (( i=1; i <= $2; i++ ))
    do
    strace -e trace=getpid,ipc,write,read -fv -ttt ./semaphores ./test_data/input_data 2> ./test_data/log_prod_$i.strace &
    done
else
    for (( i=1; i <= $2; i++ ))
    do
    ./semaphores ./test_data/input_data &
    done
fi
