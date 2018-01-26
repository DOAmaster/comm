all: server3 client1

server3: server3.cpp
	g++ server3.cpp -Wall -oserver3

client1: client1.cpp
	g++ client1.cpp -Wall -oclient1

clean:
	rm -f server3 client1


