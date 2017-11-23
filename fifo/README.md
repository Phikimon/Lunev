## Initialization
```sh
.../fifo$ cd ./sandbox
.../fifo/sandbox$ mkfifo CONS
.../fifo/sandbox$ mkfifo PROD
.../fifo/sandbox$ cd ./test_data
.../fifo/sandbox/test_data$ seq 1000000 > big_data
```

## Test
### Semi-automatically *(recommended)*
```sh
.../fifo/sandbox$ export FILE=./test_data/small_data
.../fifo/sandbox$ make make
# Please see another possible targets :)
.../fifo/sandbox$ make check
```

### Manually
```sh
.../fifo/sandbox$ make compile
.../fifo/sandbox$ ./fifo_program &
.../fifo/sandbox$ ./fifo_program ./test_data/small_data &
.../fifo/sandbox$ ps # To ensure that all processes finally died
```
