# Word Frequency — Naive, Multithreading & Multiprocessing (C)

Three C implementations for counting word frequencies in a large text file (`text8.txt`) and printing the top 10 most frequent words: a sequential (naive) version, a multithreaded version using pthreads, and a multiprocessing version using `fork()` and shared memory (`mmap`). Includes performance measurement and discussion (e.g. Amdahl's law) in the report.

## Overview

All three programs read **text8.txt**, count how often each word appears, and output the **top 10 most frequent words** plus total execution time.

| Program           | Approach        | Concurrency | Notes |
|-------------------|-----------------|-------------|--------|
| **naive.c**       | Sequential      | Single process, single thread | Read all words, process in one loop, `qsort`, print top 10. Baseline for comparison. |
| **multithreading.c** | Multithreaded | 8 threads (pthreads) | File split into 8 segments; each thread has local word counts, then results merged into a global array protected by a mutex. |
| **multiprocssing.c**  | Multiprocessing | 8 processes (`fork`) | File split into 8 segments; each child writes segment results to `segment_N.bin`; parent merges into shared memory (`mmap`) and prints top 10. Temp files deleted after merge. |

**Input:** `text8.txt` must be in the current working directory when you run the programs (not included in the repo; obtain it separately, e.g. from the project assignment or a public text corpus).

---

## Project structure

```
.
├── README.md
├── project1_os_1220184.pdf   # Report (environment, approaches, performance, Amdahl's law)
├── naive.c                   # Sequential word frequency
├── multithreading.c          # 8-thread pthread version
└── multiprocssing.c          # 8-process fork + mmap version (note: filename has typo "multiprocssing")
```

---

## Requirements

- **C compiler** — GCC (e.g. on Linux/Ubuntu or WSL). The multithreading and multiprocessing code use POSIX APIs (`pthreads`, `fork`, `mmap`, `wait`), so they are intended for a **Linux or compatible environment** (e.g. Ubuntu in a VM, as noted in the report).
- **text8.txt** — Place the input text file in the same directory as the executables (or run from that directory). The report assumes a large text file (e.g. millions of words) for meaningful performance comparison.

---

## Build and run

**Naive (sequential):**

```bash
gcc -o naive naive.c
./naive
```

**Multithreading (pthreads):**

```bash
gcc -o multithreading multithreading.c -lpthread
./multithreading
```

**Multiprocessing (fork + mmap):**

```bash
gcc -o multiprocssing multiprocssing.c
./multiprocssing
```

Run from the directory that contains `text8.txt`. Each program prints total words (or equivalent), the top 10 words with counts, and execution time.

---

## Report

System environment, code description, performance results, and analysis (including Amdahl's law):  
**`project1_os_1220184.pdf`**
