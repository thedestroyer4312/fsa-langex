#pragma once

#include <iterator>
#include <type_traits>
#include <utility>

namespace FSA{
    // Container for a regular language FSA (i.e. DFA or NFA)
    template <typename FSA_type, typename Character = char>
    class RL_FSA{
    private:
        FSA_type fsa;

    public:
        // TBD: make notes on using the implicit constructors here

        // Construct RL_FSA from given underlying FSA
        RL_FSA(FSA_type&& fsa_in) : fsa(std::forward<FSA_type>(fsa_in)) {}

        // Perform whole string matching on the FSA
        // Evaluate forward iterators whose value type is Character (or convertible to Character)
        template <typename Iterator,
            typename value_type = std::iterator_traits<Iterator>::value_type,
            std::enable_if_t<std::is_convertible<value_type, Character>>::value>
        bool evaluate(Iterator begin, Iterator end) const;

        // Core interface methods - must be implemented by any valid type
        // All of the following methods are closure properties of regular languages
        // That is, all of these operations on regular languages will, in turn, create regular languages too
        /* Intersection
         * M' = Intersection(M1, M2) will recognize the intersection of the two FSA languages
         * i.e. x in L(M') <-> x in L(M1) AND x in L(M2)
         */
        RL_FSA intersection(const RL_FSA& other) const { return RL_FSA(fsa.intersection(other.fsa)); }

        /* Union
         * M' = Intersection(M1, M2) will recognize the union of the two FSA langauges
         * i.e. x in L(M') <-> x in L(M1) OR x in L(M2)
         */
        RL_FSA union_or(const RL_FSA& other) const { return RL_FSA(fsa.union_or(other.fsa)); }

        /*
         * Kleene star
         * M' = Kleene_star(M) will recognize the Kleene star of L(M)
         * i.e. x in L(M') <-> t.e. nonnegative integer n and w in L(M) s.t. x = w^n
         */
        RL_FSA kleene_star() const { return RL_FSA(fsa.kleene_star()); };

        /* Concatenation
         * M' = Concatenate(M1, M2) will recognize the concatenation of the two FSA languages
         * i.e. x in L(M') <-> t.e. w in L(M1), z in L(M2) s.t. x = wz
         */
        RL_FSA concatenate(const RL_FSA& other) const { return RL_FSA(fsa.concatenate(other.fsa)); }

        /* Complement
         * M' = Complement(M) will recognize the complement of the FSA language
         * i.e. x in L(M') <-> x not in L(M)
         */
        RL_FSA complement() const { return RL_FSA(fsa.complement()); }

        const FSA_type& get_underlying_FSA() const { return fsa; }

        // Extension methods
        // Implemented using core inteface method implementations
        // TBD... (dynamic exponents, sequenced characters, etc.)
    };
}

// Method definitions

