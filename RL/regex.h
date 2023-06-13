#pragma once

#include <string_view>

#include "dfa.h"
#include "rl_fsa.h"

namespace LangEx{
    
    template <typename FSA_type = FSA::DFA>
    class Regex{
    private:
        FSA::RL_FSA<FSA_type> underlying_FSA;

    public:
        // Constructors

        // Constructs null regex "" (recognizes empty string)
        Regex();
        // Constructs from given pattern string p
        Regex(std::string_view p);

        // Evaluation operators
        bool match(std::string_view str) const;

        // Regular expression construction operators (immutable)

        // Concatenation (n-ary operator)
        template <Regex... exprs>
        Regex concatenate(const Regex& expr, const &exprs...) const;
        Regex concatenate(const Regex& expr) const;

        // Union (n-ary operator)
        template <Regex... exprs>
        Regex union_or(const Regex& expr, const &exprs...) const;
        Regex union_or(const Regex& expr) const;

        // Kleene star (unary operator)
        Regex kleene_star() const;

        // Helper methods

        // Recursively constructs FSA given AST of regular expression
        template <typename output_FSA_type>
        static output_FSA_type construct_underlying_FSA();

    };
};