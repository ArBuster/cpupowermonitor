#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <chrono>
#include <termios.h>
#include <sys/ioctl.h>
#include <string>
#include "AmdPowerMonitor.h"

#define _UJ_LEN_ "11"
#define _FSCANF_LEN_ (std::string("%") + _UJ_LEN_ + "lld").data()

class AmdPowerMonitor
{
private:
    FILE *fp_package = nullptr;

    // 封装功耗，最大值65532610987
    long long package_power[2] = {0};
    // 两次读取功耗的时间间隔
    std::chrono::steady_clock::time_point time[2] = {};
    // linux termios终端的设置，ter_setup[0]保存原始设置，ter_setup[1]保存修改之后的设置
    termios ter_setup[2] = {};

    bool open_rapl();

public:
    AmdPowerMonitor();
    ~AmdPowerMonitor();
    void monitor();
};

// 构造函数
AmdPowerMonitor::AmdPowerMonitor()
{
    tcgetattr(STDIN_FILENO, &ter_setup[0]);
    ter_setup[1] = ter_setup[0];
    ter_setup[1].c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSADRAIN, &ter_setup[1]);
}

// 析构函数
AmdPowerMonitor::~AmdPowerMonitor()
{
    tcsetattr(STDIN_FILENO, TCSADRAIN, &ter_setup[0]);
    if (fp_package != nullptr)
    {
        fclose(fp_package);
    }
}

// 打开intel-rapl文件
// 成功返回true, 失败返回false
bool AmdPowerMonitor::open_rapl()
{
    bool ret_value = true;

    fp_package = fopen("/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj", "r");
    time[0] = std::chrono::steady_clock::now();

    if(fp_package == nullptr)
    {
        ret_value = false;
        printf("打开文件: /sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj 失败\n");
    }

    return ret_value;
}

// 监控cpu package功耗。
// 单位微焦耳，计数器最大值65532610987
// 循环读取energy_uj计数器，用最近一次读取的值减去上一次读取的值再除以时间就是功耗
void AmdPowerMonitor::monitor()
{
    if(open_rapl() == false)
    {
        return;
    }

    fscanf(fp_package, _FSCANF_LEN_, &package_power[0]);

    double time_elapse = 0;
    char ch = 0;
    int bytes = 0;
    for (u_int8_t i = 1, k = 0; true; i ^= 1, k ^= 1)
    {
        sleep(1);

        fflush(fp_package);
        time[i] = std::chrono::steady_clock::now();
        rewind(fp_package);

        fscanf(fp_package, _FSCANF_LEN_, &package_power[i]);

        time_elapse = (double)std::chrono::duration_cast<std::chrono::microseconds>(time[i] - time[k]).count();
        printf("\r\033[Kcpu package: %3.2fw", (package_power[i] - package_power[k]) / time_elapse);
        fflush(stdout);

        ioctl(STDIN_FILENO, FIONREAD, &bytes);
        if (bytes > 0)
        {
            ch = getchar();
            if (ch == 'q' || ch == 'Q')
            {
                putchar('\n');
                break;
            }
        }
    }
}

void amd_power_monitor()
{
    AmdPowerMonitor apm;
    apm.monitor();
}
