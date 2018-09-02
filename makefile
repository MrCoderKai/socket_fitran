server : server.o utils.o
	g++ -o server server.o utils.o -std=c++11 -pthread

client : client.o utils.o
	g++ -o client client.o utils.o -std=c++11 -pthread

server.o : server.cpp common.h
	g++ -c server.cpp common.h -std=c++11 -pthread

client.o : client.cpp common.h
	g++ -c client.cpp common.h -std=c++11 -pthread

utils.o : common.h utils.cpp
	g++ -c common.h utils.cpp -std=c++11 -pthread

clean:
	rm server client *.o
