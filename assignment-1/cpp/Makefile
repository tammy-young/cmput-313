p1client: client-p1.cpp
	g++ -o p1client client-p1.cpp ttt.cpp -std=c++11 -Wall

p1server: server-p1.cpp ttt.cpp 
	g++ -o p1server ttt.cpp server-p1.cpp -std=c++11 -Wall

p2client: client-p2.cpp
	g++ -o p2client client-p2.cpp ttt.cpp -pthread -std=c++11 -Wall

p2server: server-p2.cpp ttt.cpp
	g++ -o p2server ttt.cpp server-p2.cpp -pthread -std=c++11 -Wall

compile: p1client p1server p2client p2server

clean:
	rm -f p1client p1server p2client p2server