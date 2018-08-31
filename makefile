server : server.cpp
	gcc -o server server.cpp -lpthread -std=c++11 -lstdc++

clean:
	rm server
