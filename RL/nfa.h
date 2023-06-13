#pragma once

#include "rl_fsa.h"

namespace FSA{
    class NFA{
    public:
        bool evaluate() const;
    };

    // Explicit template instantiation - compile time interface
    template class RL_FSA<NFA>;
};