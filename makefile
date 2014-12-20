CC=g++
CFLAGS=-c -Wall -g -O3 -std=c++11 -Wextra -march=native -mtune=native
LDFLAGS=-lpthread -lfreeimage
EXECUTABLE=bin/img_edit
DIRS=bin/ build/

all: $(EXECUTABLE)

$(EXECUTABLE): build/main.o
	mkdir -p $(DIRS)
	$(CC) build/main.o -o $(EXECUTABLE) $(LDFLAGS)

build/main.o: src/main.cpp
	mkdir -p $(DIRS)
	$(CC) $(CFLAGS) src/main.cpp -o build/main.o

clean:
	rm build/*.o -f
	rm $(EXECUTABLE) -f
