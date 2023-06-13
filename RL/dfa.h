#pragma once

#include <memory>
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
    private:
        struct Node{
            bool is_accept;
            Node(bool is_accept_in = false) : is_accept(is_accept_in) {}
        };

    private:
        // A DFA = (Q, Sigma, delta, q0, F)
        // Q - set of states. Encoded in `states`
        // Sigma - alphabet. Implicitly `char`
        // delta - Transition function. delta: Q x Sigma -> Q
        // If the state or character input is not found, treat as automatic reject
        // q0 - Starting state. q0 in Q
        // F - Subset of Q, tates that are accepting. Encoded in each node

        // Note: we choose shared_ptr over unique_ptr because of copy constructors/copy assignment
        // In that case, no copy of pointers is needed - simply reference already existing ones
        // This works because transition functions map a subset (or equal to) the `states`
        // that, with shared_ptr, are guaranteed to exist whereas unique_ptr requires manual deep copying
        // Thus, it reduces coding complexity and memory usage
        std::vector<std::shared_ptr<Node>> states;
        std::unordered_map<const Node*, std::unordered_map<char, const Node*>> transitions;
        const Node* start_state;
        
        /* Delta function helper
         * If the input state and character combination is found, returns next state
         * Otherwise, returns nullptr (i.e. reject the state)
         */
        const Node* delta(const Node* n, char c) const;

    public:
        // Constructors & destructor, assignments - use defaults
        // Note: a default-constructed DFA accepts no strings i.e. the language is empty set
        DFA() : start_state(nullptr) {}
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
        const Node* curr_node = start_state;
        
        // Step through DFA
        // If string is finished or curr_node is null, break
        while(begin != end && curr_node){
            char c = *begin; 
            curr_node = delta(curr_node, c);
            ++begin;
        }

        return curr_node && curr_node->is_accept;
    }
};
