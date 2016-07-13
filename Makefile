###
  # Name: Geordie Wicks
  # Username: gwicks
  # StudentID: 185828
  #
  # Basic Makefile for comp30023 project2
  ##


## CC  = Compiler.
## CFLAGS = Compiler flags.
CC	= gcc


## OB = Object files.
## SR = Source files.
## EX = Executable name.

SR1 =		server.c
OB1 =		server.o
EX1 = 		server

SR2 =		client.c
OB2 =		client.o
EX2 = 		client

OBJ =		server.o client.o 

all: $(EX1) $(EX2)

## Top level target is executable.
$(EX1):	$(OB1)
		$(CC) $(CFLAGS) -o $(EX1) $(OB1) -lpthread

## Top level target is executable.
$(EX2):	$(OB2)
		$(CC) $(CFLAGS) -o $(EX2) $(OB2)


## Clean: Remove object files and core dump files.
clean:
		rm -f $(OBJ) $(EXE)

## Clobber: Performs Clean and removes executable files.

clobber: clean
		/bin/rm $(EX1) $(EX2) 
