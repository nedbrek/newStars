#        1         2         3         4         5         6         7
#23456789012345678901234567890123456789012345678901234567890123456789012345678
.PHONY: all clean debug_exe

TCL_INC=/usr/include/tcl8.5

CC=g++
SRCPATH=../../src
CFLAGS=-c -g -Wall -I$(SRCPATH) -I$(TCL_INC) -DUSE_TCL_STUBS -fPIC
#CFLAGS+=-mno-cygwin
#LDFLAGS=-mno-cygwin

SOURCES=newStars.cpp planet.cpp race.cpp opt.cpp \
 universe.cpp fleet.cpp orders.cpp placeableObject.cpp technology.cpp \
 structuralElement.cpp ship.cpp minefield.cpp reconcile.cpp \
 actionDispatcher.cpp battle.cpp gen.cpp

EXSRC=$(addprefix $(SRCPATH)/,$(SOURCES)) nstclgui.cpp

OBJECTS=$(EXSRC:.cpp=.o)

INXML=newStarsComponents.xml newStarsHulls.xml

all: nstclgui.so $(INXML) names.txt

-include Depend.Make

debug_exe: bin/nstclguid.exe

names.txt: $(SRCPATH)/names.txt
	@cp $< .

newStarsComponents.xml: $(SRCPATH)/newStarsComponents.xml
	@cp $< .

newStarsHulls.xml: $(SRCPATH)/newStarsHulls.xml
	@cp $< .
	
clean:
	@rm -f *.o *.so nstclgui.exe

.cpp.o:
	@$(CC) $(CFLAGS) $< -o $@

bin/nstclguid.exe: $(OBJECTS) main.cpp
#	@g++ -g -o $@ main.cpp $(OBJECTS) $(SRCPATH)/libmxml.a /usr/src/tcltk-20060202-1/tk/win/Debug/tk84d.lib /usr/src/tcltk-20060202-1/tcl/win/Debug/tcl84d.lib

bin/nstclgui.exe: $(OBJECTS) main.cpp
#	@g++ -o $@ main.cpp $(OBJECTS) $(SRCPATH)/libmxml.a -ltcl -ltk

nstclgui.so: $(OBJECTS)
	@g++ $(LDFLAGS) -shared -o $@ $(OBJECTS) -lmxml -ltclstub8.5

Depend.Make: nstclgui.o
	@g++ -MM $(CFLAGS) nstclgui.cpp > Depend.Make

