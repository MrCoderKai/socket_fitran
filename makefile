server : server.cpp common.h
	gcc -o server server.cpp common.h -lpthread -std=c++11 -lstdc++

clean:
	rm server
