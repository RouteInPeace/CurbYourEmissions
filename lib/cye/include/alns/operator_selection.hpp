#pragma once

#include <cstddef>
#include <cstdlib>

namespace alns {

class OperatorSelection {
    public:
        virtual size_t select_operator() = 0;
        virtual void update(double new_cost, double old_cost, double best_cost) = 0;
};

class RandomOperatorSelection : public OperatorSelection {
    public:
        RandomOperatorSelection(size_t size) : size_(size) {}

        size_t select_operator() override {
            return rand() % size_;
        }

        void update(double /**/, double /**/, double /**/) override {}

    private:
        size_t size_;
};

}