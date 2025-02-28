all:
	g++ -o serverA serverA.cpp
	g++ -std=c++11 -o client client.cpp
	g++ -std=c++11 -o  serverM  serverM.cpp -pthread
	g++ -std=c++11 -o serverD serverD.cpp
	g++ -std=c++11 -o serverR serverR.cpp

clean:
	rm -f serverA client serverM serverD serverR