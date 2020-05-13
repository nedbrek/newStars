#        1         2         3         4         5         6         7
#23456789012345678901234567890123456789012345678901234567890123456789012345678
.PHONY: all clean debug_exe

MXML:=../../mxml-2.2.2j
TCL_INC:=/usr/include/tcl8.6

CC=g++
SRCPATH=../../src
CFLAGS=-c -g -Wall -I$(SRCPATH) -I$(TCL_INC) -I$(MXML) -DUSE_TCL_STUBS -fPIC
LDFLAGS+=-L$(MXML)

SOURCES=newStars.cpp planet.cpp race.cpp opt.cpp \
 universe.cpp fleet.cpp orders.cpp placeableObject.cpp technology.cpp \
 structuralElement.cpp ship.cpp minefield.cpp reconcile.cpp \
 actionDispatcher.cpp battle.cpp gen.cpp

EXSRC=$(addprefix $(SRCPATH)/,$(SOURCES)) nstclgui.cpp

OBJECTS=$(EXSRC:.cpp=.o)

INXML=newStarsComponents.xml newStarsHulls.xml

all: nstclgui.so $(INXML) names.txt

-include Depend.Make

debug_exe: nstclguid.exe

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

nstclguid.exe: main.o nstclgui.so
	@g++ -g -o $@ $(LDFLAGS) -Wl,-rpath,\$$ORIGIN $^ -lmxml -lpthread -ltcl8.6 -ltk8.6

nstclgui.exe: main.o nstclgui.so
	@g++ -o $@ $(LDFLAGS) -Wl,-rpath,\$$ORIGIN $^ -lmxml -lpthread -ltcl8.6 -ltk8.6

nstclgui.so: $(OBJECTS)
	@g++ $(LDFLAGS) -shared -o $@ -Wl,-rpath,\$$ORIGIN $(OBJECTS) -lmxml -ltclstub8.6

Depend.Make: nstclgui.o
	@g++ -MM $(CFLAGS) nstclgui.cpp > Depend.Make

