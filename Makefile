CC = clang++
DEBUG = -g

CFLAGS = -Wall -std=c++2a
LDFLAGS = 

.SUFFIXES: .cc .o

gif_parser : src/gif_parser.cc
	mkdir -p $(PWD)/build
	$(CC) $(DEBUG) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o build/$@ $^
	build/$@ data/demo.gif

mp4_parser : src/mp4_parser.cc
	mkdir -p $(PWD)/build
	$(CC) $(DEBUG) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o build/$@ $^
	build/$@ data/demo.mp4

.PHONY : clean
clean:
	rm -rf $(PWD)/build
