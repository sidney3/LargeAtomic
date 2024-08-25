# LargeAtomic

A single reader single writer lock free atomic storage class that can hold elements of any size. All in 85 lines of C++. This is designed for a fast path reader and slow path writer, and so the reading is done without making any copies.

Takes optional `std::memory_order` parameters that mirror `std::atomic` functionality.

```cpp
// thread 1
LargeAtomic<std::vector<int>> a{};
std::vector<int> v = {1,2,3};
a.store(v);

// thread 2
assert(a.load() == v);
```

## Benchmarks

About 10 times as fast as a naive `std::mutex` implementation

About equally fast to `std::atomic<T>`. 

Note that we impose the constraint that our implementation be single reader, single writer.

## Implementation

Keep an circular buffer representing the history of states for our register.

Inside this array, keep two pointers,

`tail` := the latest position in our history buffer that is getting read from
`head` := the latest position in our history buffer that we have written to.

So each time we make a write, bump `head` up 1. Each time we make a read, return an object that, when destroyed, will move `tail` up to `head`. (Note that this allows us to work without copying)


### Jumping the tail

An interesting issue with the above comes if, for buffer sized `N`, we make `N` consecutive writes: we would be stuck waiting for the reader. However, as we know the reader will only ever read from `head`, we simply jump `head` to one position past the child:

```cpp
if(unlikely(nextHead == currTail))
{
  nextHead = (nextHead + 1) % MaxGenerations;
}
```

And construct our next `T` at this next position rather than `nextHead`
