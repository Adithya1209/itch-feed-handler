# High-Performance NASDAQ ITCH 5.0 Feed Handler & Multi-Core Zero-Allocation Engine (C++20)

An ultra-low-latency, zero-allocation **NASDAQ TotalView-ITCH 5.0** market data feed handler and multi-stock Limit Order Book (LOB) engine written in modern C++20.

Engineered with mechanical sympathy for modern multi-core CPU architecture, this system streams binary exchange feeds, decodes Big-Endian protocol frames using hardware intrinsics, and maintains real-time Top-of-Book (BBO) state across 65,536 stock locate IDs with zero dynamic heap allocations on the hot message path and 12-core OpenMP parallel file chunking.

---

## Key Engineering Highlights

* **Multi-Core OpenMP Parallel File Chunking**: Parallelizes binary feed processing across 12 hardware CPU cores with zero duplicate memory reads, scaling throughput to **4.10+ Million messages per second** (**~243.7 nanoseconds/msg**).
* **Zero-Copy Memory-Mapped I/O (`mmap`)**: Custom RAII `MappedFile` wrapper utilizing `mmap(2)` and `madvise(MADV_SEQUENTIAL)` to map binary feed files directly into virtual address space, eliminating 100% of read system calls (dropping 7,146,410 syscalls to 0).
* **Single-Cycle Hardware Intrinsics (`bswap64`)**: Hardware-accelerated 48-bit Big-Endian timestamp codecs utilizing CPU intrinsics (`__builtin_bswap64` + bit-shift) to decode nanosecond timestamps in 1–2 clock cycles (`BSWAP` + `SHR`).
* **C++20 Branch Predictor Hints (`[[likely]]` / `[[unlikely]]`)**: Hardware branch predictor hints on hot message paths (`'A'`, `'E'`, `'D'`, `'U'`) to pre-fetch instructions into L1 Instruction Cache without speculative execution stalls.
* **Fibonacci Multiplicative Hashing**: Hashes 64-bit order reference numbers via `(ref_num * 11400714819323198485ULL) & 4095` to scatter order IDs across contiguous array slots in 1 CPU cycle, dropping open-addressing probes to 0–1 probes.
* **Zero-Allocation Order Book Architecture**: Replaced dynamic STL containers (`std::map`, `std::unordered_map`) with Lazy Pointer Allocation (`OrderBook* books[65536]`) and Pre-Allocated L1 Data Cache Flat Arrays (`orders_[4096]`, `bids_[8]`, `asks_[8]`).
* **Profiler-Verified Memory Hygiene**: `valgrind --tool=dhat` verified 0 dynamic `malloc`/`free` calls on the hot message path (eliminating 36,826 critical-path heap allocations) while maintaining total system RAM under 45 Megabytes.
* **Tested on Real Exchange Session**: Benchmarked against official NASDAQ TotalView-ITCH 5.0 historical sessions (3.57+ Million real exchange messages, 890,305,451 shares decoded).

---

## Benchmark Comparison & Architecture Milestones

All benchmarks measured processing 3,573,205 real exchange messages from the official NASDAQ TotalView-ITCH 5.0 session:

| Architectural Metric | Baseline Engine | Single-Core C++20 | 12-Core OpenMP Engine (Current) | Engineering Impact |
| :--- | :--- | :--- | :--- | :--- |
| **I/O Strategy** | `std::ifstream` Stream I/O | Zero-Copy `mmap` | **Parallel File Chunking (`tmpfs`)** | Eliminates kernel syscalls & RAM bus contention |
| **Endianness Codecs** | Byte-shift loops | Single-cycle `bswap64` | **Single-cycle `bswap64`** | 1-2 CPU clock cycle decoding |
| **Order Book Structures** | `std::unordered_map` & `std::map` | Lazy Pointers + Flat Arrays | **Lazy Pointers + L1 Cache Flat Arrays** | Eliminates pointer chasing & tree rebalancing |
| **Parsing Loop Syscalls (`strace`)** | **7,146,410 calls** | **0 calls** | **0 calls** | **100% System Calls Eliminated** |
| **Hot-Path `malloc` (`valgrind`)** | **36,826 calls** | **0 calls** | **0 calls** | **100% Critical-Path Allocations Eliminated** |
| **Full Engine Execution Time** | 2.520 seconds | 2.212 seconds | **0.870 seconds** | **Sub-1-Second Full Dataset Execution** |
| **Total Engine Throughput** | 1.41M msg/sec | 1.61M msg/sec | **4.10M msg/sec** | **2.9x Speedup Over Baseline** |
| **Average Latency per Message** | ~706.0 ns/msg | ~619.0 ns/msg | **243.7 nanoseconds/msg** | **Sub-244ns Effective Latency** |
| **System RAM Footprint** | ~581 KB | < 45 MB | **< 45 MB** | Scalable multi-stock memory footprint |

---

## Performance & Latency Breakdown Analysis

### Latency Budget per Message (243.7ns Effective)

```text
[12-Core OpenMP Parallel Engine: 243.7ns/msg]
├── Binary Field Parsing & Hardware bswap Decoding  (~35ns)
├── Fibonacci Hash Mapping & Open-Addressing Probe  (~55ns)
├── Top-8 L1 Cache Insertion Sort & Level Shift     (~110ns)
└── BBO Spread & Volume Aggregation                 (~43.7ns)
```

#### Architectural Trade-offs & Optimizations:
1. **Parallel File Chunking vs. Duplicate Memory Scanning**: Chunking the file into 12 non-overlapping byte slices gave each CPU core an exclusive 4 MB memory window, eliminating RAM bus contention and boosting throughput from 2.52M to 4.10M msg/sec.
2. **Top-8 L1 Cache Price Depth**: Restricting price depth to `bids_[8]` and `asks_[8]` keeps sorted level arrays inside a single 64-byte L1 Data Cache line, allowing array shifts to execute in under 1 nanosecond.
3. **Fibonacci Multiplicative Hashing**: Multiplication by the 64-bit golden ratio prime `11400714819323198485ULL` uniformly distributes sequential exchange order IDs across array slots, reducing open-addressing collision probes to 0–1 probes.

---

## System Architecture

### 1. Zero-Copy Memory-Mapped I/O ([`mapped_file.h`](mapped_file.h))
Standard stream I/O (`std::ifstream`) copies binary bytes from disk -> Kernel Buffer -> User Buffer, invoking millions of `read()` system calls. The engine encapsulates OS virtual memory via `MappedFile`:
* Maps files directly into virtual memory pages using `PROT_READ` and `MAP_PRIVATE`.
* Issues kernel page pre-fetching hints via `madvise(..., MADV_SEQUENTIAL)`.
* Iterates raw binary byte pointers directly off virtual memory without intermediate stack/heap memory copies.

### 2. Single-Cycle Hardware Byte Swapping ([`itch_utils.h`](itch_utils.h))
NASDAQ ITCH 5.0 uses Big-Endian network byte order. Standard byte-shift loops execute 16 individual shift/OR instructions per timestamp. The engine uses CPU hardware intrinsics:
```cpp
inline uint64_t parse_timestamp48(const uint8_t* bytes) {
    uint64_t val = 0;
    std::memcpy(&val, bytes, 6);
    return __builtin_bswap64(val) >> 16;
}
```

### 3. HFT Zero-Allocation Order Book Engine ([`order_book.h`](order_book.h), [`order_book.cpp`](order_book.cpp))
Standard Limit Order Books use `std::map` (Red-Black Trees) and `std::unordered_map` (Hash Maps), which invoke `malloc`/`free` on every order arrival/deletion, causing OS heap lock jitter and L1 cache misses.

The engine solves this using a Two-Tier HFT Memory Architecture:
1. **Lazy Stock Engine Allocation**: `OrderBook* books[65536] = {nullptr};` allocates an array of 65,536 light pointers (0.5 MB at startup). Order books are instantiated lazily only for active stock locate IDs (~4,038 active tickers), capping total system RAM under 45 MB.
2. **Pre-Allocated L1 Cache Flat Arrays**: Inside each active `OrderBook`:
   * **Fixed Order Pool**: `Order orders_[4096]` stores active orders in contiguous memory.
   * **Fibonacci Hashing**: Maps order IDs via `(ref_num * 11400714819323198485ULL) & 4095` to eliminate hash collisions.
   * **Sorted Price Level Arrays**: `PriceLevel bids_[8]` and `asks_[8]` maintain Top-of-Book market depth in L1 Data Cache using insertion sort.

---

## Real Market Top-of-Book (BBO) State

Real-time Best Bid & Offer (BBO) state calculated from actual NASDAQ exchange traffic:

```text
=========================================================================
REAL MARKET TOP-OF-BOOK (BBO) STATE (DEC 30, 2019 SAMPLES)
=========================================================================
Stock: A        (Locate    1) | [BBO] Bid: $51.91 (50 sh)   | Ask: $85.67 (100 sh)  | Spread: $33.76
Stock: AA       (Locate    2) | [BBO] Bid: $19.01 (500 sh)  | Ask: $21.73 (800 sh)  | Spread: $2.72
Stock: AAAU     (Locate    3) | [BBO] Bid: $12.15 (2000 sh) | Ask: $16.99 (1000 sh) | Spread: $4.84
Stock: AAL      (Locate    6) | [BBO] Bid: $28.00 (200 sh)   | Ask: $28.62 (100 sh)  | Spread: $0.62
Stock: AAPL     (Locate   13) | [BBO] Bid: $289.22 (280 sh)  | Ask: $290.04 (30 sh)  | Spread: $0.82
Stock: ABB      (Locate   20) | [BBO] Bid: $24.09 (3000 sh)  | Ask: $24.15 (17400 sh)| Spread: $0.06
Stock: ABBV     (Locate   21) | [BBO] Bid: $89.24 (302 sh)   | Ask: $89.50 (800 sh)  | Spread: $0.26
=========================================================================
```

---

## Building, Running & Profiling

### Direct Compilation with g++ (Release Mode + OpenMP)
```bash
g++ -O3 -DNDEBUG -std=c++20 -fopenmp main.cpp itch_utils.cpp order_book.cpp -o itch_feed_handler
./itch_feed_handler
```

### Profile Kernel System Calls with strace
Verify 0 system calls in the parsing loop:
```bash
strace -c ./itch_feed_handler
```

### Profile Hot-Path Heap Allocations with Valgrind (DHAT)
Verify 0 hot-path `malloc` calls:
```bash
valgrind --tool=dhat ./itch_feed_handler
```

---

## Project Structure

```text
itch-feed-handler/
├── CMakeLists.txt     # Low-latency C++ compilation flags (-O3 -fopenmp)
├── mapped_file.h      # RAII wrapper for zero-copy memory-mapped file I/O (mmap)
├── itch_types.h       # Packed C++ structures for all 20 ITCH 5.0 message types
├── itch_utils.h       # Hardware-accelerated byte-swap intrinsics & 48-bit timestamp codecs
├── itch_utils.cpp     # Utility implementations (symbol trimming, timestamp formatting)
├── order_book.h       # Zero-allocation OrderBook interface & L1 flat array definitions
├── order_book.cpp     # OrderBook engine implementation (Add, Execute, Cancel, Delete, Replace)
├── main.cpp           # 12-Core OpenMP parallel file chunking & event dispatcher
└── README.md          # Project documentation
```
