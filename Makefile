CC=gcc
CFLAGS= -Wall -Wextra -lgumbo -lcurl -g
OBJ = main.o network.o

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

mount-http-dir: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

.PHONY: clean

clean:
	rm -f *.o mount-http-dir
