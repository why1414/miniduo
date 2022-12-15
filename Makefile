OPT ?= -g2
AR := ar
CC := cc
CXX := g++

CFLAGS := -I. -pthread  -std=c++11  $(OPT)
CXXFLAGS := -I. -pthread  -std=c++11  $(OPT)

LDFLAGS := -pthread -lminiduo
LIBS :=  

SRC_DIR = miniduo
WORKSPACE := workspace

MINIDUO_SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')
MINIDUO_OBJECTS = $(MINIDUO_SOURCES:.cpp=.o)

TEST_SOURCES = $(shell find test -name '*.cpp')
TESTS = $(TEST_SOURCES:.cpp=)

EXAMPLE_SOURCES := $(shell find examples -name '*.cpp')
EXAMPLES = $(EXAMPLE_SOURCES:.cpp=)

LIBRARY = libminiduo.a

TARGETS = $(LIBRARY) $(EXAMPLES) $(TESTS)

default: $(TARGETS)
miniduo_tests: $(TESTS)
miniduo_examples: $(EXAMPLES)
$(TESTS): 
$(EXAMPLES): 

install: $(LIBRARY)
	sudo mkdir -p /usr/local/include/miniduo
	sudo cp -f $(SRC_DIR)/*.h /usr/local/include/miniduo
	sudo cp -f $(LIBRARY) /usr/local/lib

uninstall:
	sudo rm -rf /usr/local/include/miniduo /usr/local/lib/$(LIBRARY)

clean:
			-rm -f $(TARGETS)
			-rm -f */*.o

$(LIBRARY): $(MINIDUO_OBJECTS)
		rm -f $@
		$(AR) -rs $@ $(MINIDUO_OBJECTS)

.cpp.o:
		$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
		$(CC) $(CFLAGS) -c $< -o $@

.cpp:
		$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)  $(LIBS)
