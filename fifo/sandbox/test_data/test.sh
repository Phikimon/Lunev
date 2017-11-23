#!bin/bash
cd test_data
rm output_data 2> /dev/null
bash gen_prod.sh $1 $2 $3 &
bash gen_cons.sh $1 $2 &
