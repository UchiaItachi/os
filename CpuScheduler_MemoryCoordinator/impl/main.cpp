
#include "CpuScheduler.h"
#include "MemoryCoordinator.h"

#include <iostream>
#include <sstream>

int main(int argc, char* argv[])
{
    uint32_t wakeUpInterval = 0;

    if (argc <= 1)
    {
      std::cout << std::endl << "Schedule interval not passed as an argument " << std::endl;
#ifdef ENABLE_CPU
        wakeUpInterval = 10;
#elif ENABLE_MEM
        wakeUpInterval = 3;
#endif
     std::cout << "Default the schedule interval to : " << wakeUpInterval << " seconds " << std::endl;
    }
    else
    {
      int result;
      std::stringstream(argv[1]) >> result;
      wakeUpInterval = result;
      std::cout << std::endl << "Schedule interval in seconds = " << wakeUpInterval << std::endl;
    }

#ifdef ENABLE_CPU
    CpuScheduler sched;
    sched.initialize();
    sched.schedule(wakeUpInterval);
    sched.close();
#elif ENABLE_MEM
    MemoryCoordinator memCord;
    memCord.initialize();
    memCord.analyse(wakeUpInterval);
    memCord.close();
#endif
    return 0;
}