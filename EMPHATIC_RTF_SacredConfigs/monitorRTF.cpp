#include "../lib/EthernetInterface.h"

#include <iostream>
#include <iomanip>
#include <cstdio>
#include <ctime>
#include <thread>
#include <mutex>
#include <csignal>

bool stopRunning = false;

void my_handler(int s)
{
    printf("Caught signal %d\n",s);
    stopRunning = true;
}

class DataThread
{
private:
    std::mutex mtx_;
    std::thread thread_;

    bool keepRunning_;

    void awaitData()
    {
        EthernetInterface eth_burst("192.168.192.100", "5556");
        eth_burst.setBurstTarget();

        FILE* fout;
        fout = fopen("deltat.txt", "w");

        while(true)
        {
            try
            {
                std::vector<uint64_t> data = eth_burst.recieve_burst_single_packet(1, 00000);

                printf("Size: %lu\n", data.size());
                for(auto& datum : data)
                {
                    fprintf(fout, "%lu\n", datum);
                }
                fflush(fout);
            }
            catch(std::string& e)
            {
                //std::cout << e << std::endl;
                if(!keepRunning_) break;
            }
        }

        fclose(fout);
    }

public:

    DataThread() : thread_()
    {
        keepRunning_ = false;
    }

    void start()
    {
        keepRunning_ = true;
        thread_ = std::thread([this]{awaitData();});
    }

    void stop()
    {
        keepRunning_ = false;
    }

    void join()
    {
        if(thread_.joinable()) thread_.join();
    }
    
};

class Num256bit
{
private:
    uint32_t ints_[8];
public:
    friend Num256bit operator&(const Num256bit& rhs, const Num256bit& lhs);
    friend Num256bit operator|(const Num256bit& rhs, const Num256bit& lhs);
    friend Num256bit operator^(const Num256bit& rhs, const Num256bit& lhs);

    Num256bit() {}

    Num256bit(const Num256bit& rhs)
    {
        for(int i = 0; i < 8; ++i) ints_[i] = rhs.ints_[i];
    }

    Num256bit(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t e, uint32_t f, uint32_t g, uint32_t h)
    {
        ints_[7] = a;
        ints_[6] = b;
        ints_[5] = c;
        ints_[4] = d;
        ints_[3] = e;
        ints_[2] = f;
        ints_[1] = g;
        ints_[0] = h;
    }

    Num256bit operator~() const
    {
        Num256bit result;
        for(int i = 0; i < 8; ++i) result.ints_[i] = ~ints_[i];
        return result;
    }

    void operator=(const Num256bit& rhs)
    {
        for(int i = 0; i < 8; ++i) ints_[i] = rhs.ints_[i];
    }

    uint64_t operator[](const int& i) const
    {
        return (0x100000000 << i) | ints_[i];
    }

    void print()
    {
        printf("0x");
        for(int i = 7; i >= 0; --i) printf("%08x", ints_[i]);
        printf("\n");
    }
};

Num256bit operator&(const Num256bit& rhs, const Num256bit& lhs)
{
    Num256bit result;
    for(int i = 0; i < 8; ++i) result.ints_[i] = rhs.ints_[i] & lhs.ints_[i];
    return result;
}

Num256bit operator|(const Num256bit& rhs, const Num256bit& lhs)
{
    Num256bit result;
    for(int i = 0; i < 8; ++i) result.ints_[i] = rhs.ints_[i] | lhs.ints_[i];
    return result;
}

Num256bit operator^(const Num256bit& rhs, const Num256bit& lhs)
{
    Num256bit result;
    for(int i = 0; i < 8; ++i) result.ints_[i] = rhs.ints_[i] ^ lhs.ints_[i];
    return result;
}

void print_counters(EthernetInterface& eth){

    // Input counters
    std::cout << "Input counters: \n";
    for(int i = 0; i < 8; ++i)
    {
        printf("Input %d count: %10ld\n", i+1, eth.recieve(7+i*4));
    }
    std::cout << " Output counters: \n";
    for(int i = 0; i < 4; ++i)
    {
        printf("Output %d count: %10ld\n", i+1, eth.recieve(54+i*4));
    }
    std::cout<<std::endl;

    return;
}

int main()
{
    //ctrl+c handling
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    
    //ethernet interface instance
    EthernetInterface eth("192.168.192.100", "5555");

    //print clock frequencies
    std::cout<<"**** Clock configuration **** \n";
    std::cout << std::hex << (eth.recieve(1)>>3) << std::endl;
    printf("USER 1: %0.3f\n", (float)eth.recieve(100)/1000000.0);
    printf("USER 2: %0.3f\n", (float)eth.recieve(101)/1000000.0);
    printf("USER 3: %0.3f\n", (float)eth.recieve(102)/1000000.0);
    printf("USER 4: %0.3f\n", (float)eth.recieve(103)/1000000.0);
    printf("EXTERN: %0.3f\n", (float)eth.recieve(104)/1000000.0);
    printf("Sys160: %0.3f\n", (float)eth.recieve(105)/1000000.0);

    //start DAQ thread
    DataThread dt;
    dt.start();

    //reset counters and print
    eth.send(0, 0x8);
    usleep(10000);
    std::cout<<"**** Initial counter values **** \n";
    print_counters(eth);
    std::cout<<std::endl;


    // Start monitoring of counters
    while(!stopRunning){
        usleep(1000000);
        print_counters(eth);
    }
    
    //configure TAC
    eth.send(106, 1 | (3 << 4) | (uint64_t(3000) << 32));
    while(!stopRunning) usleep(10000000);
    eth.send(106, 0 | (3 << 4) | (uint64_t(3000) << 32));
    eth.setBurstMode(false);

    dt.stop();
    dt.join();

}
