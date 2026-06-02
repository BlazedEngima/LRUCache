#include <benchmark/benchmark.h>

#include <blazed/lru.hpp>
#include <random>

namespace {

using blazed::LRUCache;

constexpr int kValue = 42;

static void BM_Insert(benchmark::State& state) {
  const auto capacity = static_cast<std::size_t>(state.range(0));

  for (auto _ : state) {
    LRUCache<int, int> cache(capacity);

    for (std::size_t i = 0; i < capacity; ++i) {
      cache.insert(static_cast<int>(i), kValue);
    }

    benchmark::ClobberMemory();
  }
}

static void BM_HitLookupHot(benchmark::State& state) {
  const auto capacity = static_cast<std::size_t>(state.range(0));

  LRUCache<int, int> cache(capacity);

  for (std::size_t i = 0; i < capacity; ++i) {
    cache.insert(static_cast<int>(i), kValue);
  }

  constexpr int key = 0;

  for (auto _ : state) {
    auto& value = cache.get(key);
    benchmark::DoNotOptimize(value);
    benchmark::ClobberMemory();
  }
}

static void BM_HitLookupRandom(benchmark::State& state) {
  const auto capacity = static_cast<std::size_t>(state.range(0));

  LRUCache<int, int> cache(capacity);

  for (std::size_t i = 0; i < capacity; ++i) {
    cache.insert(static_cast<int>(i), kValue);
  }

  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, static_cast<int>(capacity - 1));

  for (auto _ : state) {
    auto& value = cache.get(dist(rng));
    benchmark::DoNotOptimize(value);
    benchmark::ClobberMemory();
  }
}

static void BM_MissLookup(benchmark::State& state) {
  const auto capacity = static_cast<std::size_t>(state.range(0));

  LRUCache<int, int> cache(capacity);

  for (std::size_t i = 0; i < capacity; ++i) {
    cache.insert(static_cast<int>(i), kValue);
  }

  constexpr int missing_key = -1;

  for (auto _ : state) {
    auto value = cache.find(missing_key);
    benchmark::DoNotOptimize(value);
    benchmark::ClobberMemory();
  }
}

static void BM_InsertEvict(benchmark::State& state) {
  const auto capacity = static_cast<std::size_t>(state.range(0));

  LRUCache<int, int> cache(capacity);

  for (std::size_t i = 0; i < capacity; ++i) {
    cache.insert(static_cast<int>(i), kValue);
  }

  int next_key = static_cast<int>(capacity);

  for (auto _ : state) {
    cache.insert(next_key++, kValue);
    benchmark::ClobberMemory();
  }
}

static void BM_UpdateExisting(benchmark::State& state) {
  const auto capacity = static_cast<std::size_t>(state.range(0));

  LRUCache<int, int> cache(capacity);

  for (std::size_t i = 0; i < capacity; ++i) {
    cache.insert(static_cast<int>(i), kValue);
  }

  std::mt19937 rng(12345);
  std::uniform_int_distribution<int> dist(0, static_cast<int>(capacity - 1));

  for (auto _ : state) {
    auto existing_key = dist(rng);
    cache[static_cast<size_t>(existing_key)] = kValue + 1;
    benchmark::ClobberMemory();
  }
}

}  // namespace

BENCHMARK(BM_Insert)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_HitLookupHot)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_HitLookupRandom)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_MissLookup)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_InsertEvict)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK(BM_UpdateExisting)->Arg(1'000)->Arg(10'000)->Arg(100'000);

BENCHMARK_MAIN();
