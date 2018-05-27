
AR ?= ar
CXX ?= g++
RM ?= rm

OBJS = filesystem.o

all: filesystem.a

filesystem.a: $(OBJS)
	$(AR) -r $@ $^

%.o: %.cpp
	$(CXX) -O2 -g -Wall -std=c++11 $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJS) filesystem.a

.PHONY: all clean

