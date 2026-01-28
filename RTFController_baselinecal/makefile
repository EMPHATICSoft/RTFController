all: otsEthTest otsEthTest_Monitor

otsEthTest: ../lib/EthernetInterface.cpp ../lib/EthernetInterface.h otsEthTest.cpp 
	g++ -O3 -o $@ $^ -lpthread

otsEthTest_Monitor: ../lib/EthernetInterface.cpp ../lib/EthernetInterface.h otsEthTest_Monitor.cpp 
	g++ -O3 -o $@ $^ -lpthread

clean:
	rm otsEthTest
