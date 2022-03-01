CC=gcc

mysh: mysh.c linkedlist.h linkedlist.c node.h
	$(CC) -o mysh -Wall -Werror -g mysh.c linkedlist.c

clean:
	rm -f mysh
	rm -rf tester tests tests-out
