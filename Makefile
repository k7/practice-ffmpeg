CC = clang++
DEBUG = -g

CFLAGS = -Wall -std=c++2a
LDFLAGS = 

.SUFFIXES: .cc .o

% : src/%.cc
	mkdir -p $(PWD)/build
	$(CC) $(DEBUG) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o build/$@ $^
	build/$@ data/demo.gif

.PHONY : clean
clean:
	rm -rf $(PWD)/build
