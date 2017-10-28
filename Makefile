myshell : myshell.o execute.o globals.o parser.o
	cc -std=c99 -Wall -pedantic -Werror -o myshell myshell.o execute.o globals.o parser.o -lm

myshell.o : myshell.c myshell.h
	cc -std=c99 -Wall -pedantic -Werror -c myshell.c

execute.o : execute.c myshell.h
	cc -std=c99 -Wall -pedantic -Werror -c execute.c

globals.o : globals.c myshell.h
	cc -std=c99 -Wall -pedantic -Werror -c globals.c

parser.o : parser.c myshell.h
	cc -std=c99 -Wall -pedantic -Werror -c parser.c