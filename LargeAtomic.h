#pragma once
#include <array>
#include <atomic>
#include <functional>
#include <utility>

#include "GenerationNode.h"
#include "macros.h"

template <typename T, long MaxGenerations = 100> class LargeAtomic;

template <typename T> class ConcurrentView {
  friend LargeAtomic<T>;

private:
  std::reference_wrapper<GenerationNode<T>> node;
  ConcurrentView(GenerationNode<T> &node) : node(node) {}

public:
  ~ConcurrentView() {
    while (!node.get().tryRemoveOwner()) {
    }
  }
  const T &operator*() { return node.get().load(); }

  ConcurrentView(const ConcurrentView &) = delete;
  ConcurrentView(ConcurrentView &&) = delete;
};

template <typename T, long MaxGenerations> class LargeAtomic {
  static_assert(MaxGenerations >= 2);

private:
  std::atomic<size_t> head = 0;
  std::array<GenerationNode<T>, MaxGenerations> generations = {};

public:
  template <typename... Args> LargeAtomic(Args &&...args) noexcept(noexcept(T{})) {
    bool initGeneration =
        generations[0].tryStore(new T{std::forward<Args>(args)...});
    DEBUG_ASSERT(initGeneration);
  }
  ConcurrentView<T> load() {
    while (true) {
      size_t currHead = head.load(std::memory_order_relaxed);

      if (likely(generations[currHead].tryAddOwner())) {
        return ConcurrentView<T>{generations[currHead]};
      }
    }
  }

  template <typename... Args>
    requires std::is_constructible_v<T, Args...>
  void store(Args &&...args) {
    T *newT = new T{std::forward<Args>(args)...};

    while (true) {
      size_t localHead = head.load(std::memory_order_relaxed);
      size_t nextHead = localHead + 1;
      if(nextHead == MaxGenerations)
      {
        nextHead = 0;
      }

      if (unlikely(!generations[nextHead].tryStore(newT))) {
        continue;
      }
      while (!generations[localHead].tryRemoveOwner()) {
      }

      head.store(nextHead, std::memory_order_relaxed);
      return;
    }
  }
};
