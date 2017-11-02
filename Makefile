# JAKRIN JUANGBHANICH
# MyShell MakeFile

PROJECT = myshell
HEADERS = $(PROJECT).h
OBJ = $(PROJECT).o execute.o globals.o parser.o

C99 = cc -std=c99
CFLAGS = -Wall -pedantic -Werror
COMPILE = $(C99) $(CFLAGS)

$(PROJECT): $(OBJ)
	$(COMPILE) -o $(PROJECT) $(OBJ) -lm

%.o : %.c $(HEADERS)
	$(COMPILE) -c $<

clean:
	rm -f $(PROJECT) $(OBJ)