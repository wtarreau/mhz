# MHz - CPU frequency measurement utility

It's fairly common, especially in embedded systems, to be fooled by a CPU
frequency not matching the believed one. Most of the time the reason stems from
missing frequency operation points, but it can also be caused by bogus clock
drivers, device tree issues, hardware issues, boot loader issues, firmware
issues, different silicon quality, thermal throttling and even (long ago)
vendors cheating by only reporting high values in cpufreq.

This tool runs long loops of inexpensive arithmetic operations (XOR) that are
not optimizable so that we're almost certain that most CPU cores will run them
at a rate of one operation per clock cycle, not less, not more.

This has been tested on various cores including old 180 MHz MIPS and less old
MIPS32/MIPS64, many 32- and 64-bit Arm cores of v5/v7/v8/v9 architectures,
arious i386 and x86_64 up to 6 GHz, RISC-V, PowerPC, Sparc64 and Alpha EV6,
all of them reporting accurate values.

The program first runs a self-calibration so that the short loop takes at least
20 milliseconds. Each measure is run 5 times and the lowest measure is returned
in order to hide as much as possible latency spikes caused by interrupt
processing and context switches. This delay of 20ms is chosen so that it
remains accurate on systems with clocks of 100, 250 and 1000 Hz. The longer
loop is 5 times more instructions, so a full sample cannot take less than 120
ms and since 5 samples are needed, a full test takes at least 600ms.

Various numbers are reported by default, including the timings of the 50- and
250- microsecond loops, and the measured TSC (time stamp counter) frequency,
when available. Most modern processors have their TSC running at a fixed
frequency which is often higher than the base frequency but lower than the
maximum frequency. Knowing the exact frequency is useful when performing clock
measurements on code, as the frequencies of the instructions and the TSC rarely
match nowadays.

## Usage

The usage message can be obtained with `-h` on the command line:
```
$ ./mhz -h
Usage: ./mhz [-h|-c|-t]* [lines [heat [count]]]
  -h      show this help
  -c      show CPU freq only (in MHz)
  -i      report integral frequencies only
  -t      show TSC freq only (in MHz)
  lines   number of measurements (one line per measurement). Def: 1
  heat    pre-heat time in microseconds. Def: 0
  count   calibration value, higher is slower but more accurate. Def: auto
```

The currently supported arguments are:

  - `-c` : will only report the CPU frequency in MHz, not the rest of the
    fields.
  - `-t` : will only report the TSC frequency in MHz, not the rest of the
    fields.
  - `-i` : truncate the frequency to the nearest integer value. This is
    essentially used with `-c` or `-t` for use in scripts.
  - `lines` : this is the number of measurements to perform before quitting.
    By default a single measurement is run. Increasing this value allows to
    collect more values to observe variations, or simply to monitor the CPU
    frequency in real time.
  - `heat` : forces the CPU to run a busy loop of that many microseconds before
    starting measurements. This can be needed on modern CPUs which switch to
    low-power mode after a few milliseconds of idle, so as not to affect the
    first measurement by the time it takes to wake from low-power.
  - `count` : this is the loop count value to be used as a starting point.
    There is little point in setting it, as it will automatically be adjusted
    if not sufficient. However it can sometimes be helpful to permit to run
    multiple tests under the exact same conditions, e.g. by copying the value
    from a previous run, or by forcing a large value.

## Examples

Run 3 measures and exit:
```
$ ./mhz 3
```

Run 3 measures after a pre-heating period of 500ms:
```
$ ./mhz 3 500000
```

Show the integer frequencies of all CPUs on the machine:
```
ncpu=$(nproc)
for ((i=0; i<ncpu;i ++)); do echo -n "$(taskset -c $i ./mhz -c -i) " ;done
echo
```

## Observations and hints

When measuring a CPU's frequency on a multi-core system, please consider always
setting the CPU you want to measure using `taskset -c $CPU`. Indeed, it's very
common to see different CPU frequencies and even different core models. Running
multiple tests without binding to a specific CPU will just report a lot of
variation caused by the random CPU core that runs the program.

In order to check the theoretical frequency ranges supported by the various
CPUs, using `lscpu -e` is very convenient as it will report for each core the
advertised minimum and maximum frequencies. It may happen that a CPU will run
above the advertised maximum, without being a problem.

Experience shows that reported values are often off by ~0.1%, sometimes
slightly more. Using longer loops doesn't get rid of that. It may be caused by
periodic interrupts and other noisy activity. Seeing 1196 MHz for 1200 MHz is
not necessarily an issue and even some vendors will call that "1200".

Most CPUs can only run at integral multiples of a reference clock. Reference
clocks of 24, 25, 33.333 or 50 MHz are commonly seen. This helps doing the
mental round up that may be needed. For example seeing 1293 MHz could indicate
1296 MHz on a system running on multiples of 24 MHz, or 1300 MHz on a system
running on multiples of 25/50 MHz. In the Arm world, multiples of 24 MHz are
quite common, so that advertised frequencies of 1300, 1500, 1600, 1800 and 2000
MHz are in fact more likely 1296, 1512, 1608, 1800 and 2016.

The program is not sensitive to memory bandwidth as is only uses registers.
However it is mandatory to build it with optimizations enable (-O2 or -O3), and
never run it built at -O0 as it will perform unwanted memory accesses.

Finally, the CPU frequency alone isn't an indicator of system performance, it's
only one metric. For a more realistic system benchmark, please consider using
[sbc-bench](https://github.com/ThomasKaiser/sbc-bench) which gathers various
utilities (including mhz) and monitors many metrics all along the tests in
order to detect anomalies.

## Licensing

The license is MIT/X11 meaning that you can basically do what you want with
this code and include it in your own projects. A mention of the download
location is not strictly required but would help your users resync with the
occasional updates that try to improve measurement accuracy. Similarly, fixes
and improvements are welcome.
