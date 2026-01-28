all: configureRTF

configureRTF: ../lib/EthernetInterface.cpp ../lib/EthernetInterface.h configureRTF.cpp 
	g++ -O3 -o $@ $^ -lpthread

clean:
	rm configureRTF


all: monitorRTF

monitorRTF: ../lib/EthernetInterface.cpp ../lib/EthernetInterface.h monitorRTF.cpp 
	g++ -O3 -o $@ $^ -lpthread

clean:
	rm monitorRTF
