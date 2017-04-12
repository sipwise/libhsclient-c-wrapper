CC=g++
CXXFLAGS+=-I../include -g -Wall -O3 -fPIC -std=c++11
LDFLAGS=-lhsclient -O3 -shared -fPIC -Wl,-soname,$(LIBNAME).so.$(VERSION)

CPPFILES=$(wildcard *.cpp)
OFILES=$(CPPFILES:.cpp=.o)


.PHONY: all clean

all: $(LIBSO)

$(LIBSO): $(LIBSOVER)
	ln -fs $(LIBSOVER) $(LIBSO)

$(LIBSOVER): $(OFILES)
	$(CC) -o $@ $(OFILES) $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OFILES)
	rm -f $(LIBSO)
	rm -f $(LIBSOVER)
