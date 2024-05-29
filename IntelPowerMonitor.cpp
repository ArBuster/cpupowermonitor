#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <chrono>
#include <termios.h>
#include <sys/ioctl.h>
#include "IntelPowerMonitor.h"

#define _UJ_LEN_ 13

class IntelPowerMonitor
{
private:
    FILE *fp_package;
    FILE *fp_iacore;

    // 封装功耗，最大值262143328850
    long long package_power[2] = {0};
    // ia core功耗
    long long iacore_power[2] = {0};
    // gt core + uncore功耗, 由封装功耗-ia core功耗得出
    long long gtuncore_power[2] = {0};
    // 两次读取功耗的时间间隔
    std::chrono::steady_clock::time_point time[2] = {};
    // linux termios终端的设置，ter_setup[0]保存原始设置，ter_setup[1]保存修改之后的设置
    termios ter_setup[2] = {};

    bool open_rapl();

public:
    IntelPowerMonitor();
    ~IntelPowerMonitor();
    void monitor();
};

// 构造函数
IntelPowerMonitor::IntelPowerMonitor()
{
    tcgetattr(STDIN_FILENO, &ter_setup[0]);
    ter_setup[1] = ter_setup[0];
    ter_setup[1].c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(STDIN_FILENO, TCSADRAIN, &ter_setup[1]);
}

// 析构函数
IntelPowerMonitor::~IntelPowerMonitor()
{
    tcsetattr(STDIN_FILENO, TCSADRAIN, &ter_setup[0]);
    if (fp_package != nullptr)
    {
        fclose(fp_package);
    }
    if (fp_iacore != nullptr)
    {
        fclose(fp_iacore);
    }
}

// 打开intel-rapl文件
// 成功返回true, 失败返回false
bool IntelPowerMonitor::open_rapl()
{
    bool ret_value = true;

    fp_package = fopen("/sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj", "r");
    fp_iacore = fopen("/sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/energy_uj", "r");
    time[0] = std::chrono::steady_clock::now();

    if(fp_package == nullptr)
    {
        ret_value = false;
        printf("打开文件: /sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj 失败\n");
    }

    if(fp_iacore == nullptr)
    {
        ret_value = false;
        printf("打开文件: /sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/energy_uj 失败\n");
    }

    return ret_value;
}

// 监控cpu package和IA core功耗。
// 单位微焦耳，计数器最大值262143328850
// 循环读取energy_uj计数器，用最近一次读取的值减去上一次读取的值再除以时间就是功耗
void IntelPowerMonitor::monitor()
{
    if(open_rapl() == false)
    {
        return;
    }

    fscanf(fp_package, "%12lld", &package_power[0]);
    fscanf(fp_iacore, "%12lld", &iacore_power[0]);

    gtuncore_power[0] = package_power[0] - iacore_power[0];

    double time_elapse = 0;
    char ch = 0;
    int bytes = 0;
    for (u_int8_t i = 1, k = 0; true; i ^= 1, k ^= 1)
    {
        sleep(1);

        fflush(fp_package);
        fflush(fp_iacore);
        time[i] = std::chrono::steady_clock::now();
        rewind(fp_package);
        rewind(fp_iacore);

        fscanf(fp_package, "%12lld", &package_power[i]);
        fscanf(fp_iacore, "%12lld", &iacore_power[i]);
        gtuncore_power[i] = package_power[i] - iacore_power[i];

        time_elapse = (double)std::chrono::duration_cast<std::chrono::microseconds>(time[i] - time[k]).count();
        printf("\r\033[Kcpu package: %3.2fw%*siacore: %3.2fw%*suncore + gtcore: %3.2fw",
               (package_power[i] - package_power[k]) / time_elapse, 4, "",
               (iacore_power[i] - iacore_power[k]) / time_elapse, 4, "",
               (gtuncore_power[i] - gtuncore_power[k]) / time_elapse);
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

void intel_power_monitor()
{
    IntelPowerMonitor ipm;
    ipm.monitor();
}
