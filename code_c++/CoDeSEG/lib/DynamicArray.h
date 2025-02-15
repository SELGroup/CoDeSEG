//
// Created by Yantuan Xian on 2024/6/26.
//

#ifndef DYNARRAY_H
#define DYNARRAY_H
#include <vector>
#include <array>

#define BLK_SIZE 65536

template<typename T>
class DynamicArray {
    std::vector<std::array<T, BLK_SIZE> *> blocks;
    long last_idx = 0;

public:
    DynamicArray() = default;

    DynamicArray(const DynamicArray &other) {
        for (std::size_t i = 0; i < other.blocks.size(); i++) {
            const auto blk = new std::array<T, BLK_SIZE>;
            const auto src_blk = other.blocks[i];

            std::copy(src_blk->begin(), src_blk->end(), blk->begin());
            blocks.push_back(blk);
        }
        last_idx = other.last_idx;
    }

    DynamicArray &clear() {
        last_idx = 0;
        for (const auto el: blocks) {
            delete el;
        }
        return *this;
    }

    ~DynamicArray() {
        last_idx = 0;
        for (const auto el: blocks) {
            delete el;
        }
    }


    DynamicArray &copy_from(const DynamicArray &other) {
        for (const auto el: blocks) {
            delete el;
        }

        for (std::size_t i = 0; i < other.blocks.size(); i++) {
            const auto blk = new std::array<T, BLK_SIZE>;
            const auto src_blk = other.blocks[i];

            std::copy(src_blk->begin(), src_blk->end(), blk->begin());
            blocks.push_back(blk);
        }
        last_idx = other.last_idx;

        return *this;
    }

    [[nodiscard]] unsigned long size() const {
        return last_idx;
    }

    [[nodiscard]] unsigned long capacity() const {
        if (blocks.empty())
            return 0;
        return blocks.size() * BLK_SIZE;
    }

    void push(const T &el) {
        if (capacity() == last_idx) {
            const auto blk = new std::array<T, BLK_SIZE>;
            blocks.push_back(blk);
        }

        const auto idx = last_idx % BLK_SIZE;
        const auto blk = blocks.back();
        (*blk)[idx] = el;
        last_idx += 1;
    }

    T &operator[](const unsigned long index) {
        const auto i = index / BLK_SIZE;
        const auto j = index % BLK_SIZE;
        return blocks[i]->at(j);
    }
};
#endif //DYNARRAY_H
