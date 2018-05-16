.PHONY: all clean reg

CC=g++
CFLAGS=-c -g -Wall -fPIC
#CFLAGS+=-mno-cygwin
#LDFLAGS+=-mno-cygwin

HEADERS=reconcile.h newStars.h planet.h classDef.h race.h opt.h actionDispatcher.h
SOURCES=newStars.cpp planet.cpp race.cpp opt.cpp actionDispatcher.cpp \
        universe.cpp fleet.cpp orders.cpp placeableObject.cpp \
        technology.cpp structuralElement.cpp battle.cpp \
        minefield.cpp ship.cpp reconcile.cpp gen.cpp

OBJECTS=$(SOURCES:.cpp=.o)

EXECUTABLE=newStars.exe

all: $(EXECUTABLE)

gen_test.exe: gen_test.o $(OBJECTS) gen.o
	@$(CC) $(LDFLAGS) $^ -lmxml -o $@

$(EXECUTABLE): $(OBJECTS) main.o
	@$(CC) $(LDFLAGS) $^ -lmxml -o $@

.cpp.o:
	@$(CC) $(CFLAGS) $< -o $@

-include Depend.Make

clean:
	@rm -f $(OBJECTS) $(EXECUTABLE) *~

Depend.Make:
	@g++ -MM *.cpp > Depend.Make

