# LargeAtomic

A lock-less single writer many reader atomic storage class that can hold elements of any size. Always respects acquire-release semantics.

```cpp
// thread 1
LargeAtomic<std::vector<int>> a{};
std::vector<int> v = {1,2,3};
a.store(v);

// thread 2
assert(*a.load() == v);
```

## Implementation

The fundamental problem with such a structure is figuring out when we can free the memory that we have allocated for our object. Because there can be any number of readers viewing potentially many outdated versions of the object, this is hard to figure out. So we save all of the histories of our object that are currently getting observed, and keep reference counts for each one.

Conceptually, we create a list representing the history of `T`, as well as a pointer to the most recent version. When we want to retrieve the value, we return where this pointer points. When we want to add a value, we shift the pointer up one.

_More detailed explanation_:

For some type `T`, create an array to represent the history of `T` over time.

```
T0, T1, T2, T3
```

And keep a `head` variable that tracks the current state. Each of these values in our history array is a `GenerationNode`.

Each time we want to read a value, we retrieve the history value held at `head` and add an "owner" to it. When we are done reading the value, we decrement the owner from this item.

When we want to modify a value, we shift `head` up 1, add an owner to head, and decrement an owner from the position before head.
