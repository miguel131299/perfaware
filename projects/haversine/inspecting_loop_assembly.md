# Inspecting Loop Assembly

## Test Configuration

- **Compiler**: clang-18
- **Architecture**: x86-64
- **Optimization Level**: -O1
- **CPU Frequency**: 2.7 GHz (2.7 × 10⁹ cycles/second)

## Loop Under Test

```cpp
for (u64 index = 0; index < params->fileSize; ++index) {
    buffer[index] = (char)index;
}
```

## Assembly Analysis

### Code Size

The compiled loop body occupies **13 bytes** of machine code in the instruction cache.

### Assembly Disassembly (Intel Syntax)

```asm
; Loop body (13 bytes total)
5555555578A: xor    eax, eax              ; Initialize index = 0
5555555578C: nop    DWORD PTR [rax]       ; 4-byte alignment NOP

; Main loop starts here
555555557890: mov    BYTE PTR [rbp+rax], al   ; buffer[index] = (char)index
555555557894: inc    rax                       ; index++
555555557797: cmp    rax, QWORD PTR [rbx+8]   ; compare index < fileSize
55555555779B: jb     555555557890              ; jump if below (unsigned <)
```

**Instruction Breakdown:**

1. `mov [rbp+rax], al` - Write byte to memory (4 bytes)
2. `inc rax` - Increment loop counter (3 bytes)
3. `cmp rax, [rbx+8]` - Compare with limit (4 bytes)
4. `jb` - Conditional branch (2 bytes)

**Total: 13 bytes**

## Performance Metrics

### Measured Bandwidth

- **Best throughput**: 1.39 GiB/s (binary gigabytes per second)
- **Converted**: 1.39 × 1024³ = 1,492,501,135 bytes/second

### Cycles Per Byte Written

```
CPU_frequency / Bandwidth = Cycles_per_byte

(2.7 × 10⁹ cycles/sec) ÷ (1.39 × 1024³ bytes/sec) = 1.81 cycles/byte
```

**Result: ~1.81 CPU cycles per byte written**

## What This Means

### Memory-Bound Operation

The loop is **memory-bound**, not CPU-bound:

- The CPU can execute instructions faster than memory can accept writes
- Each byte write takes ~1.81 cycles to complete
- Modern CPUs can retire multiple instructions per cycle, but must wait for memory

### Performance Characteristics

1. **Write Latency**: Each memory write requires ~1.81 cycles
   - This includes cache hierarchy latency
   - L1 cache write-through to L2/L3
   - Memory controller overhead

2. **Instruction Efficiency**: The loop is well-optimized
   - Minimal overhead (only 4 instructions)
   - Good register allocation (rbp=buffer, rax=index)
   - Branch prediction friendly (loop pattern)

3. **Cache Behavior**:
   - Sequential writes have good spatial locality
   - Hardware prefetcher can predict access pattern
   - Write-combining buffers batch writes to cache lines

### Comparison to Theoretical Limits

**Typical L1 Cache Latency**: ~4 cycles

- Our measured 1.81 cycles/byte suggests write combining and pipelining
- Multiple writes can be in-flight simultaneously
- Store buffer allows CPU to continue while writes complete

**Memory Hierarchy Impact**:

```
L1 Cache:  ~1-4 cycles (if all writes stay in L1)
L2 Cache:  ~10-20 cycles
L3 Cache:  ~40-75 cycles
DRAM:      ~200-300 cycles
```

The 1.81 cycles/byte indicates excellent cache performance with effective write buffering.

## Optimization Considerations

### Why This Loop is Efficient

1. **Simple Address Calculation**: `[rbp + rax]` uses base+index addressing mode
   - Single instruction with no extra overhead
   - No multiplication or complex arithmetic

2. **Minimal Loop Overhead**: Only 4 instructions
   - No unnecessary moves or stack operations
   - Registers stay hot across iterations

3. **Write Pattern**: Sequential writes are hardware-friendly
   - Prefetcher can predict next cache line
   - Write combining reduces memory bandwidth usage
   - TLB entries stay cached (4KB pages)

### What Could Make It Faster

1. **Vectorization**: Using SIMD instructions (SSE/AVX)

   ```cpp
   // Could write 16-64 bytes per instruction
   // Example: _mm_store_si128, _mm256_store_si256
   ```

2. **Cache Line Awareness**: Write full cache lines (64 bytes)

   ```cpp
   // Non-temporal stores to bypass cache
   // Example: _mm_stream_si128
   ```

3. **Memory Alignment**: Ensure buffer is aligned to cache line boundaries
   - Reduces cache line splits
   - Improves write combining efficiency

4. **Hardware Characteristics**:
   - DDR5 vs DDR4 memory would increase bandwidth
   - More memory channels increase throughput
   - CPU with better cache hierarchy

## Conclusion

This simple loop achieves **1.81 cycles per byte** written, which is excellent for a scalar (non-SIMD) memory write loop. The performance is limited by memory subsystem bandwidth, not CPU execution speed. The compiler generated efficient code with minimal overhead, and the hardware's write buffering and cache system are working effectively.

For comparison:

- **Perfect L1 cache**: ~1 cycle/byte (theoretical limit)
- **Our result**: ~1.81 cycles/byte (very good)
- **DRAM bound**: ~200+ cycles/byte (would be bad)

The 1.81 cycles/byte indicates we're hitting L1/L2 cache efficiently with good write combining behavior.
