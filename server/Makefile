EXECUTE_WITH_STRACE=1
CHILDREN_NUMBER=3

CC=gcc
SOURCES=main.c
HEADERS=

all: compile

make: compile ex

compile:
	$(CC) $(FLAGS) $(SOURCES) -o ./server

ex: compile clean
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACE) $(CHILDREN_NUMBER)

check:
	ps | wc
	wc -l ./test_data/output_data

clean:
	rm ./test_data/log* 2> /dev/null || true

kill:
	pkill server || true
