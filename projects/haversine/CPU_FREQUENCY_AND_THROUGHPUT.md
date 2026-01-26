# CPU Frequency and Throughput Relationship

## The Formula

The relationship between CPU frequency, throughput, and performance:

```
Throughput (bytes/sec) = CPU_frequency (cycles/sec) / cycles_per_byte
```

Rearranged to find cycles per byte:

```
cycles_per_byte = CPU_frequency / Throughput
```

## Example Calculation

From our frontend testing results:

- **Throughput**: 4.17 GB/s = 4.17 × 1024³ = 4,476,497,404 bytes/sec
- **CPU Frequency**: 4.7 GHz (turbo boost) = 4,700,000,000 cycles/sec

```
cycles_per_byte = 4,700,000,000 / 4,476,497,404
                ≈ 1.05 cycles/byte
```

## Interpretation

**~1 cycle per byte** for the `MOVAllBytesASM` loop is **optimal performance** for:

- Single-byte memory stores
- Simple loop with minimal overhead (inc, cmp, jb)

The CPU achieves this through:

- **Out-of-order execution**: Loop overhead hidden while store executes
- **Store buffer**: Buffering writes to memory
- **Branch prediction**: Predicted loop branches don't stall
- **Write combining**: Multiple writes batched together

## Why This Matters

This relationship lets you:

1. **Estimate CPU frequency** from throughput measurements
2. **Calculate cycles per operation** to understand bottlenecks
3. **Compare different implementations** by cycles/byte rather than wall time
4. **Understand theoretical limits** of your hardware

## Note on CPU Frequency

Modern CPUs don't run at a fixed frequency:

- Base frequency (e.g., 2.7 GHz)
- Turbo boost frequency (e.g., 4.7 GHz)
- Dynamic frequency scaling based on load and thermals

The RepetitionTester uses `rdtsc` (read timestamp counter) and `gettimeofday` to estimate the actual frequency during testing.
