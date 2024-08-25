#pragma once
#include "PackedPair.h"
#include "macros.h"
#include <atomic>

/*

   Below is the state diagram for a generational node, where (STATE, x)
  indicates that the node is in STATE and has x owners

  (Empty, 0)    <-------------------------------
     |                                          |
     | tryStore                                 |
     |                                          |
  (Filling, 0)                                  |
     |                                          |
     | tryStore                                 |
     |                                          |
  (Full, N) -----------------------------> (Freeing, 0)
  |      ^      tryRemoveOwner && N == 1
  |      |
  |------|
  tryAddOwner

*/

template <typename T> class GenerationNode {
  enum class NodeState { Empty, Filling, Full, Freeing };
  using state_t = PackedPair<NodeState, uint32_t>;
  std::atomic<state_t> currState = state_t{NodeState::Empty, 0};
  T *currT = nullptr;

private:
  static constexpr NodeState getStateEnum(state_t s) { return s.first(); }
  static constexpr uint32_t getOwnersCount(state_t s) { return s.second(); }

private:
  bool tryFree() {
    state_t toFreeState{NodeState::Full, 1};
    static constexpr state_t freedState{NodeState::Empty, 0};
    return currState.compare_exchange_strong(toFreeState, freedState);
  }

public:
  // only call on construction
  constexpr void addInitPointer(T *ptr)
  {
    currT = ptr;
  }
  ~GenerationNode()
  {
    delete currT;
  }
  template<typename ... Args>
  bool tryStore(Args&&... args) {
    state_t emptyState{NodeState::Empty, 0};
    static constexpr state_t constructingState{NodeState::Filling, 0};
    static constexpr state_t constructedState{NodeState::Full, 1};

    if (!currState.compare_exchange_strong(emptyState, constructingState)) {
      return false;
    }

    new (currT) T{std::forward<Args>(args)...};

    currState.store(constructedState);
    return true;
  }
  bool tryAddOwner() {
    auto localState = currState.load();

    if (getStateEnum(localState) != NodeState::Full) {
      return false;
    }
    DEBUG_ASSERT(getOwnersCount(localState) > 0 && "empty full");
    return currState.compare_exchange_strong(
        localState, state_t{NodeState::Full, getOwnersCount(localState) + 1});
  }
  bool tryRemoveOwner() {
    auto localState = currState.load();

    if (getStateEnum(localState) != NodeState::Full) {
      return false;
    }

    DEBUG_ASSERT(getOwnersCount(localState) > 0);

    if (getOwnersCount(localState) == 1) {
      return tryFree();
    } else {
      return currState.compare_exchange_strong(
          localState, state_t{NodeState::Full, getOwnersCount(localState) - 1});
    }
  }

public:
  const T &load() const {
    auto localState = currState.load();
    DEBUG_ASSERT(getStateEnum(localState) == NodeState::Full);

    return *currT;
  }
};
