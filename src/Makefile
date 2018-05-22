.PHONY: all clean reg

MXML:=(User supplied)

CC=g++
CPPFLAGS+=-c -g -Wall -fPIC -I$(MXML)
LDFLAGS+=-L$(MXML)

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
	@$(CC) -o $@ -Wl,-rpath,\$$ORIGIN $(LDFLAGS) $^ -lmxml -lpthread

.cpp.o:
	@$(CC) $(CPPFLAGS) $< -o $@

-include Depend.Make

clean:
	@rm -f $(OBJECTS) $(EXECUTABLE) *~ main.o

Depend.Make:
	@g++ -MM *.cpp > Depend.Make

