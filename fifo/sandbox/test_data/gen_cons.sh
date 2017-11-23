#!bin/bash
cd ../
if [ $1 -eq 1 ] 
then
    for (( i=1; i <= $2; i++ )) 
    do
        strace -ttt ./fifo_program >> ./test_data/output_data 2> ./test_data/log_cons_$i &
    done
else
    for (( i=1; i <= $2; i++ )) 
    do
        ./fifo_program >> ./test_data/output_data &
    done
fi
