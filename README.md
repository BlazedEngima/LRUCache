# LRUCache

A simple C++ implementation of a **Least Recently Used (LRU) Cache**.

## Features

* O(1) lookup
* O(1) insertion
* O(1) eviction
* Configurable capacity
* STL-style iterators
* Header-only

## Example

```cpp
#include <blazed/lru.hpp>
#include <blazed/lru_format.hpp>
#include <print>

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  blazed::LRUCache<int, int> lru(3);
  for (int i = 0; i < static_cast<int>(lru.capacity()); i++) {
    lru.insert(i, i);
  }

  for (const auto& ele : lru) {
    std::println("Key: {}, Val: {}", ele.first, ele.second);
  }

  // or alternatively, use the provided print formats
  std::println("{}", lru);

  lru.insert(5, 5);
  std::println("{:d}", lru);

  return 0;
}
```

## Implementation

The cache is implemented using:

* `std::unordered_map` for fast key lookup
* `std::list` for tracking usage order

The front of the list contains the most recently used item, while the back contains the least recently used item.

## Complexity

| Operation | Complexity |
| --------- | ---------- |
| Lookup    | O(1)       |
| Insert    | O(1)       |
| Erase     | O(1)       |
| Update    | O(1)       |

## Benchmarks

All results measured using **Google Benchmark (Release mode)**.

### CPU

AMD Ryzen 7 3750H (Running on an old laptop):

* CPU(s) scaling MHz: 125%
* CPU max MHz       : 2300.0000
* CPU min MHz       : 1400.0000
* L1 Data           : 32 KiB per core
* L2 Cache          : 512 KiB per core
* L3 Cache          : 4 MiB shared

---

### Performance (average time per operation)

| Operation       | Time (ns/op)    |
| --------------- | --------------- |
| Insert          | ~3.6 ns         |
| Lookup (L1 hot) | ~10.4 ns        |
| Lookup (L2 hot) | ~17 ns          |
| Lookup (L3 hot) | ~64 ns          |
| Lookup (cold)   | ~212 ns         |
| Miss lookup     | ~13 ns          |
| Insert + evict  | ~37 ns          |
| Update (L1 Hot) | ~13 ns          |
| Update (L2 + L3)| ~25 ns          |
| Update (Cold)   | ~190 ns          |

## License

MIT License.
