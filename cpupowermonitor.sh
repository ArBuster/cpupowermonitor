#!/bin/sh

if [ `whoami` == "root" ]; then
    package0=$(cat /sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj)
    core0=$(cat /sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/energy_uj)
    uncore0=$[$package0-$core0]
    while true;do
        sleep 1.5
        package1=$(cat /sys/class/powercap/intel-rapl/intel-rapl:0/energy_uj)
        core1=$(cat /sys/class/powercap/intel-rapl/intel-rapl:0/intel-rapl:0:0/energy_uj)
        uncore1=$[$package1-$core1]
        clear
        echo $package1 $package0 | awk '{ printf("%s%.2f%s\n", "cpu package: ", ($1-$2)/1500000, "w") }'
        echo $core1 $core0 | awk '{ printf("%s%.2f%s\n", "core: ", ($1-$2)/1500000, "w") }'
        echo $uncore1 $uncore0 | awk '{ printf("%s%.2f%s\n", "uncore: ", ($1-$2)/1500000, "w") }'
        package0=$package1
        core0=$core1
        uncore0=$uncore1
    done
else
    echo "Require root privilege."
fi
