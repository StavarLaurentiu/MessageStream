CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

PORT_SERVER = 12345

IP_SERVER = 127.0.0.1

ID_CLIENT = 4018

all: server subscriber

utils.o: utils.cpp

server: server.cpp utils.o

subscriber: subscriber.cpp utils.o

.PHONY: clean run_server run_subscriber

run_server:
	./server ${PORT_SERVER}

run_subscriber:
	./subscriber ${ID_CLIENT} ${IP_SERVER} ${PORT_SERVER}

clean:
	rm -f server subscriber