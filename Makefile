TARGET	:= smash-MPI.so
CC	:= mpicc
CFLAGS	:= -std=c99 -fPIC -Wall -c -I.
LIB	:= -ldl
SRC	:= hooking.c
OBJ	:= $(SRC:.c=.o)

.DEFAULT_GOAL	:= $(TARGET)

.PHONY: clean

$(TARGET): $(OBJ)
	$(CC) $(LIB) -shared $? -o $@

%.o: %.c
	$(CC) $(CFLAGS) $<

clean:
	@rm -f $(OBJ) $(TARGET)
