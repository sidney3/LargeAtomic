#pragma once
#include <atomic>
#include <memory>
#include <cassert>
#include <string>
#include <sstream>
#include <iostream>
#include "PackedPair.h"

/*

   Below is the state diagram for a generational node, where (STATE, x) indicates
   that the node is in STATE and has x owners

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

template<typename T>
class GenerationNode
{
    enum class NodeState
    {
        Empty, Filling, Full, Freeing
    };
    using state_t = PackedPair<NodeState, uint32_t>;
    std::atomic<state_t> currState = state_t{NodeState::Empty, 0};
    std::shared_ptr<T> currT = nullptr;
private:
    static NodeState getCurrState(state_t s)
    {
        return s.first();
    }
    static uint32_t getOwnersCount(state_t s)
    {
        return s.second();
    }
    auto currStatePrintable()
    {
        auto localState = currState.load();
        std::string nodeState = [&]()
        {
            switch(getCurrState(localState))
            {
            case NodeState::Empty: return "Empty";
            case NodeState::Filling: return "Filling";
            case NodeState::Full: return "Full";
            case NodeState::Freeing: return "Freeing";
            }
            throw std::runtime_error{"unrecognized state"};
        }();
        std::stringstream out;
        out << nodeState << " " << getOwnersCount(localState);
        return out.str();
    }
private:
    bool tryFree()
    {
        state_t toFreeState{NodeState::Full, 1};
        state_t freeingState{NodeState::Freeing, 0};
        state_t freedState{NodeState::Empty, 0};

        if(!currState.compare_exchange_strong(toFreeState, freeingState))
        {
            return false;
        }

        assert(currT != nullptr && "double free");
        currT = nullptr;

        bool emptyingSucceeds = currState.compare_exchange_strong(freeingState, freedState);
        assert(emptyingSucceeds && "Once freeing begins must not be interrupted");

        return true;
    }
public:
    bool tryStore(std::shared_ptr<T> toStore)
    {
        state_t emptyState{NodeState::Empty, 0};
        state_t constructingState{NodeState::Filling, 0};
        state_t constructedState{NodeState::Full, 1};

        if(!currState.compare_exchange_strong(emptyState, constructingState))
        {
            return false;
        }
        currT = toStore;

        bool constructionSucceeds = currState.compare_exchange_strong(constructingState, constructedState);

        assert(constructionSucceeds && "Once construction begins it must not be interrupted");


        return true;
    }
public:
    bool tryAddOwner()
    {
        auto localState = currState.load();

        if(getCurrState(localState) != NodeState::Full)
        {
            return false;
        }
        assert(getOwnersCount(localState) > 0 && "empty full");
        return currState.compare_exchange_strong(localState, state_t{NodeState::Full, getOwnersCount(localState) + 1});

    }
    bool tryRemoveOwner()
    {
        auto localState = currState.load();

        if(getCurrState(localState) != NodeState::Full)
        {
            return false;
        }

        assert(getOwnersCount(localState) > 0);

        if(getOwnersCount(localState) == 1)
        {
            return tryFree();
        }
        else
        {
            return currState.compare_exchange_strong(localState, state_t{NodeState::Full, getOwnersCount(localState) - 1});
        }
    }
    bool empty() const
    {
        auto localState = currState.load();
        return getCurrState(localState) == NodeState::Empty;
    }
public:
    const T &load() const
    {
        auto localState = currState.load();
        assert(getCurrState(localState) == NodeState::Full);

        return *currT;
    }
};
