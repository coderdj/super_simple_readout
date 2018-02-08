# super simple makefile
# requires CAENVMElib

CC	= g++
CFLAGS	= -Wall -g -DLINUX -fPIC -std=c++11 $(shell root-config --cflags)
LDFLAGS = -lCAENVME $(shell root-config --libs) $(shell root-config --glibs)
SOURCES = $(shell echo ./*cc)
OBJECTS = $(SOURCES: .cc=.o)
CPP	= main

all: $(SOURCES) $(CPP)

$(CPP) : $(OBJECTS)
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) $(LIBS) -o $(CPP)

clean:
	rm $(CPP)
