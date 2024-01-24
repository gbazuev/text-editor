CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c2x

editor: editor.c
	$(CC) editor.c $(CFLAGS) -o editor.o

TODO: make goals for every file (every part like io, config etc.)
