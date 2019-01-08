CC=g++ -Wall

all: main

main: helper.o main.o
	$(CC) -pthread -o main helper.o main.o

main.o: helper.cc main.cc
	$(CC) -c helper.cc main.cc

queue.o: queue.cc
	$(CC) -c queue.cc

tidy:
	rm -f *.o core

clean:
	rm -f main producer consumer *.o core
#include "imap.hpp"
#include "UI.hpp"
#include <memory>

int main(int argc, char** argv) {
	auto elements = std::make_unique<UI>(argc, argv);
	return elements->exec();
}
