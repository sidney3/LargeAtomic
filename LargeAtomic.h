#include <utility>
#include <atomic>
#include <memory>
#include <array>

#include "GenerationNode.h"


template<typename T, long MaxGenerations = 100>
class LargeAtomic;

template<typename T>
class ConcurrentView
{
    friend LargeAtomic<T>;
private:
    std::reference_wrapper<GenerationNode<T>> node;
    ConcurrentView(GenerationNode<T> &node)
        : node(node)
    {}
public:
    ~ConcurrentView()
    {
        while(!node.get().tryRemoveOwner())
        {
        }
    }
    const T &operator*()
    {
        return node.get().load();
    }

    ConcurrentView(const ConcurrentView&) = delete;
    ConcurrentView(ConcurrentView&&) = delete;
};

template<typename T, long MaxGenerations>
class LargeAtomic 
{
    static_assert(MaxGenerations >= 2);
private:
    std::atomic<size_t> head = 0;
    std::array<GenerationNode<T>, MaxGenerations> generations = {};
private:
    static size_t idx(size_t i) { return i % MaxGenerations; }
public:
    template<typename ... Args>
    LargeAtomic(Args&&... args)
    {
        assert(generations[0].tryStore(std::make_shared<T>(std::forward<Args>(args)...)));
    }
    ConcurrentView<T> load()
    {
        while(true)
        {
            size_t currHead = head.load();

            if(generations[idx(currHead)].tryAddOwner())
            {
                return ConcurrentView<T>{generations[idx(currHead)]};
            }
        }
    }

    
    template<typename ... Args>
        requires std::is_constructible_v<T, Args...>
    void store(Args&&... args)
    {
        std::shared_ptr<T> newT = std::make_shared<T>(std::forward<Args>(args)...);

        while(true)
        {
            size_t localHead = head.load();
            size_t nextHead = localHead + 1;

            if(!generations[idx(nextHead)].tryStore(newT))
            {
                continue;
            }
            /*
                Shift the head up one. In the initial case the head holds 
                an empty value so we check for this.
            */
            while(!generations[idx(localHead)].tryRemoveOwner())
            {
            }

            head.fetch_add(1);
            return;
        }
    }
};
