#!bin/bash
cd test_data
rm output_data 2> /dev/null
# $1 = $(EXECUTE_WITH_STRACES)
# $2 = number of processes to execute
bash gen_prod.sh $1 $2 &
bash gen_cons.sh $1 $2 &
