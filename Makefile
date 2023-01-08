OPT := -g
AR := ar
CC := gcc
CXX := g++

CXXFLAGS := -I.  $(OPT) 
LDFLAGS := -pthread
LIBS :=  

SRC_DIR = ./miniduo
# WORKSPACE := workspace

MINIDUO_SOURCES := $(shell find $(SRC_DIR) -name '*.cpp')
MINIDUO_HEADERS := $(shell find $(SRC_DIR) -name '*.h')
MINIDUO_OBJECTS := $(MINIDUO_SOURCES:.cpp=.o)

TEST_SOURCES := $(shell find test -name '*.cpp')
TESTS := $(TEST_SOURCES:.cpp=)

EXAMPLE_SOURCES := $(shell find examples -name '*.cpp')
EXAMPLES := $(EXAMPLE_SOURCES:.cpp=)

LIBRARY := libminiduo.a

TARGETS := $(LIBRARY) $(EXAMPLES) $(TESTS)

default: $(TARGETS)
miniduo_tests: $(TESTS)
miniduo_examples: $(EXAMPLES)

$(TESTS): $(LIBRARY)
$(EXAMPLES): $(LIBRARY)

install: $(LIBRARY)
	sudo mkdir -p /usr/local/include/miniduo
	sudo cp -f $(MINIDUO_HEADERS) /usr/local/include/miniduo
	sudo cp -f $(LIBRARY) /usr/local/lib

uninstall:
	sudo rm -rf /usr/local/include/miniduo /usr/local/lib/$(LIBRARY)

$(LIBRARY): $(MINIDUO_OBJECTS)
		rm -f $@
		$(AR) -rs $@ $(MINIDUO_OBJECTS)

.cpp.o:
		$(CXX) $(CXXFLAGS) -c $< -o $@

.cpp:
		$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)  $(LIBS) $(LIBRARY)




clean:
			-rm -f $(TARGETS)
			-rm -f $(MINIDUO_OBJECTS)
clean_tests:
	rm -f $(TESTS)

clean_examples:
	rm -f $(EXAMPLES)

clean_bins: clean_tests clean_examples

clean_objs:
	rm -f $(MINIDUO_OBJECTS)