# ditvector

A dynamic bitvector that support rank and select queries.

The datastructure allows for one template argument that defines the size of the bitblocks stored in the leafs.
This parameter can be used to select the appropriate trade-off between space and time complexity.
As a sensible default a size of 512 bits is used.

## Operations

The datastructure supports the following instructions which all have logarithmic runtime.
* `insert(index, true/false)`
* `del(index)`
* `set(index)`
* `unset(index)`
* `flip(index)`
* `rank(index, true/false)`
* `select(index, true/false)`
* `complement()`
* `size()` returns number of bits in bitvector
* `extract()` returns all bits as std::vector<bool>

## Usage

```c++
BitVector<16> bv;
bv.insert(0, false);
bv.insert(0, true);
bv.access(0);         // 1
bv.rank(1, true);     // 1
bv.flip(0);
bv.rank(1, true);     // 0
```

