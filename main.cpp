#include <unistd.h>
#include <string>
#include <stdio.h>
#include "CpuInfo.hpp"
#include "IntelPowerMonitor.h"

int main(int argc, char *argv[])
{
    if (getegid() == 0)
    {
        std::string vendor;
        bool ret_value = get_cpu_vendor(vendor);
        if (ret_value == true)
        {
            if (vendor == "GenuineIntel")
            {
                intel_power_monitor();
            }
            else
            {
                printf("vendor: %s is unsupported.\n", vendor.data());
            }
        }
    }
    else
    {
        printf("Require root privilege.\n");
    }

    return 0;
}
