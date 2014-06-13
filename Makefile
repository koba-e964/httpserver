EXEC = httpserver
OBJ  = httpserver.o serve.o

$(EXEC) : $(OBJ)
	gcc -o $(EXEC) $(OBJ)
%.o : %.c
	gcc -c -O2 -Wall -o $@ $<
clean:
	rm -rf $(EXEC) *.o *~
