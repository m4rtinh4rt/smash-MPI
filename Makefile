TARGET	:= smash-MPI.so
CC	:= mpicc
CFLAGS	:= -std=gnu99 -fPIC -Wall -Wextra -c -I.
LIB	:= -ldl
SRC	:= $(wildcard *.c)
OBJ	:= $(SRC:.c=.o)

.DEFAULT_GOAL	:= $(TARGET)

.PHONY: clean

$(TARGET): $(OBJ)
	$(CC) $(LIB) -shared $? -o $@

%.o: %.c
	$(CC) $(CFLAGS) $<

clean:
	@rm -f $(OBJ) $(TARGET)
