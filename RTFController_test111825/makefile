all: otsEthTest

otsEthTest: ../lib/EthernetInterface.cpp ../lib/EthernetInterface.h otsEthTest.cpp 
	g++ -O3 -o $@ $^ -lpthread

clean:
	rm otsEthTest
