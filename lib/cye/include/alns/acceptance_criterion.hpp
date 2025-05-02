#pragma once

namespace alns {

class AcceptanceCriterion {
    public:
    virtual bool accept(double current, double previous, double best) = 0;
};

class HillClimbingCriterion : public AcceptanceCriterion {
    public:
    bool accept(double current, double previous, double /**/) override {
        return current < previous;
    }
};

}