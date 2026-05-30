#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <concepts>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace blazed {

template <typename Hash, typename Equal, typename Key, typename Lookup>
concept HeterogeneousLookup = requires(Hash h, Equal e, Lookup lk, Key k) {
  { h(lk) } -> std::convertible_to<size_t>;
  { e(lk, k) } -> std::convertible_to<bool>;
};

/**
 *  @brief A fixed-capacity Least Recently Used (LRU) cache.
 *
 *  This container stores key-value pairs with unique keys and evicts the
 *  least recently used elements when capacity is exceeded.
 *
 *  Lookup, insertion, and update operations are O(1) average time.
 *  Recency is updated on successful lookup or modification.
 *
 *  Internally, the container is implemented using:
 *  - a std::list to maintain usage ordering (most recently used at front)
 *  - a std::unordered_map to provide O(1) key lookup into list nodes
 *
 *  Accessing an existing element through find(), get(), or operator[]
 *  will mark it as most recently used.
 *
 *  @ingroup associative_containers
 *  @headerfile LRUCache
 *  @since C++20 (or your version)
 *
 *  @tparam K
 *    Type of key objects. Must be hashable using Hash and comparable using
 *    KeyEqual.
 *
 *  @tparam V
 *    Type of mapped objects stored in the cache.
 *
 *  @tparam Hash
 *    Hash function object type. Defaults to std::hash<K>.
 *
 *  @tparam KeyEqual
 *    Equality comparison function object type. Defaults to std::equal_to<K>.
 *
 *  @tparam Allocator
 *    Allocator type used for both internal list and hash table storage.
 *    Defaults to std::allocator<std::pair<const K, V>>.
 *
 *  The value type of the container is:
 *    std::pair<const K, V>
 */
template <typename K, typename V, typename Hash = std::hash<K>,
          typename KeyEqual = std::equal_to<K>,
          typename Allocator = std::allocator<std::pair<const K, V>>>
class LRUCache {
  inline static constexpr std::size_t default_capacity = 4096;

  /// Type used for stored key-value nodes.
  using Node = std::pair<const K, V>;
  /// Iterator to internal list nodes.
  using NodeIterator = std::list<Node>::iterator;
  /// Const iterator to internal list nodes.
  using ConstNodeIterator = std::list<Node>::const_iterator;

  using MapValue = std::pair<const K, NodeIterator>;
  using NodeAllocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using MapAllocator = typename std::allocator_traits<
      Allocator>::template rebind_alloc<MapValue>;

 private:
  /// Maximum number of elements the cache can hold.
  std::size_t m_max_capacity;
  /// Hash table mapping keys to list iterators for O(1) lookup.
  std::unordered_map<K, NodeIterator, Hash, KeyEqual, MapAllocator> m_map;
  /// Doubly-linked list storing elements in access order.
  /// Front = most recently used, back = least recently used.
  std::list<Node, NodeAllocator> m_list;

 private:
  /**
   *  @brief Removes least recently used elements until capacity is satisfied.
   */
  void evict() {
    while (m_list.size() > m_max_capacity) {
      auto it = std::prev(m_list.end());
      m_map.erase(it->first);
      m_list.pop_back();
    }
  }

  /**
   *  @brief Moves the given node to the front of the LRU list.
   *
   *  @param it Iterator to the node being promoted.
   */
  void touch(NodeIterator it) { m_list.splice(m_list.begin(), m_list, it); }

  /**
   *  @brief Inserts a new key-value pair into the cache.
   *
   *  If insertion causes capacity overflow, the least recently used element
   *  is evicted.
   *
   *  @param key The key to insert.
   *  @param value The value to associate with the key.
   *
   *  @return Iterator to the newly inserted node.
   */
  template <typename Key, typename Val>
  NodeIterator insert_new(Key&& key, Val&& value) {
    m_list.emplace_front(std::forward<Key>(key), std::forward<Val>(value));

    auto it = m_list.begin();
    m_map.emplace(it->first, it);

    evict();

    return it;
  }

  /**
   *  @brief Inserts a key-value pair.
   *
   *  Helper for insert().
   *  If the key exists, no update will occur and m_list.end() will be returned.
   *  Otherwise, a new element is inserted.
   *
   *  @return std::pair<Iterator, bool> where Iterator points to the affected
   * node and bool indicates whether the update occured.
   */
  template <typename Key, typename Val>
  std::pair<NodeIterator, bool> insert_impl(Key&& key, Val&& value) {
    if (auto it = m_map.find(key); it != m_map.end()) {
      return std::make_pair(m_list.end(), false);
    }

    return std::make_pair(
        insert_new(std::forward<Key>(key), std::forward<Val>(value)), true);
  }

  /**
   *  @brief Inserts or updates a key-value pair.
   *
   *  Helper for emplace_or_update().
   *  If the key exists, the value is updated and the element is marked as
   *  most recently used.
   *  Otherwise, a new element is inserted.
   *
   *  @return Iterator to the affected node.
   */
  template <typename Key, typename Val>
  NodeIterator insert_or_update_impl(Key&& key, Val&& value) {
    if (auto it = m_map.find(key); it != m_map.end()) {
      it->second->second = std::forward<Val>(value);
      touch(it->second);
      return it->second;
    }

    return insert_new(std::forward<Key>(key), std::forward<Val>(value));
  }

  /**
   *  @brief Returns reference to value associated with key.
   *
   *  Helper function of the operator[] overload.
   *  If the key does not exist, a default-constructed value is inserted.
   *
   *  @requires V must be default-constructible.
   */
  template <typename Key>
    requires std::default_initializable<V>
  NodeIterator get_or_insert_default(Key&& key) {
    if (auto it = m_map.find(key); it != m_map.end()) {
      touch(it->second);
      return it->second;
    }

    return insert_new(std::forward<Key>(key), V{});
  }

  /**
   *  @brief Access element by key with bounds checking.
   *
   *  Helper for get().
   *  If the key exists, the element is marked as most recently used.
   *
   *  @throws std::out_of_range if key is not present.
   *
   *  @return Reference to mapped value.
   */
  template <typename Key>
  V& get_impl(const Key& key) {
    auto it = m_map.find(key);
    if (it == m_map.end()) {
      throw std::out_of_range("Key not found");
    }

    touch(it->second);
    return it->second->second;
  }

  /**
   *  @brief Finds an element without throwing on failure.
   *
   *  Helper for find().
   *  This version does not update the recency list.
   *
   *  @return ConstIterator to element if found, otherwise end().
   */
  template <typename Key>
  ConstNodeIterator find_impl(const Key& key) const {
    auto it = m_map.find(key);

    if (it == m_map.end()) {
      return m_list.end();
    }

    return static_cast<ConstNodeIterator>(it->second);
  }

  /**
   *  @brief Finds an element without throwing on failure.
   *
   *  Helper for find_with_recency_update().
   *  If found, the element is marked as most recently used.
   *
   *  @return Iterator to element if found, otherwise end().
   */
  template <typename Key>
  NodeIterator find_with_recency_update_impl(const Key& key) {
    auto it = m_map.find(key);

    if (it == m_map.end()) {
      return m_list.end();
    }

    touch(it->second);
    return it->second;
  }

  /**
   *  @brief Removes an element from the cache.
   *
   *  Helper for erase().
   *
   *  @return true if element was removed, false if key was not found.
   */
  template <typename Key>
  bool erase_impl(const Key& key) {
    auto it = m_map.find(key);
    if (it == m_map.end()) {
      return false;
    }

    m_list.erase(it->second);
    m_map.erase(it);

    return true;
  }

 public:
  explicit LRUCache() : m_max_capacity(default_capacity) {
    m_map.reserve(m_max_capacity * 2);  // Minimizes rehashing
    m_list = std::list<Node>();
  }

  explicit LRUCache(std::size_t capacity) : m_max_capacity(capacity) {
    m_map.reserve(capacity * 2);
    m_list = std::list<Node>();
  }

  LRUCache(const LRUCache&) = default;
  LRUCache(LRUCache&&) = default;

  /**
   *  @brief Returns number of elements currently stored.
   *
   *  @return Number of cached elements.
   */
  [[nodiscard(
      "Ignoring size defeats the purpose of checking the container size")]]
  std::size_t size() const noexcept {
    return m_map.size();
  }

  /**
   *  @brief Returns the capacity of the LRUCache.
   *
   *  @return Max number of elements the LRUCache can store.
   */
  [[nodiscard(
      "Ignoring capacity defeats the purpose of checking the container "
      "capacity")]]
  std::size_t capacity() const noexcept {
    return m_max_capacity;
  }

  /**
   *  @brief Checks whether a key exists in the cache.
   *
   *  Does not modify recency order.
   *
   *  @return true if key exists, false otherwise.
   */
  [[nodiscard(
      "Ignoring contains() defeats the purpose of the existence check")]]
  bool contains(const K& key) const {
    return m_map.contains(key);
  }

  /**
   *  @brief Access element by key with bounds checking.
   *
   *  If the key exists, the element is marked as most recently used.
   *
   *  @throws std::out_of_range if key is not present.
   *
   *  @return Reference to mapped value.
   */
  [[nodiscard("Ignoring the retrieved value")]]
  V& get(const K& key) {
    return get_impl(key);
  }

  template <typename Key>
    requires HeterogeneousLookup<Hash, KeyEqual, K, Key> &&
             (!std::same_as<std::remove_cvref_t<Key>, K>)
  [[nodiscard("Ignoring the retrieved value")]]
  V& get(const Key& key) {
    return get_impl(key);
  }

  /**
   *  @brief Finds an element without throwing on failure.
   *
   *  This overload does not update the recency order.
   *  The returned const iterator prevents modification of the underlying
   *  element to preserve cache consistency. For mutable access with recency
   *  update, use find_with_recency_update().
   *
   *  @return ConstIterator to element if found, otherwise end().
   */
  [[nodiscard("Ignoring the found key, value pair")]]
  ConstNodeIterator find(const K& key) const {
    return find_impl(key);
  }

  template <typename Key>  // Heterogeneous Lookup overload
    requires HeterogeneousLookup<Hash, KeyEqual, K, Key> &&
             (!std::same_as<std::remove_cvref_t<Key>, K>)
  [[nodiscard("Ignoring the found key, value pair")]]
  ConstNodeIterator find(const Key& key) const {
    return find_impl(key);
  }

  /**
   *  @brief Finds an element without throwing on failure.
   *
   *  If found, the element is marked as most recently used.
   *
   *  @return Iterator to element if found, otherwise end().
   */
  [[nodiscard("Ignoring the found key, value pair")]]
  NodeIterator find_with_recency_update(const K& key) {
    return find_with_recency_update_impl(key);
  }

  template <typename Key>  // Heterogeneous Lookup overload
    requires HeterogeneousLookup<Hash, KeyEqual, K, Key> &&
             (!std::same_as<std::remove_cvref_t<Key>, K>)
  [[nodiscard("Ignoring the found key, value pair")]]
  NodeIterator find_with_recency_update(const Key& key) {
    return find_with_recency_update_impl(key);
  }

  /**
   *  @brief Access or insert element.
   *
   *  If the key exists, returns reference to mapped value.
   *  Otherwise inserts a default-constructed value.
   *
   *  @requires V must be default-constructible.
   *
   *  @return Reference to mapped value.
   */
  V& operator[](const K& key) { return get_or_insert_default(key)->second; }

  V& operator[](K&& key) {
    return get_or_insert_default(std::move(key))->second;
  }

  template <typename Key>  // Heterogeneous Lookup overload
    requires HeterogeneousLookup<Hash, KeyEqual, K, Key> &&
             (!std::same_as<std::remove_cvref_t<Key>, K>)
  V& operator[](Key&& key) {
    return get_or_insert_default(std::forward<Key>(key))->second;
  }

  /**
   *  @brief Inserts a key-value pair.
   *
   *  If the key exists, no update will occur and m_list.end() will be returned.
   *  Otherwise, a new element is inserted.
   *
   *  @return std::pair<Iterator, bool> where Iterator points to the affected
   * node and bool indicates whether the update occured.
   */
  template <typename Key, typename... Args>  // Variadic argument overload
    requires std::is_constructible_v<V, Args&&...> &&
             std::is_constructible_v<K, Key>
  std::pair<NodeIterator, bool> insert(Key&& key, Args&&... args) {
    return insert_impl(std::forward<Key>(key), V(std::forward<Args>(args)...));
  }

  template <typename Key>  // Lvalue overload
    requires std::is_constructible_v<K, Key>
  std::pair<NodeIterator, bool> insert(Key&& key, const V& val) {
    return insert_impl(std::forward<Key>(key), val);
  }

  template <typename Key>  // Rvalue overload
    requires std::is_constructible_v<K, Key>
  std::pair<NodeIterator, bool> insert(Key&& key, V&& val) {
    return insert_impl(std::forward<Key>(key), std::move(val));
  }

  /**
   *  @brief Inserts or updates a key-value pair.
   *
   *  If the key exists, the value is updated and the element is marked as
   *  most recently used.
   *  Otherwise, a new element is constructed in place.
   *
   *  @return Iterator to the affected node.
   */
  template <typename Key, typename... Args>  // Variadic argument overload
    requires std::is_constructible_v<V, Args&&...> &&
             std::is_constructible_v<K, Key>
  NodeIterator insert_or_update(Key&& key, Args&&... args) {
    return insert_or_update_impl(std::forward<Key>(key),
                                 V(std::forward<Args>(args)...));
  }

  template <typename Key>  // Lvalue overload
    requires std::is_constructible_v<K, Key>
  NodeIterator insert_or_update(Key&& key, const V& val) {
    return insert_or_update_impl(std::forward<Key>(key), val);
  }

  template <typename Key>  // Rvalue overload
    requires std::is_constructible_v<K, Key>
  NodeIterator insert_or_update(Key&& key, V&& val) {
    return insert_or_update_impl(std::forward<Key>(key), std::move(val));
  }

  /**
   *  @brief Removes an element from the cache.
   *
   *  @return true if element was removed, false if key was not found.
   */
  bool erase(const K& key) { return erase_impl(key); }

  template <typename Key>  // Heterogeneous Lookup overload
    requires HeterogeneousLookup<Hash, KeyEqual, K, Key> &&
             (!std::same_as<std::remove_cvref_t<Key>, K>)
  bool erase(const Key& key) {
    return erase_impl(key);
  }

  /**
   *  @brief Returns iterator to most recently used element.
   */
  NodeIterator begin() noexcept { return m_list.begin(); }
  ConstNodeIterator begin() const noexcept { return m_list.begin(); }
  ConstNodeIterator cbegin() const noexcept { return m_list.cbegin(); }

  /**
   *  @brief Returns iterator past the last element.
   */
  NodeIterator end() noexcept { return m_list.end(); }
  ConstNodeIterator end() const noexcept { return m_list.end(); }
  ConstNodeIterator cend() noexcept { return m_list.cend(); }
};

};  // namespace blazed

#endif  // !LRU_CACHE__
