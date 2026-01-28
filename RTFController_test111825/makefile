all: otsEthTest otsEthTestWorking

otsEthTest: ../lib/EthernetInterface.cpp ../lib/EthernetInterface.h otsEthTest.cpp 
	g++ -O3 -o $@ $^ -lpthread

otsEthTestWorking: ../lib/EthernetInterface.cpp ../lib/EthernetInterface.h otsEthTestWorking.cpp
	g++ -O3 -o $@ $^ -lpthread

clean:
	rm otsEthTest otsEthTestWorking
