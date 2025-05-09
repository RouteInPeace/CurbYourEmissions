#pragma once

namespace alns {


class HillClimbingCriterion {
    public:
    bool accept(double current, double previous, double /**/) {
        return current < previous;
    }
};

}