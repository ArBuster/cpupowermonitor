#include <stdio.h>
#include "CpuInfo.hpp"

static inline void cpuid(u_int32_t op,
                         u_int32_t *eax,
                         u_int32_t *ebx,
                         u_int32_t *ecx,
                         u_int32_t *edx)
{
    *eax = op;
    *ecx = 0;
    asm volatile(
        "cpuid"
        : "=a"(*eax),
          "=b"(*ebx),
          "=c"(*ecx),
          "=d"(*edx)
        : "0"(*eax), "2"(*ecx)
        : "memory");
}

// 获取cpu厂商id
// 返回true表成功，false表示出错
bool get_cpu_vendor(std::string &vendor)
{
    bool ret_value = false;

    u_int32_t eax = 0;
    u_int32_t id[3] = {0};

    cpuid(0, &eax, &id[0], &id[2], &id[1]);
    if ((u_int8_t *)&id[0] != 0)
    {
        vendor.assign((char *)&id);
        ret_value = true;
    }
    else
    {
        printf("%s failed\n", __func__);
    }

    return ret_value;
}
