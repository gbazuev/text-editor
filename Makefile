.PHONY: clean

CC = gcc	
CFLAGS = -Wall -Wextra -pedantic -std=c2x
EXECUTABLE := editor

objects = main.o terminal.o renderbuf.o search.o render.o \
		  io.o input.o hlfiletypes.o hldb.o hl.o \
		  erow.o editing.o config.o algo.o

$(EXECUTABLE): $(objects) 
	$(CC) -o $(EXECUTABLE) $(objects)
	rm *.o
	rm *.h.gch

main.o: main.c input.h render.h terminal.h config.h io.h
	$(CC) -c $(CFLAGS) main.c input.h render.h terminal.h config.h io.h

terminal.o: terminal.c keys.h config.h
	$(CC) -c $(CFLAGS) terminal.c

renderbuf.o: renderbuf.c
	$(CC) -c $(CFLAGS) renderbuf.c

search.o: search.c config.h keys.h hlhelpers.h input.h 
	$(CC) -c $(CFLAGS) search.c config.h keys.h hlhelpers.h input.h

render.o: render.c config.h renderbuf.h hlhelpers.h hl.h settings.h algo.h
	$(CC) -c $(CFLAGS) render.c config.h renderbuf.h hlhelpers.h hl.h settings.h algo.h

io.o: io.c config.h hl.h terminal.h render.h input.h
	$(CC) -c $(CFLAGS) io.c config.h hl.h terminal.h render.h input.h

input.o: input.c render.h keys.h erow.h terminal.h config.h \
	settings.h io.h editing.h search.h
	$(CC) -c $(CFLAGS) input.c render.h keys.h erow.h terminal.h \
		settings.h io.h editing.h search.h

hlfiletypes.o: hlfiletypes.c
	$(CC) -c $(CFLAGS) hlfiletypes.c

hldb.o: hldb.c hlfiletypes.h hlhelpers.h esyntax.h
	$(CC) -c $(CFLAGS) hldb.c hlfiletypes.h hlhelpers.h esyntax.h

hl.o: hl.c erow.h hlhelpers.h hldb.h esyntax.h config.h
	$(CC) -c $(CFLAGS) hl.c erow.h hlhelpers.h hldb.h esyntax.h config.h

erow.o: erow.c hl.h settings.h config.h algo.h
	$(CC) -c $(CFLAGS) erow.c hl.h settings.h config.h algo.h

editing.o: editing.c config.h
	$(CC) -c $(CFLAGS) editing.c config.h

config.o: config.c terminal.h
	$(CC) -c $(CFLAGS) config.c terminal.h

algo.o: algo.c
	$(CC) -c $(CFLAGS) algo.c

clean:
	rm $(EXECUTABLE)
