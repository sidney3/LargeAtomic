#pragma once
#include <array>
#include <atomic>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

template <typename T, long MaxGenerations = 10> class LargeAtomic {
  static_assert(MaxGenerations >= 2);

private:
  std::atomic<size_t> head = 0;
  std::atomic<size_t> tail = 0;
  std::array<std::aligned_storage_t<sizeof(T), alignof(T)>, MaxGenerations>
      history{};

public:
  template <typename... Args>
  constexpr LargeAtomic(Args &&...args) noexcept(noexcept(T{})) {
    new (&history[0]) T{std::forward<Args>(args)...};
  }

  ~LargeAtomic() {
    for (auto &storage : history) {
      std::destroy_at(&storage);
    }
  }

private:
  void release(size_t index, std::memory_order order) {
    tail.store(head, order);
  }
  const T &loadImpl(size_t index) {
    return *std::launder(reinterpret_cast<const T *>(&history[index]));
  }
  class LargeAtomicReturn {
  private:
    std::reference_wrapper<LargeAtomic<T, MaxGenerations>> container;
    size_t generation;
    std::memory_order order;

  public:
    LargeAtomicReturn(LargeAtomic<T, MaxGenerations> &container,
                      size_t generation, std::memory_order order)
        : container(container), generation(generation), order(order) {}
    ~LargeAtomicReturn() {}
    const T &operator*() { return container.get().loadImpl(generation); }
  };

public:
  /*
    Critically, return a copy as otherwise this value might get overwritten
    by the writing thread.
  */
  LargeAtomicReturn load(std::memory_order order = std::memory_order_acquire) {
    size_t currHead = head.load(order);
    return LargeAtomicReturn{*this, currHead, order};
  }

  template <typename U>
    requires std::is_constructible_v<T, U>
  void store(U &&obj, std::memory_order order = std::memory_order_release) {
    size_t currTail = tail.load(order);
    size_t currHead = head.load(order);
    size_t nextHead = (currHead + 1) % MaxGenerations;

    // child has lagged behind: need to jump past it
    if (nextHead == currTail) {
      nextHead = (nextHead + 1) % MaxGenerations;
    }
    std::destroy_at(&history[nextHead]);
    new (&history[nextHead]) T{std::forward<U>(obj)};

    head.store(nextHead, order);
  }
};
