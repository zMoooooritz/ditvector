
#ifndef BITVECTOR
#define BITVECTOR

#include "avl.hpp"

#include <vector>
#include <bitset>

// encapsualte the members that are needed for the bitvector tree structure
template <size_t S>
struct BV_Node : Node<BV_Node<S>> {
    #ifdef ADS_DEBUG
    uint32_t id;
    #endif
    uint32_t nums;
    uint32_t ones;
    std::bitset<S> *data;

    BV_Node() {
        #ifdef ADS_DEBUG
        id = rand() % 99;
        #endif
        nums = 0;
        ones = 0;
        data = new std::bitset<S>;
    }

    ~BV_Node() {
        delete data;
    }
};

// represents a dynamic bitvector that allows for inserts and deletes everywhere
//  as well as rank and select queries
template <size_t S = 512>
class BitVector : public AVL<BV_Node<S>> {
    private:
        size_t BLOCK_SIZE;
        size_t TARGET_SIZE;
        size_t SPLIT_BOUND;
        size_t LOWER_BOUND;

        std::bitset<S> FULL_MASK;
        std::bitset<S> MSB_MASK;
        std::bitset<S> LSB_MASK;

        BV_Node<S> *insert(BV_Node<S> *, uint32_t, bool);
        BV_Node<S> *del(BV_Node<S> *, uint32_t);
        void flip(BV_Node<S> *, uint32_t);
        void set(BV_Node<S> *, uint32_t);
        void unset(BV_Node<S> *, uint32_t);
        uint32_t rank(BV_Node<S> *, uint32_t, bool);
        uint32_t select(BV_Node<S> *, uint32_t, bool);
        bool access(BV_Node<S> *, uint32_t);
        void complement(BV_Node<S> *);
        BV_Node<S> *find_block(BV_Node<S> *, uint32_t*);

        #ifdef ADS_DEBUG
        void show(BV_Node<S> *);
        bool validate(BV_Node<S> *);
        #endif

        void propagate_update(BV_Node<S> *, BV_Node<S> *, int32_t, int32_t);

        void split_block_update(BV_Node<S> *, BV_Node<S> *, BV_Node<S> *);

        void steal_left(BV_Node<S> *, BV_Node<S> *);
        void steal_right(BV_Node<S> *, BV_Node<S> *);

        void merge_left_pre_update(BV_Node<S> *, BV_Node<S> *);
        void merge_right_pre_update(BV_Node<S> *, BV_Node<S> *);
        void merge_post_update(BV_Node<S> *);

        void rotate_left_update(BV_Node<S> *);
        void rotate_right_update(BV_Node<S> *);

    public:
        void insert(uint32_t, bool);
        void del(uint32_t);
        void flip(uint32_t);
        void set(uint32_t);
        void unset(uint32_t);
        uint32_t rank(uint32_t, bool);
        uint32_t select(uint32_t, bool);
        bool access(uint32_t);
        void complement();
        std::vector<bool> extract();

        #ifdef ADS_DEBUG
        void show();
        bool validate();
        #endif

        uint32_t operator[](uint32_t);
        void operator~();

        BitVector();
        BitVector(std::vector<bool>);
};

#endif

