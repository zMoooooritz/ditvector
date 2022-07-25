#include "bit_vector.cpp"

#include <chrono>

const size_t BLOCK_SIZE = 512;

#ifdef ADS_DEBUG
bool succ(std::string name) {
    std::cout << " The test \"" << name << "\" did run successfully" << std::endl; 
    return true;
}

bool succ(std::string name, int time) {
    std::cout << " The test \"" << name << "\" did run successfully in "  << time << " ms" << std::endl; 
    return true;
}

bool fail(std::string name) {
    std::cout << " The test \"" << name << "\" did not run successfully" << std::endl; 
    return false;
}

std::vector<bool> get_start_configuration(std::string bit_string) {
    std::vector<bool> bits;
    bits.reserve(bit_string.size());
    for (auto bit : bit_string)
        bits.push_back(bit == '1');
    return bits;
}

std::vector<bool> get_default_start_configuration() {
    std::string bit_string("1001010101011110101010101010111101010101010101010110101010101010");
    return get_start_configuration(bit_string);
}

bool test_bv_insert() {
    std::string name = "bv insert";
    BitVector<BLOCK_SIZE> bv;
    bv.insert(0, false);
    bv.insert(0, true);
    bv.validate();
    if (bv.access(0) && !bv.access(1))
        return succ(name);
    return fail(name);
}

bool test_bv_select() {
    std::string name = "bv select";
    BitVector<BLOCK_SIZE> bv(get_default_start_configuration());
    if (bv.select(5, true) == 9 && bv.select(29, false) == 63 && bv.select(1, true) == 0)
        return succ(name);
    return fail(name);
}

bool test_bv_rank() {
    std::string name = "bv rank";
    BitVector<BLOCK_SIZE> bv(get_default_start_configuration());
    if (bv.rank(0, true) == 0 && bv.rank(11, true) == 5 && bv.rank(63, false) == 28)
        return succ(name);
    return fail(name);
}

bool test_bv_extact() {
    std::string name = "bv extract";
    std::vector<bool> bits = get_default_start_configuration();
    BitVector<BLOCK_SIZE> bv(bits);
    std::vector<bool> new_bits = bv.extract();
    if (bits == new_bits)
        return succ(name);
    return fail(name);
}

bool test_bv_set() {
    std::string name = "bv set";
    BitVector<BLOCK_SIZE> bv(get_default_start_configuration());
    bv.set(1);
    bv.set(63);
    if (bv[1] && bv[63])
        return succ(name);
    return fail(name);
}

bool test_bv_unset() {
    std::string name = "bv unset";
    BitVector<BLOCK_SIZE> bv(get_default_start_configuration());
    bv.unset(0);
    bv.unset(3);
    bv.unset(62);
    if (!bv[0] && !bv[3] && !bv[62])
        return succ(name);
    return fail(name);
}

bool test_bv_flip() {
    std::string name = "bv flip";
    BitVector<BLOCK_SIZE> bv(get_default_start_configuration());
    bv.flip(0);
    bv.flip(1);
    bv.flip(2);
    bv.flip(3);
    if (!bv[0] && bv[1] && bv[2] && !bv[3])
        return succ(name);
    return fail(name);
}

bool test_bv_insdel() {
    std::string name = "bv insert/delete";
    std::string bit_string("10010101010111101010101010101111010101010101010101101010101010101001111000111111010010010110010110010101010101010101011110010111");
    std::vector<bool> bits = get_start_configuration(bit_string);
    BitVector<BLOCK_SIZE> bv(bits);
    bv.validate();
    for (int i = 0; i < 100; i++)
        bv.insert(0, i % 2);
    if (!bv.validate())
        return fail(name);
    for (int i = 0; i < 100; i++)
        bv.del(0);
    if (!bv.validate())
        return fail(name);
    for (int i = 0; i < 100; i++)
        bv.insert(40, i % 2);
    if (!bv.validate())
        return fail(name);
    for (int i = 0; i < 100; i++)
        bv.del(40);
    if (!bv.validate())
        return fail(name);
    for (int i = 0; i < 100; i++)
        bv.insert(127, i % 2);
    if (!bv.validate())
        return fail(name);
    for (int i = 0; i < 100; i++)
        bv.del(127);
    if (!bv.validate())
        return fail(name);
    for (int i = 0; i < 100; i++)
        bv.insert(128, i % 2);
    if (!bv.validate())
        return fail(name);
    for (int i = 0; i < 100; i++)
        bv.del(128);
    if (!bv.validate())
        return fail(name);
    std::vector<bool> new_bits = bv.extract();
    if (bits == new_bits)
        return succ(name);
    return fail(name);
}

bool test_bv_rdm_insdel() {
    std::string name = "bv rdm insert/delete";
    std::vector<uint32_t> indices;
    BitVector<BLOCK_SIZE> bv;
    for (int i = 0; i < 100000; i++) {
        uint32_t index = rand() % (i + 1);
        indices.push_back(index);
        bv.insert(index, i % 2);
    }
    if (!bv.validate())
        return fail(name);
    for (int i = indices.size() - 1; i >= 0; i--)
        bv.del(indices[i]);
    if (!bv.validate())
        return fail(name);

    std::vector<bool> bits = bv.extract();
    if (bits.size() == 0)
        return succ(name);
    std::cout << "Used elements that caused an error: " << std::endl;
    for (int i = indices.size() - 1; i >= 0; i--)
        std::cout << indices[i] << " ";
    std::cout << std::endl;
    return fail(name);
}

bool test_bv_big_insdel() {
    std::string name = "bv big insert/delete";
    auto start = std::chrono::system_clock::now();
    uint32_t count = 10000;
    BitVector<BLOCK_SIZE> bv;
    for (uint32_t i = 0; i < count - 1; i++)
        bv.insert(0, i % 2);
    bv.insert(0, 1);
    if (!bv.validate())
        return fail(name);
    std::vector<bool> bits = bv.extract();
    if (bits.size() != count)
        return fail(name);
    for (uint32_t i = 0; i < count; i++)
        bv.del(0);
    auto end = std::chrono::system_clock::now();
    int time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    bits = bv.extract();
    if (bits.size() == 0)
        return succ(name, time);
    return fail(name);
}
#endif

std::pair<long long, long long> benchmark_bv(uint32_t count) {
    const size_t BLK_SIZE = 8;
    auto start = std::chrono::system_clock::now();
    BitVector<BLK_SIZE> bv;
    for (uint32_t i = 0; i < count; i++)
        bv.insert(0, i % 2);
    for (uint32_t i = 0; i < count; i++)
        bv.rank(i / 2 + 1, i % 2);
    uint32_t tree_size = bv.tree_size();
    for (uint32_t i = 0; i < count; i++)
        bv.select(i / 2 + 1, i % 2);
    for (uint32_t i = 0; i < count; i++)
        bv.del(0);
    auto end = std::chrono::system_clock::now();
    long long time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    long long size = (tree_size * sizeof(BV_Node<BLK_SIZE>)) * 8 + ((tree_size + 1) / 2) * BLK_SIZE;
    
    return std::make_pair(time, size);
}

int main(int argc, char *argv[]) {

    bool benchmark = true;

    if (benchmark) {

        std::vector<int> counts {};
        for (int i = 10; i < 26; i++)
            counts.push_back(1 << i);

        for (auto count : counts) {
            std::pair<long long, long long> p = benchmark_bv(count);
            long long time = p.first;
            long long size = p.second;
            std::cout << "RESULT"
                << " time=" << time
                << " space=" << size
                << std::endl;
        }
    } else {

        bool test_result = true;

        std::cout << "================================" << std::endl << std::endl;;

        #ifdef ADS_DEBUG
        test_result &= test_bv_insert();
        test_result &= test_bv_select();
        test_result &= test_bv_rank();
        test_result &= test_bv_extact();
        test_result &= test_bv_set();
        test_result &= test_bv_unset();
        test_result &= test_bv_flip();
        test_result &= test_bv_insdel();
        test_result &= test_bv_rdm_insdel();
        test_result &= test_bv_big_insdel();

        #endif

        std::cout << std::endl << "================================" << std::endl;
        if (test_result)
            std::cout << "  All tests passed" << std::endl;
        else
            std::cout << "  At least one test failed" << std::endl;

        return test_result ? 0 : 1;
    }
}


