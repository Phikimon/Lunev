EXECUTE_WITH_STRACE=1

CC=gcc
SOURCES=main.c
HEADERS=

all: compile

make: compile exec

compile:
	$(CC) $(FLAGS) $(SOURCES) -o ./signals

exec: compile clean
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACE)

check:
	ps | wc
	wc -l ./test_data/output_data

clean:
	rm ./test_data/log* 2> /dev/null || true

kill:
	pkill signals || true
