#pragma once

#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "rl_fsa.h"

namespace FSA{
    /* Deterministic Finite Automaton
     * Immutable DFA class built with recursive construction in mind
     * Implements the RL_FSA interface (see rl_fsa.h)
     */
    class DFA{
    public:
        using node_id_t = size_t;

        // node -> char -> node
        using transition_tb_t = std::unordered_map<
            node_id_t,
            std::unordered_map<char, node_id_t>
        >;

    private:
        // A DFA = (Q, Sigma, delta, q0, F)
        // Q - set of states. Described by `num_states`
        // Sigma - alphabet. Implicitly `char`
        // delta - Transition function. delta: Q x Sigma -> Q
        // If the state or character input is not found, treat as automatic reject
        // q0 - Starting state. q0 in Q
        // F - Subset of Q, states that are accepting. Encoded in `accept_states`
        size_t num_states;
        transition_tb_t transitions;
        std::vector<bool> accept_states;
        node_id_t start_state;

        /* Delta function helper
         * If the input state and character combination is found, returns next state
         * Otherwise, returns nullptr (i.e. reject the state)
         */
        std::optional<node_id_t> delta(node_id_t id, char c) const;

    public:
        // Constructors & destructor, assignments - use defaults
        // Note: a default-constructed DFA accepts no strings i.e. the language is empty set
        DFA() = default;
        DFA(const DFA&) = default;
        DFA(DFA&&) = default;
        ~DFA() = default;
        DFA& operator=(const DFA&) = default;
        DFA& operator=(DFA&&) = default;

        bool evaluate(std::string_view s) const;
        template <typename Iterator>
        bool evaluate(Iterator begin, Iterator end) const;

        // Core interface methods for RL_FSA
        // For detailed descriptions, see RL_FSA class definition (rl_fsa.h)
        // Union, intersection, concatenation, complement, kleene star
        DFA intersection(const DFA& other) const;
        DFA union_or(const DFA& other) const;
        DFA kleene_star() const;
        DFA concatenate(const DFA& other) const;
        DFA complement() const;

        /*
         * Optional in terms of interface, but doubtless to be highly useful
         * Applies Myphill-Nerode theorem to minimize DFA states which reduce
         * memory usage and execution time
         */
        DFA minimize_states() const;

        // Blanks the DFA so that its language is empty set once more
        void clear();
        
        // Returns DFA whose language is {""} i.e. only accepts empty string
        // This is not the same as default-constructed DFA because that also rejects empty string
        static DFA empty_string();

        // Other declarations
        friend DFA cross_product_construction(const DFA&, const DFA&, bool);
    };
};

// Template method implementations (others are in .cpp)
namespace FSA{
    template <typename Iterator>
    bool DFA::evaluate(Iterator begin, Iterator end) const{
        bool result = false;

        // Empty DFA accepts no strings
        if(num_states != 0){
            node_id_t curr_node = start_state;

            // Step through DFA
            // If string is finished or curr_node is invalid, break)
            while(begin != end){
                char c = *begin;
                std::optional<node_id_t> next_node = delta(curr_node, c);

                if(next_node.has_value()){
                    curr_node = *next_node;
                }else{
                    // delta() returns no transition -> automatic reject
                    // by our convention (like NFA)
                    break;
                }

                ++begin;
            }

            // Accept conditions:
            // 1. string must be fully evaluated (begin = end)
            // 2. must have ended on an accepting state
            result = (begin == end) && accept_states[curr_node];
        }
        return result;
    }
};
