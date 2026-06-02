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
