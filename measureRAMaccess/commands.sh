for x in /sys/devices/system/cpu/cpu*/online; do
  echo 0 >"$x"
done

for CPUFREQ in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do [ -f $CPUFREQ ] || continue; echo -n performance > $CPUFREQ; done

cat /proc/cpuinfo
