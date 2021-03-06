# super simple makefile
# requires CAENVMElib

CC	= g++
CFLAGS	= -Wall -g -O -DLINUX -fPIC -std=c++11
LDFLAGS = -lCAENVME -lncurses
SOURCES = $(shell echo ./*cc)
OBJECTS = $(SOURCES: .cc=.o)
CPP	= main

all: $(SOURCES) $(CPP)

$(CPP) : $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(CPP)

clean:
	rm $(CPP)
