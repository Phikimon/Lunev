EXECUTE_WITH_STRACES=0

all:

make: compile easy check

compile:
	gcc ../main.c ../fifo.c -o ./fifo_program

check_for_locks_existence:
	test -e CONS*
	test -e PROD*

# Every of following targets(basic, easy, medium, hard, die)
# shall be called in such a way:
# make FILE=<file_name:small_data,big_data> <target>
basic: check_for_locks_existence compile 
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACES) 1 $(FILE)

easy: check_for_locks_existence compile 
	bash -x ./test_data/test.sh $(EXECUTE_WITH_STRACES) 10 $(FILE)

medium: check_for_locks_existence compile 
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACES) 100 $(FILE)

hard: check_for_locks_existence compile 
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACES) 300 $(FILE)

die: check_for_locks_existence compile 
	bash ./test_data/test.sh $(EXECUTE_WITH_STRACES) 1000 $(FILE)

check:
	ps | wc
	wc -l ./test_data/output_data

clean:
	rm ./log*

plog:
	tail -n 30 ./test_data/log_prod_$(log)$(LOG)

clog:
	tail -n 30 ./test_data/log_cons_$(log)$(LOG)

kill:
	pkill fifo

find:
	grep "= "${find}${FIND} ./test_data/log_*
