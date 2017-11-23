#!bin/bash
# $1 = $(EXECUTE_WITH_STRACES)
# $2 = $(CHILDREN_NUMBER)
rm ./test_data/output_data 2> /dev/null
if [ $1 -eq 1 ]
then
    strace -fv -ttt ./server ./test_data/input_data $2 >> ./test_data/output_data 2> ./test_data/log.strace &
else
    ./server ./test_data/input_data $2 >> ./test_data/output_data &
fi
