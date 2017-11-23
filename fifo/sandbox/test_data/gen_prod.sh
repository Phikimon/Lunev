#!bin/bash
cd ../
if [ $1 -eq 1 ] 
then
    for (( i=1; i <= $2; i++ )) 
    do
        strace -ttt ./fifo_program $3 2> ./test_data/log_prod_$i &
    done
else
    for (( i=1; i <= $2; i++ )) 
    do
        ./fifo_program $3 &
    done
fi
