EXECUTE_WITH_STRACES=0

CC=gcc
SOURCES=main.c
HEADERS=

all: compile

make: compile easy

compile:
	$(CC) $(FLAGS) $(SOURCES) -o ./semaphores

basic: compile clean
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACES) 1

easy: compile clean
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACES) 5

medium: compile clean
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACES) 100

hard: compile clean
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACES) 300

die: compile clean
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACES) 1000

check:
	ps | wc
	wc -l ./test_data/output_data
	ipcs

clean:
	rm ./test_data/log* 2> /dev/null || true

rm:
	ipcrm -S 0xdeadbeef 2> /dev/null || true
	ipcrm --all=shm 2> /dev/null || true
	make check

plog:
	tail -n 30 ./test_data/log_prod_$(log)$(LOG)

clog:
	tail -n 30 ./test_data/log_cons_$(log)$(LOG)

kill:
	pkill semaphores || true

# This is very useful for me,
# because I don't have '|' on
# my keyboard and every time
# I have to turn on Num lock
# and type Alt + 1-2-4
# (124 == '|')
ps:
	ps | less

find:
	grep "= "${find}${FIND} ./test_data/*
