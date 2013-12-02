APP  := timer.coverage
SRC  := ../src/
OBJS := ltimer.o main.o

CFLAGS += -g -O0 --coverage

coverage: clean $(APP)
	rm -rf ./out
	./$(APP)
	lcov --capture --directory ./ --output-file coverage.info
	genhtml coverage.info --output-directory out

$(APP): $(OBJS) Makefile
	gcc $(CFLAGS) $(OBJS) -o $(APP)
	
ltimer.h:
	ln -sf $(SRC)/ltimer.h ltimer.h
	
ltimer.c: ltimer.h
	ln -sf $(SRC)/ltimer.c ltimer.c
		
clean:
	rm -f *.o $(APP) coverage.info *.gcno *.gcna
	
		