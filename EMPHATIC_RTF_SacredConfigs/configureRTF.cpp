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


int main()
{
    //ctrl+c handling
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);
    
    // ----- Ethernet interface instance ------
    EthernetInterface eth("192.168.192.100", "5555");

    // -----  Set input DACs ----- 
    // Configure DAC thresholds for the 8 input channels
    // Empirically derived DAC value. Works with an input NIM signal
    uint32_t dac_value = 1250;
    uint32_t dac_settings[] = {dac_value,dac_value,dac_value,dac_value,dac_value,dac_value,dac_value,dac_value};
    for(int i = 0; i < 8; ++i)
    {
        eth.send(2, (i << 12) | dac_settings[i]);
        //printf("%d  %x\n", i, dac_settings[i]);
        eth.send(1, 0x1);
        eth.send(1, 0x2);
        usleep(20);
    }

    // -----  Configure lookup tables -----
    // Currently configured to not do any logic operations
    // Simply takes inputs from channels 1,2,3, and 4 and derives them to the outputs (internal) 1,2,3 and 4
    const Num256bit LUT8_I1(0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa);
    const Num256bit LUT8_I2(0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc, 0xcccccccc);
    const Num256bit LUT8_I3(0xf0f0f0f0, 0xf0f0f0f0, 0xf0f0f0f0, 0xf0f0f0f0, 0xf0f0f0f0, 0xf0f0f0f0, 0xf0f0f0f0, 0xf0f0f0f0);
    const Num256bit LUT8_I4(0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00, 0xff00ff00);
    const Num256bit LUT8_I5(0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000, 0xffff0000);
    const Num256bit LUT8_I6(0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0xffffffff, 0x00000000, 0xffffffff, 0x00000000);
    const Num256bit LUT8_I7(0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000);
    const Num256bit LUT8_I8(0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0x00000000, 0x00000000, 0x00000000, 0x00000000);

    Num256bit lut8_table1 = LUT8_I1;
    Num256bit lut8_table2 = LUT8_I2;
    Num256bit lut8_table3 = LUT8_I3;
    Num256bit lut8_table4 = LUT8_I4;

    //set output 1 LUTs
    for(int i = 0; i < 8; ++i)
    {
        eth.send(52, lut8_table1[i]);
        //printf("%8d  %016lx\n", i, lut8_table1[i]);
    }
    eth.send(52, 0x20000000000 | 0xaaaaaaaa);  //output LUT, select I1
    //set output 2 LUTs
    for(int i = 0; i < 8; ++i)
    {
        eth.send(56, lut8_table2[i]);
    }
    eth.send(56, 0x20000000000 | 0xaaaaaaaa);  //output LUT, select I1
    //set output 3 LUTs
    for(int i = 0; i < 8; ++i)
    {
        eth.send(60, lut8_table3[i]);
    }
    eth.send(60, 0x20000000000 | 0xaaaaaaaa);  //output LUT, select I1
    // set output 4 LUTs
    for(int i = 0; i < 8; ++i)
    {
        eth.send(64, lut8_table4[i]);
    }
    eth.send(64, 0x20000000000 | 0xaaaaaaaa);  //output LUT, select I1

    
    // -----  Configure input signals -----
    /*
    Registers 4-51 to configure the input signals
    12 inputs x 4 register each
    First 8 inputs correspond to the LEMO inputs in the NIM+

    Formatting
    // Register 3
    logic [31:0]      padding3;
    logic [31:0]      count;
    // Register 2
    logic [55:0]      padding2;
    logic [7:0]       delay;
    // Register 1
    logic [63:0]      stretch;
    // Register 0
    logic [46:0]      padding0;
    logic             invert;
    logic [7:0]       mask;
    logic [7:0]       trig_pattern;
    */

    // Registers 
    // No delay applied, strech 7 clock cycles
    // Trigger pattern:
    // 10f03 - 0001 00001111 00000011
    // Invert input signal, then look for 4 consecutive bits down-down-up-up

    //input 1 settings
    eth.send( 4, 0x10f03); //trig 
    eth.send( 5, 0x7); //stretch
    eth.send( 6, 0x0); //delay
    //input 2 settings
    eth.send( 8, 0x10f03); //trig 
    eth.send( 9, 0x7); //stretch
    eth.send(10, 0x0); //delay
    //input 3 settings
    eth.send(12, 0x10f03); //trig 
    eth.send(13, 0x7); //stretch
    eth.send(14, 0x0); //delay
    //input 4 settings
    eth.send(16, 0x10f03); //trig 
    eth.send(17, 0x7); //stretch
    eth.send(18, 0x0); //delay

    // -----  Configure pulse generator -----  
    // Not really used at this point
    eth.send(98, 0x50010);


    // -----  Configure clocks -----  
    // Register 3 structure
    /*
    logic [7:0]       clkout5_divide;
    logic [7:0]       clkout4_divide;
    logic [7:0]       clkout3_divide;
    logic [7:0]       clkout2_divide;
    logic [7:0]       clkout1_divide;
    logic [7:0]       clkout0_divide;
    logic [7:0]       divclk_divide;
    logic [7:0]       clkfbout_mult;
    */
    // 0x0404041e05100105
    // 00000100  00000100  00000100  00011110  00000101  00010000  00000001  00000101
    // 4 - 4 - 30 - 5 - 16 - 1 - 5
    // Base frequency is 160 MHz
    // Common factor: 5/1 x 40 MHz = 800 MHz
    // User 1: 800 MHz/16 = 50 MHz
    // User 2: 800 MHz/5 = 160 MHz
    // User 3: 800 MHz/30 = 26.6 MHz
    // User 4: 800 MHz/4 = 200 MHz
    eth.send(3, 0x0404041e05100105);
    // Reset PLL user
    eth.send(1, 0x4);

    // -----  Configure the output multiplexors -----
    // Define what to put in the different output channels
    // Menu
    /*
        0x000 - Trigger output 1
        0x001 - Trigger output 2
        0x002 - Trigger output 3
        0x003 - Trigger output 4
        0x004 - User clock 1
        0x005 - User clock 2
        0x006 - User clock 3
        0x007 - User clock 4
        0x008 - Pulse generator
    */

    // Registers 68-71 correspond to the NIM/TTL outputs in the back of the RTF
    eth.send(68, 0x101); // Trigger output
    eth.send(69, 0x104); // User clock 1
    eth.send(70, 0x106); // User clock 3
    eth.send(71, 0x107); // User clock 4
    
    // Clock distribution to ARICH
    // Using RJ45 output ports 8-11 & 32-35, pin pair 3-6
    // Correspond to register 74
    eth.send(74, 0x107); // 200 MHz clock
 
    // Trigger distribution to ARICH
    // Using RJ45 output ports 48-51 & 72-75, pin pair 3-6 
    // Triggers to front of RTF
    // Corresponds to register 72 for pin pair 3-6
    eth.send(72, 0x106);
    // Corresponds to register 84 for pin pair 4-5
    eth.send(84, 0x101);



    //print clock frequencies
    std::cout << " Setup clock frequencies \n";
    std::cout << std::hex << (eth.recieve(1)>>3) << std::endl;
    printf("USER 1: %0.3f\n", (float)eth.recieve(100)/1000000.0);
    printf("USER 2: %0.3f\n", (float)eth.recieve(101)/1000000.0);
    printf("USER 3: %0.3f\n", (float)eth.recieve(102)/1000000.0);
    printf("USER 4: %0.3f\n", (float)eth.recieve(103)/1000000.0);
    printf("EXTERN: %0.3f\n", (float)eth.recieve(104)/1000000.0);
    printf("Sys160: %0.3f\n", (float)eth.recieve(105)/1000000.0);

    //prepare burst mode
    eth.setBurstMode(true);

    //start DAQ thread
    DataThread dt;
    dt.start();

    //reset counters and print
    eth.send(0, 0x8);
    usleep(10000);
    
    //configure TAC
    eth.send(106, 1 | (3 << 4) | (uint64_t(3000) << 32));
    while(!stopRunning) usleep(10000000);
    eth.send(106, 0 | (3 << 4) | (uint64_t(3000) << 32));
    eth.setBurstMode(false);

    dt.stop();
    dt.join();

}
