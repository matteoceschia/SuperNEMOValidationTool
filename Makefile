CC=g++
INCLUDES=-I/Users/cpatrick/CadfaelBrew/include
CFLAGS=-c -g -Wall `root-config --cflags`
LDFLAGS=`root-config --glibs` -L/Users/cpatrick/CadfaelBrew/lib/ -lboost_filesystem-mt -lboost_system-mt
OBJECTS=$(SOURCES:.cc=.o)

# make a binary for every .cxx file
all : $(patsubst %.cxx, %.o, $(wildcard *.cxx))

$(EXECUTABLE): $(OBJECTS)
	$(CC)  $(LDFLAGS) $(OBJECTS) -o $@

%.o : %.cxx 
	$(CC) $(INCLUDES) ${CFLAGS} -o $@ $<

clean:
	rm -f $(wildcard *.o) $(patsubst %.cxx, %, $(wildcard *.cxx))

