VPATH = ../src

CC = g++
CFLAGS = -g -pthread -Wall
OBJECTS = EventLoop.o util.o log.o poller.o channel.o

test1: $(OBJECTS) test1.o
	$(CC) $(CFLAGS) -o test1 $(OBJECTS) test1.o

test2: $(OBJECTS) test2.o
	$(CC) $(CFLAGS) -o test2 $(OBJECTS) test2.o

test3: $(OBJECTS) test3.o
	$(CC) $(CFLAGS) -o test3 $(OBJECTS) test3.o

test1.o: EventLoop.h util.h
test2.o: EventLoop.h
test3.o: EventLoop.h channel.h

clean:
	rm -f *.o test1 test2 test3 test4 test5