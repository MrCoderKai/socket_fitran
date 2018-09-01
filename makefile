server : server.cpp common.h
	g++ -o server server.cpp common.h -std=c++11 -pthread

client : client.cpp common.h
	g++ -o client client.cpp common.h -std=c++11 -pthread

clean:
	rm server client
