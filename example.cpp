#include "bit_vector.cpp"

const size_t BLOCK_SIZE = 64;
    
int main(int argc, char *argv[]) {

    BitVector<BLOCK_SIZE> bv;
    bv.insert(0, false);
    bv.insert(0, true);
    std::cout << bv.access(0) << std::endl;
    std::cout << bv.rank(1, true) << std::endl;
    bv.flip(0);
    std::cout << bv.rank(1, true) << std::endl;
    bv.del(0);
    std::cout << bv.size() << std::endl;
}

