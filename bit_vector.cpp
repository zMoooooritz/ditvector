#include "bit_vector.hpp"

#include <algorithm>   // used for the std::min operation

template <size_t S>
BitVector<S>::BitVector() : AVL<BV_Node<S>>() {
    BLOCK_SIZE = S;
    TARGET_SIZE = BLOCK_SIZE / 2;
    SPLIT_BOUND = (BLOCK_SIZE * 3) / 4;
    LOWER_BOUND = BLOCK_SIZE / 4;

    std::string fmask = std::string(BLOCK_SIZE, '1');
    std::string mmask = std::string(TARGET_SIZE, '1') + std::string(TARGET_SIZE, '0');
    std::string lmask = std::string(TARGET_SIZE, '0') + std::string(TARGET_SIZE, '1');

    FULL_MASK = std::bitset<S>(fmask);
    MSB_MASK  = std::bitset<S>(mmask);
    LSB_MASK  = std::bitset<S>(lmask);
}

// construct the bitvector tree structure from the provided bool vector
template <size_t S>
BitVector<S>::BitVector(std::vector<bool> bits) : BitVector() {
    uint32_t num_leafs = (bits.size() + TARGET_SIZE - 1) / TARGET_SIZE;

    this->build_balanced_tree(NULL, num_leafs);
    BV_Node<S> *leaf = this->root;
    while (leaf->l)
        leaf = leaf->l;
    for (uint32_t i = 0; i < num_leafs; i++) {
        uint32_t count = 0;
        for (uint32_t j = 0; j < TARGET_SIZE && i * TARGET_SIZE + j < bits.size(); j++,count++)
            (*leaf->data)[BLOCK_SIZE - j - 1] = bits[(i * TARGET_SIZE) + j];
        propagate_update(leaf, NULL, count, (*leaf->data).count());
        leaf = this->next_leaf(leaf);
    }
}

template <size_t S>
void BitVector<S>::insert(uint32_t index, bool value) {
    this->root = insert(this->root, index, value);
}

template <size_t S>
void BitVector<S>::del(uint32_t index) {
    this->root = del(this->root, index);
}

template <size_t S>
void BitVector<S>::flip(uint32_t index) {
    flip(this->root, index);
}

template <size_t S>
void BitVector<S>::set(uint32_t index) {
    set(this->root, index);
}

template <size_t S>
void BitVector<S>::unset(uint32_t index) {
    unset(this->root, index);
}

template <size_t S>
uint32_t BitVector<S>::rank(uint32_t index, bool value) {
    return rank(this->root, index, value);
}

template <size_t S>
uint32_t BitVector<S>::select(uint32_t index, bool value) {
    return select(this->root, index, value);
}

template <size_t S>
bool BitVector<S>::access(uint32_t index) {
    return access(this->root, index);
}

template <size_t S>
void BitVector<S>::complement() {
    complement(this->root);
}

template <size_t S>
uint32_t BitVector<S>::size() {
    return size(this->root);
}

// collect all the bits in the bitvector and return it as one consecutive bool vector
template <size_t S>
std::vector<bool> BitVector<S>::extract() {
    BV_Node<S> *node = this->root;
    while (node->l)
        node = node->l;
    std::vector<bool> bits;
    while (node) {
        for (uint32_t i = 0; i < node->nums; i++)
            bits.push_back((*node->data)[BLOCK_SIZE - i - 1]);
        node = this->next_leaf(node);
    }
    return bits;
}

template <size_t S>
uint32_t BitVector<S>::operator[](uint32_t index) {
    return access(this->root, index);
}

template <size_t S>
void BitVector<S>::operator~() {
    complement(this->root);
}

// nserts the provided value (either 0 or 1) into the bitvector at the given index
// in case the leaf of the insertion block is full this node needs to be split
template <size_t S>
BV_Node<S> *BitVector<S>::insert(BV_Node<S> *node, uint32_t index, bool value) {
    if (!node)
        node = new BV_Node<S>;

    // find the block where the index is located (updates index accordingly)
    BV_Node<S> *leaf = find_block(node, &index);

    if (index > BLOCK_SIZE) {
        std::cout << "Invalid index for insert operation (skipping operation)" << std::endl;
        return node;
    }
    
    // block is full; a split is required
    // it might be necessary to balance the tree afterwards
    if (leaf->nums >= BLOCK_SIZE) {
        this->split_block(leaf);
        leaf = find_block(leaf, &index);
        node = this->fix_tree(leaf);
    }

    // update the data of the node to include the new bit
    // propagate the changes up the tree
    std::bitset<S> msb = *leaf->data & ~(FULL_MASK >> index);
    std::bitset<S> lsb = *leaf->data & (FULL_MASK >> index);
    std::bitset<S> new_bit;
    if (value)
        new_bit.set(BLOCK_SIZE - index - 1);
    *leaf->data = msb | new_bit | (lsb >> 1);
    propagate_update(leaf, NULL, (uint64_t) 1 + std::max((int64_t) 0, (int64_t) index-leaf->nums), value ? 1 : 0);
    return node;
}

// remove the bit specified by the index from the bitvector 
// in case the resulting leaf has too few elements it is required to steal bits or merge with another leaf
template <size_t S>
BV_Node<S> *BitVector<S>::del(BV_Node<S> *node, uint32_t index) {
    // finds the block where the index is located (updates index accordingly)
    BV_Node<S> *leaf = find_block(node, &index);

    if (index < 0 || index >= BLOCK_SIZE) {
        std::cout << "Invalid index for delete operation (skipping operation)" << std::endl;
        return node;
    }

    // update the data of the node to exclude the bit
    // propagate the changes up the tree
    std::bitset<S> msb = *leaf->data & ~(FULL_MASK >> index);
    std::bitset<S> lsb = *leaf->data & (FULL_MASK >> (index + 1));
    int8_t value  = (*leaf->data)[BLOCK_SIZE - index - 1] == 0 ? 0 : -1;
    *leaf->data = msb | (lsb << 1);
    propagate_update(leaf, NULL, -1, value);

    if (leaf->nums > LOWER_BOUND)
        return node;

    BV_Node<S> *prev = this->prev_leaf(leaf);
    BV_Node<S> *next = this->next_leaf(leaf);

    // if there are no other leafs just return; nothing to do
    if (!prev && !next)
        return node;

    if (prev && !next) {                     // use the previous leaf for stealing / merging
        if (prev->nums >= SPLIT_BOUND)
            steal_left(leaf, prev);          // steal from the previous leaf (since it has sufficient bits)
        else
            return this->merge_left(leaf, prev);   // merge with previous leaf
        return node;
    } else if (!prev && next) {              // use the next leaf for stealing / merging
        if (next->nums >= SPLIT_BOUND)
            steal_right(leaf, next);         // steal from the next leaf (since it has sufficient bits)
        else
            return this->merge_right(leaf, next);  // merge with next leaf
        return node;
    }

    if (prev->nums >= SPLIT_BOUND || next->nums >= SPLIT_BOUND) { // steal bits since both 'neighbour' leafs have enought bits
        if (prev->nums > next->nums) {
            steal_left(leaf, prev);                               // steal left since it has more bits
        } else {
            steal_right(leaf, next);                              // steal right since it has more bits
        }
        return node;
    } else {                                                      // both 'neighbour' leafs have only few bits
        if (prev->nums < next->nums) {
            return this->merge_left(leaf, prev);                        // merge left since it has less bits
        } else {
            return this->merge_right(leaf, next);                       // merge right since it has less bits
        }
    }
    return node;
}

// flip the content of the bit addressed by index
template <size_t S>
void BitVector<S>::flip(BV_Node<S> *node, uint32_t index) {
    BV_Node<S> *leaf = find_block(node, &index);

    int8_t value = (*leaf->data)[BLOCK_SIZE - index - 1] > 0 ? -1 : 1;
    (*leaf->data).flip(BLOCK_SIZE - index - 1);
    propagate_update(leaf, NULL, 0, value);
}

// set the bit addressed by index
template <size_t S>
void BitVector<S>::set(BV_Node<S> *node, uint32_t index) {
    BV_Node<S> *leaf = find_block(node, &index);

    int8_t value = (*leaf->data)[BLOCK_SIZE - index - 1] > 0 ? 0 : 1;
    (*leaf->data).set(BLOCK_SIZE - index - 1);
    propagate_update(leaf, NULL, 0, value);
}

// unset the bit addressed by index
template <size_t S>
void BitVector<S>::unset(BV_Node<S> *node, uint32_t index) {
    BV_Node<S> *leaf = find_block(node, &index);

    int8_t value = (*leaf->data)[BLOCK_SIZE - index - 1] > 0 ? -1 : 0;
    (*leaf->data).reset(BLOCK_SIZE - index - 1);
    propagate_update(leaf, NULL, 0, value);
}

// calculate the number of occurrences of value in the bitvector up to index
template <size_t S>
uint32_t BitVector<S>::rank(BV_Node<S> *node, uint32_t index, bool value) {
    if (this->is_leaf(node)) {
        std::bitset<S> data = *node->data & ~(FULL_MASK >> index);
        return value ? data.count() : std::min(node->nums, (uint32_t) index) - data.count();
    }

    // use the information stored in the inner nodes and recursion to quickly calcualte the rank
    uint32_t num_val = value ? node->ones : node->nums - node->ones;
    if (index < node->nums)
        return rank(node->l, index, value);
    else
        return num_val + rank(node->r, index-node->nums, value);
}

// calculate the index of the num'th occurrence of value in the bitvector
template <size_t S>
uint32_t BitVector<S>::select(BV_Node<S> *node, uint32_t num, bool value) {
    if (this->is_leaf(node)) {
        if ((value ? node->ones : node->nums - node->ones) < num) {
            std::cout << "Invalid num for select operation (returning invalid value)" << std::endl;
            return -1;
        }

        uint32_t count = 0;
        std::bitset<S> data = *node->data;
        for (uint32_t i = 0; i < node->nums; i++) {
            if (((data[BLOCK_SIZE - i - 1] > 0) == value) && ++count == num)
                return i;
        }
    }

    uint32_t num_val = value ? node->ones : node->nums - node->ones;
    if (num <= num_val) {
        return select(node->l, num, value);
    } else {
        return node->nums + select(node->r, num - num_val, value);
    }
}

// return the bit that is located at index in the bitvector
template <size_t S>
bool BitVector<S>::access(BV_Node<S> *node, uint32_t index) {
    node = find_block(node, &index);
    return (*node->data)[BLOCK_SIZE - index - 1] > 0;
}

// invert the bitvector so that each 0 becomes a 1 and vice versa
template <size_t S>
void BitVector<S>::complement(BV_Node<S> *node) {
    if (!node)
        return;

    node->ones = node->nums - node->ones;
    if (this->is_leaf(node)) {
        *node->data = (*node->data).flip() & ~(FULL_MASK >> node->nums);
    } else {
        complement(node->l);
        complement(node->r);
    }
}

// calculate the number of bits that are stored in the structure
template <size_t S>
uint32_t BitVector<S>::size(BV_Node<S> *node) {
    if (!node)
        return 0;
    return node->nums + size(node->r);
}

// find the node (always a leaf) that contains the bit at the position index
// index is updated as well to locate the bit inside the leaf block
template <size_t S>
BV_Node<S> *BitVector<S>::find_block(BV_Node<S> *node, uint32_t* index) {
    if (this->is_leaf(node))
        return node;
    if (*index < node->nums)
        return find_block(node->l, index);
    *index -= node->nums;
    return find_block(node->r, index);
}

// propagate changes in nodes up the tree to keep the navigation structure correct
template <size_t S>
void BitVector<S>::propagate_update(BV_Node<S> *node, BV_Node<S> *prev_node, int32_t nums, int32_t ones) {
    if (!node)
        return;

    if (node->l == prev_node) {
        node->nums += nums;
        node->ones += ones;
    }
    if (this->is_leaf(node)) {
        node->height = 1;
    } else {
        uint8_t height_l = node->l->height;
        uint8_t height_r = node->r->height;
        node->height = 1 + (height_l > height_r ? height_l : height_r);
    }
    propagate_update(node->p, node, nums, ones);
}

// update the data in the three nodes (parent and both child nodes) involved in the operation
template <size_t S>
void BitVector<S>::split_block_update(BV_Node<S> *node, BV_Node<S> *left, BV_Node<S> *right) {
    *left->data = *node->data & MSB_MASK;
    *right->data = (*node->data & LSB_MASK) << TARGET_SIZE;
    delete node->data;
    node->data = NULL;
    left->nums = TARGET_SIZE;
    right->nums = TARGET_SIZE;
    node->nums = TARGET_SIZE;
    left->ones = (*left->data).count();
    right->ones = (*right->data).count();
    node->ones = left->ones;
    propagate_update(node, NULL, 0, 0);
}

// take some bits from the left 'neighbour' leaf and add them to node
// this ensures that the tree remains balanced; afterwards propagate the changes
template <size_t S>
void BitVector<S>::steal_left(BV_Node<S> *node, BV_Node<S> *prev_leaf) {
    uint32_t steal_bits = (prev_leaf->nums - node->nums) / 2;
    std::bitset<S> steal_data = (*prev_leaf->data) >> (BLOCK_SIZE - prev_leaf->nums) & (FULL_MASK >> (BLOCK_SIZE - steal_bits));

    *prev_leaf->data &= (FULL_MASK << (BLOCK_SIZE - (prev_leaf->nums - steal_bits)));
    *node->data = (steal_data << (BLOCK_SIZE - steal_bits)) | (*node->data >> steal_bits);

    uint32_t ones = steal_data.count();
    propagate_update(node, NULL, steal_bits, ones);
    propagate_update(prev_leaf, NULL, -steal_bits, -ones);
}

// take some bits from the right 'neighbour' leaf and add them to node
// this ensures that the tree remains balanced; afterwards propagate the changes
template <size_t S>
void BitVector<S>::steal_right(BV_Node<S> *node, BV_Node<S> *next_leaf) {
    uint32_t steal_bits = (next_leaf->nums - node->nums) / 2;
    std::bitset<S> steal_data  = *next_leaf->data >> (BLOCK_SIZE - steal_bits);

    *next_leaf->data <<= steal_bits;
    *node->data = *node->data | (steal_data << (BLOCK_SIZE - node->nums - steal_bits));

    uint32_t ones = steal_data.count();
    propagate_update(node, NULL, steal_bits, ones);
    propagate_update(next_leaf, NULL, -steal_bits, -ones);
}

// process the changes required after a left merge
template <size_t S>
void BitVector<S>::merge_left_pre_update(BV_Node<S> *node, BV_Node<S> *prev_leaf) {
    *node->data = *prev_leaf->data | (*node->data >> prev_leaf->nums);
    propagate_update(node, NULL, prev_leaf->nums, prev_leaf->ones);
    propagate_update(prev_leaf, NULL, -prev_leaf->nums, -prev_leaf->ones);
}

// process the changes required after a right merge
template <size_t S>
void BitVector<S>::merge_right_pre_update(BV_Node<S> *node, BV_Node<S> *next_leaf) {
    *node->data = *node->data | (*next_leaf->data >> node->nums);
    propagate_update(node, NULL, next_leaf->nums, next_leaf->ones);
    propagate_update(next_leaf, NULL, -next_leaf->nums, -next_leaf->ones);
}

template <size_t S>
void BitVector<S>::merge_post_update(BV_Node<S> *node) {
    propagate_update(node, NULL, 0, 0);
}

// process the changes required after a left rotation
template <size_t S>
void BitVector<S>::rotate_left_update(BV_Node<S> *node) {
    node->nums += node->l->nums;
    node->ones += node->l->ones;
    propagate_update(node->l, NULL, 0, 0);
}

// process the changes required after a left rotation
template <size_t S>
void BitVector<S>::rotate_right_update(BV_Node<S> *node) {
    node->r->nums -= node->nums;
    node->r->ones -= node->ones;
    propagate_update(node->r, NULL, 0, 0);
}

#ifdef ADS_DEBUG
template <size_t S>
void BitVector<S>::show() {
    std::cout << std::endl;
    show(this->root);
}

template <size_t S>
bool BitVector<S>::validate() {
    bool val = validate(this->root);
    if (!val) {
        std::cout << "Nicht valider Baum" << std::endl;
    }
    return val;
}

// print the content of the bitvector and the current configuration of tree to std::out
// mainly used for dabugging purposes
template <size_t S>
void BitVector<S>::show(BV_Node<S> *node) {
    if (!node)
        return;

    uint32_t ht = this->node_depth(node);
    std::string indent1 = "+";
    std::string indent2 = "| ";
    for (uint32_t i = 0; i < 2 * ht; i++) {
        indent1.append("-");
        indent2.append(" ");
    }
    if (ht == 0)
        std::cout << indent1 << "Root" << std::endl;
    else if (this->is_leaf(node))
        std::cout << indent1 << "Leaf" << std::endl;
    else
        std::cout << indent1 << "BV_Node<S>" << std::endl;
    std::cout << indent2 << "id  :   " << node->id << std::endl;
    std::cout << indent2 << "nums:   " << node->nums << std::endl;
    std::cout << indent2 << "ones:   " << node->ones << std::endl;
    std::cout << indent2 << "height: " << node->height << std::endl;
    if (node->data != NULL)
        std::cout << indent2 << "data: " << *node->data << std::endl;
    std::cout <<  "|" << std::endl;
    show(node->l);
    show(node->r);
}

template <size_t S>
bool BitVector<S>::validate(BV_Node<S> *node) {
    if (this->is_leaf(node)) {
        if (node->ones == node->data->count())
            return true;
        return false;
    }
    uint32_t nums = 0;
    uint32_t ones = 0;
    BV_Node<S> *iter = node->l;
    while (iter) {
        nums += iter->nums;
        ones += iter->ones;
        iter = iter->r;
    }
    
    if (node->nums != nums || node->ones != ones || node->height != std::max(node->l->height, node->r->height) + 1)
        return false;
    return validate(node->l) && validate(node->r);
}
#endif

