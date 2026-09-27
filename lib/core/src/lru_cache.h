#pragma once

#include <cstddef>
#include <list>
#include <optional>
#include <utility>

// Keeps the most recently used entries up to a fixed capacity.
// Not thread-safe; the owning task must be the only user.
template <typename Key, typename Value>
class LruCache {
 public:
  explicit LruCache(size_t capacity) : capacity_(capacity) {}

  // Returns the cached value and marks it as the most recently used.
  std::optional<Value> get(const Key& key) {
    const auto entry = findEntry(key);
    if (entry == entries_.end()) {
      return std::nullopt;
    }
    entries_.splice(entries_.begin(), entries_, entry);
    return entries_.front().second;
  }

  // Evicts the least recently used entry when the cache is full.
  void put(const Key& key, Value value) {
    const auto entry = findEntry(key);
    if (entry != entries_.end()) {
      entries_.erase(entry);
    }
    entries_.emplace_front(key, std::move(value));
    if (entries_.size() > capacity_) {
      entries_.pop_back();
    }
  }

  bool contains(const Key& key) const {
    for (const auto& entry : entries_) {
      if (entry.first == key) {
        return true;
      }
    }
    return false;
  }

 private:
  // Linear search is fine: the cache holds only a handful of artworks.
  typename std::list<std::pair<Key, Value>>::iterator findEntry(const Key& key) {
    for (auto entry = entries_.begin(); entry != entries_.end(); ++entry) {
      if (entry->first == key) {
        return entry;
      }
    }
    return entries_.end();
  }

  size_t capacity_;
  std::list<std::pair<Key, Value>> entries_;  // Front is the most recent.
};
