CC=gcc
TARGET=bsh
RM=rm
RMFLAGS = -f

all: bsh

bsh: bsh.c
		$(CC) -lreadline bsh.c -o $(TARGET)

clean:
	$(RM) $(RMFLAGS) $(TARGET)