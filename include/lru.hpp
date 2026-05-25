#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace blazed {

template <typename K, typename V, typename Hash = std::hash<K>,
          typename KeyEqual = std::equal_to<K>,
          typename Allocator = std::allocator<std::pair<K, V>>>
class LRUCache {
  inline static constexpr std::size_t default_capacity = 4096;
  using NodeIterator = std::list<std::pair<K, V>>::iterator;

 private:
  std::size_t m_max_capacity;
  std::unordered_map<K, NodeIterator, Hash, KeyEqual, Allocator> m_map;
  std::list<std::pair<K, V>, Allocator> m_list;

 private:
  void evict() {
    while (m_list.size() > m_max_capacity) {
      auto it = m_list.back();
      m_map.erase(it->first);
      m_list.pop_back();
    }
  }

  template <typename Key, typename Val>
  NodeIterator upsert_impl(Key&& key, Val&& value) {
    if (auto it = m_map.find(key); it != m_map.end()) {
      it->second->second = std::forward<Val>(value);
      m_list.splice(m_list.begin(), m_list, it->second);
      return it->second;
    }

    auto node_it =
        m_list.emplace_front(std::forward<Key>(key), std::forward<Val>(value));

    m_map.emplace(node_it.first, node_it);
    evict();
    return node_it;
  }

 public:
  explicit LRUCache() : m_max_capacity(default_capacity) {
    m_map.reserve(default_capacity);
    m_list = std::list<V>();
  }

  explicit LRUCache(std::size_t capacity) : m_max_capacity(capacity) {
    m_map.reserve(capacity);
    m_list = std::list<V>();
  }

  LRUCache(const LRUCache&) = default;
  LRUCache(LRUCache&&) = default;

  // Throws out of range if key does not exist
  [[nodiscard("Ignoring the retrieved value")]]
  V& get(const K& key) {
    auto it = m_map.at(key);
    m_list.splice(m_list.begin(), m_list, it->second);
    return it->second->second;
  }

  template <typename Key, typename... Args>
    requires std::is_constructible_v<V, Args&&...> &&
             std::is_constructible_v<K, Key>
  NodeIterator emplace_or_update(Key&& key, Args&&... args) {
    return upsert_impl(std::forward<Key>(key), V(std::forward<Args>(args)...));
  }

  template <typename Key>
    requires std::is_constructible_v<K, Key>
  NodeIterator emplace_or_update(Key&& key, const V& val) {
    return upsert_impl(std::forward<Key>(key), val);
  }

  template <typename Key>
    requires std::is_constructible_v<K, Key>
  NodeIterator emplace_or_update(Key&& key, V&& val) {
    return upsert_impl(std::forward<Key>(key), std::move(val));
  }
};

};  // namespace blazed

#endif  // !LRU_CACHE__
