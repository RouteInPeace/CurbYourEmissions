#pragma once

#include <cstddef>
#include <cstdlib>

namespace alns {

class RandomOperatorSelection {
    public:
        RandomOperatorSelection(size_t size) : size_(size) {}

        size_t select_operator() {
            return rand() % size_;
        }

        void update(double /**/, double /**/, double /**/) {}

    private:
        size_t size_;
};

}
