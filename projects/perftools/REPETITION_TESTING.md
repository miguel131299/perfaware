# Repetition Testing - Performance Measurement Guide

## Overview

The repetition tester runs file I/O operations repeatedly until the system stabilizes, finding the minimum (best-case) execution time. This document explains how to measure system-level performance metrics during these tests.

## Running the Repetition Tester

### Basic Usage

```bash
cd build/projects/haversine

# Run with default 10 seconds per test
./repetition_tester ./data_json_1000000.json

# Run with custom test duration (5 seconds per test)
./repetition_tester ./data_json_1000000.json 5000

# Run with longer test duration (30 seconds per test)
./repetition_tester ./data_json_1000000.json 30000
```

### Test Variants

The tester runs 6 test combinations:

1. **fread** without malloc
2. **fread** with malloc
3. **read** (POSIX syscall) without malloc
4. **read** with malloc
5. **fread chunked** (64KB chunks) without malloc
6. **fread chunked** with malloc

Each variant tests the malloc overhead impact on different I/O methods.

## Measuring Page Faults with perf

Page faults indicate memory access patterns and can reveal cache behavior or virtual memory pressure.

### Step 1: Get the Process ID

Open `htop` to monitor the process:

```bash
htop
```

When the repetition tester starts, look for the `repetition_tester` process and note its **PID** (Process ID).

### Step 2: Monitor Page Faults

In another terminal, run `perf stat` with the PID:

```bash
# Monitor page faults with 500ms interval updates
sudo perf stat -e page-faults -p <PID> --interval-print 500 sleep 100
```

Replace `<PID>` with the actual process ID.

#### Example

If the process ID is 176388:

```bash
sudo perf stat -e page-faults -p 176388 --interval-print 500 sleep 100
```

This will:

- Monitor the process for 100 seconds
- Report page faults every 500ms
- Show total page faults at the end

### Step 3: Interpret Results

Lower page fault counts indicate better memory behavior:

- **Fewer major page faults**: Better cache utilization
- **Fewer minor page faults**: Less OS paging overhead

### Advanced: Multiple Metrics

To monitor multiple performance metrics:

```bash
sudo perf stat -e page-faults,cache-misses,cycles -p <PID> --interval-print 500 sleep 100
```

## Example Workflow

### Terminal 1: Run the Repetition Tester

```bash
cd /home/miguel/Repos/perfaware/build/projects/haversine
./repetition_tester ./data_json_1000000.json 5000
```

### Terminal 2: Get Process ID and Monitor

```bash
# Open htop to see the process
htop

# Once you see the repetition_tester running, note the PID
# Then in another terminal, run perf stat (example with PID 12345):
sudo perf stat -e page-faults -p 12345 --interval-print 500 sleep 100
```

## What to Look For

1. **Malloc vs No-Malloc Difference**: Compare page fault counts between malloc and no-malloc variants
2. **I/O Method Performance**: Compare fread vs read syscall
3. **Throughput Correlation**: Check if lower page faults correlate with higher throughput

## Alternative: System-wide Measurement

For a full system view without needing to track PID:

```bash
# Measure during the entire test
time ./repetition_tester ./data_json_1000000.json 5000
```

Or use `strace` to see syscalls:

```bash
strace -c ./repetition_tester ./data_json_1000000.json 5000
```

## Notes

- Requires `linux-tools` package for `perf`: `sudo apt install linux-tools-generic`
- Requires `htop` for process monitoring: `sudo apt install htop`
- Some perf features require elevated privileges (hence `sudo`)
- Page fault counts depend on system load and available memory
