#ifndef AVL_DEF
#define AVL_DEF

/* #define ADS_DEBUG */

#include <iostream>
#include <cstdint>
#include <cstddef>

// encapsualte the base members that are needed for a tree structure
template <typename T>
struct Node {
    T *p;
    T *l;
    T *r;
    uint8_t height;

    Node() {
        p = NULL;
        l = NULL;
        r = NULL;
        height = 1;
    }

    ~Node() {
        delete l;
        delete r;
    }
};

// AVL tree that allows for a template node type and customizable merge/steal/rotate actions
template <typename T>
class AVL {
    protected:
        bool is_leaf(T *);
        void split_block(T *);
        uint32_t node_depth(T *);
        uint32_t tree_size(T *);

        T *next_leaf(T *);
        T *prev_leaf(T *);
        T *merge_left(T *, T *);
        T *merge_right(T *, T *);
        T *fix_tree(T *);

        T *rotate_right(T *);
        T *rotate_left(T *);
        T *rotate_right_left(T *);
        T *rotate_left_right(T *);
        uint32_t height(T *);
        int32_t difference(T *);
        T *balance(T *);
        T *build_balanced_tree(T *, uint32_t);

        virtual void split_block_update(T *, T *, T *) = 0;

        virtual void steal_left(T *, T *) = 0;
        virtual void steal_right(T *, T *) = 0;

        virtual void merge_left_pre_update(T *, T *) = 0;
        virtual void merge_right_pre_update(T *, T *) = 0;
        virtual void merge_post_update(T *) = 0;

        virtual void rotate_left_update(T *) = 0;
        virtual void rotate_right_update(T *) = 0;

        T *root;

    public:
        AVL();
        ~AVL();
        uint32_t tree_size();
};

// create the root node of the tree
template <class T>
AVL<T>::AVL() {
    root = new T;
}

// deconstruct the full tree
template <class T>
AVL<T>::~AVL() {
    delete root;
}

// calcuale the size (number of nodes) of the tree
template <class T>
uint32_t AVL<T>::tree_size() {
    return tree_size(root);
}

// return whether or not this node is a leaf (has no child nodes)
template <class T>
bool AVL<T>::is_leaf(T *node) {
    return !node->l && !node->r;
}

// split the provided node (is a leaf)
// replace the leaf with an 'inner' node and attach two new leaves as childs
template <class T>
void AVL<T>::split_block(T *node) {
    T *new_left = new T;
    T *new_right = new T;
    node->l = new_left;
    node->r = new_right;
    new_left->p = node;
    new_right->p = node;
    split_block_update(node, new_left, new_right);
}

// calculates the number of edges on the direct path to the root node
template <class T>
uint32_t AVL<T>::node_depth(T *node) {
    return !node->p ? 0 : 1 + node_depth(node->p);
}

// calculates the tree size (the number of nodes in the tree)
template <class T>
uint32_t AVL<T>::tree_size(T *node) {
    return 1 + (is_leaf(node) ? 0 : tree_size(node->l) + tree_size(node->r));
}

// merge the leaf with the left 'neighbour' leaf
// this ensures that the tree remains compact; afterwards propagate the changes
template <class T>
T *AVL<T>::merge_left(T *node, T* prev_leaf) {
    merge_left_pre_update(node, prev_leaf);

    T *node_p;
    T *update_node;
    if (node->p == prev_leaf->p) {
        node_p = node->p;
        node->p = node_p->p;
        if (node_p->p)
            node_p->p->r == node_p ? node_p->p->r = node : node_p->p->l = node;
        update_node = node;
    } else {
        node_p = prev_leaf->p;
        T *leaf_neighbour = node_p->l == prev_leaf ? node_p->r : node_p->l;
        leaf_neighbour->p = node_p->p;
        if (node_p->p)
            node_p->p->r == node_p ? node_p->p->r = leaf_neighbour : node_p->p->l = leaf_neighbour;
        update_node = leaf_neighbour;
    }

    merge_post_update(update_node);

    node = fix_tree(node);
    node_p->l = NULL;
    node_p->r = NULL;
    delete prev_leaf;
    delete node_p;
    return node;
}

// merge the leaf with the right 'neighbour' leaf
// this ensures that the tree remains compact; afterwards propagate the changes
template <class T>
T *AVL<T>::merge_right(T *node, T* next_leaf) {
    merge_right_pre_update(node, next_leaf);

    T *node_p;
    T *update_node;
    if (node->p == next_leaf->p) {
        node_p = node->p;
        node->p = node_p->p;
        if (node_p->p)
            node_p->p->r == node_p ? node_p->p->r = node : node_p->p->l = node;
        update_node = node;
    } else {
        node_p = next_leaf->p;
        T *leaf_neighbour = node_p->r == next_leaf ? node_p->l : node_p->r;
        leaf_neighbour->p = node_p->p;
        if (node_p->p)
            node_p->p->r == node_p ? node_p->p->r = leaf_neighbour : node_p->p->l = leaf_neighbour;
        update_node = leaf_neighbour;
    }

    merge_post_update(update_node);

    node = fix_tree(node);
    node_p->l = NULL;
    node_p->r = NULL;
    delete next_leaf;
    delete node_p;
    return node;
}

// iterate the tree from the provided node up to the root
// in case a node is unbalanced rebalance the tree
template <class T>
T *AVL<T>::fix_tree(T *node) {
    while (node->p) {
        node = node->p;
        node = balance(node);
    }
    return node;
}

// find the left 'neighbour' leaf and return it
template <class T>
T *AVL<T>::prev_leaf(T *node) {
    T *curr = NULL;
    T *next = node;

    while (next && curr == next->l) {
        curr = next;
        next = next->p;
    }
    if (!next)
        return NULL;

    curr = next->l;

    while (curr && curr->r)
        curr = curr->r;
    return curr;
}

// find the right 'neighbour' leaf and return it
template <class T>
T *AVL<T>::next_leaf(T *node) {
    T *curr = NULL;
    T *next = node;

    while (next && curr == next->r) {
        curr = next;
        next = next->p;
    }
    if (!next)
        return NULL;

    curr = next->r;

    while (curr && curr->l)
        curr = curr->l;
    return curr;
}

// perform a single left rotation on the provided node in order to balance the tree
// update the content of the involved noes accordingly
template <class T>
T *AVL<T>::rotate_left(T *node) {
    T *r = node->r;
    T *node_p = node->p;

    node->r = r->l;
    if (node_p)
        node_p->r == node ? node_p->r = r : node_p->l = r;
    node->r->p = node;
    node->p = r;

    r->l = node;
    r->p = node_p;

    rotate_left_update(r);
    return r;
}

// perform a single right rotation on the provided node in order to balance the tree
// update the content of the involved noes accordingly
template <class T>
T *AVL<T>::rotate_right(T *node) {
    T *l = node->l;
    T *node_p = node->p;

    node->l = l->r;
    if (node_p)
        node_p->r == node ? node_p->r = l : node_p->l = l;
    node->l->p = node;
    node->p = l;

    l->r = node;
    l->p = node_p;

    rotate_right_update(l);
    return l;
}

// perform a left rotation and afterwards a right rotation on the provided node in order to balance the tree
// update the content of the involved noes accordingly
template <class T>
T *AVL<T>::rotate_left_right(T *node) {
    T *l = node->l;
    node->l = rotate_left(l);
    return rotate_right(node);
}

// perform a right rotation and afterwards a left rotation on the provided node in order to balance the tree
// update the content of the involved noes accordingly
template <class T>
T *AVL<T>::rotate_right_left(T *node) {
    T *r = node->r;
    node->r = rotate_right(r);
    return rotate_left(node);
}

// calculate the height of a node (max number of descents to a leaf)
template <class T>
uint32_t AVL<T>::height(T *node) {
    if (!node)
        return 0;
    if (is_leaf(node))
        return 1;
    return 1 + (node->l->height > node->r->height ? node->l->height : node->r->height);
}

// calculate the height difference of the childs of the node
template <class T>
int32_t AVL<T>::difference(T *node) {
    return height(node->l) - height(node->r); 
}

// determine if the tree is unbalanced at the provided node
// in case the it is unbalanced determine to which side and apply the matching rotation
template <class T>
T *AVL<T>::balance(T *node) {
    int32_t factor = difference(node);

    if (factor > 1) {                        // unbalanced to the left side
        if (difference(node->l) > 0)
            node = rotate_right(node);
        else
            node = rotate_left_right(node);
    } else if (factor < -1) {                // unbalanced to the right side
        if (difference(node->r) > 0)
            node = rotate_right_left(node);
        else
            node = rotate_left(node);
    }
    return node;
}

// given a required number of leafs construct a balanced binary tree (notably an avl tree) that has that many leafs
template <class T>
T *AVL<T>::build_balanced_tree(T *parent, uint32_t num_leafs) {
    if (num_leafs == 0)
        return NULL;

    T *node;
    if (parent) {
        node = new T;
        node->p = parent;
    } else {
        node = root;
    }

    if (num_leafs == 1)
        return node;

    uint32_t left_num_leafs = num_leafs / 2;
    uint32_t right_num_leafs = num_leafs - left_num_leafs;

    node->l = build_balanced_tree(node, left_num_leafs);
    node->r = build_balanced_tree(node, right_num_leafs);

    return node;
}

#endif

