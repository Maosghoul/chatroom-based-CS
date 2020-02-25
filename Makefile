CPP = g++
CPPFLAGS = -std=c++11

all :server.cpp client.cpp
	$(CPP) $(CPPFLAGS) server.cpp -o server
	$(CPP) $(CPPFLAGS) client.cpp -o client

clean:
	rm -f *.o server client
