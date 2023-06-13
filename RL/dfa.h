#pragma once

#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "rl_fsa.h"

namespace FSA{
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
        
        // Helper methods
        /* Delta function helper
         * If the input state and character combination is found, returns next state
         * Otherwise, returns nullptr (i.e. reject the state)
         */
        const Node* delta(const Node* n, char c) const;
        void config_empty_string();

    public:
        // Empty DFA recognizes empty string
        DFA();

        bool evaluate(std::string_view s) const;

        template <typename Iterator,
            typename value_type = std::iterator_traits<Iterator>::value_type,
            std::enable_if_t<std::is_convertible<value_type, char>::value>>
        bool evaluate(Iterator begin, Iterator end) const;

        // Core interface methods for RL_FSA
        // For detailed descriptions, see RL_FSA class definition (rl_fsa.h)
        // Union, intersection, concatenation, complement, kleene star
        
    };
};

// Method implementations
namespace FSA{
    DFA::DFA(){
        config_empty_string();
    }

    void DFA::config_empty_string(){
        // Initializes a NEW DFA to only accept the empty string
        // We do this by creating one accept state with no transitions
        auto state = std::make_shared<Node>(true);
        
        // Add to states and set as start state
        start_state = state.get();
        states.push_back(state);
    }

    template <typename Iterator,
        typename value_type,
        std::enable_if_t<std::is_convertible<value_type, char>::value>>
    bool DFA::evaluate(Iterator begin, Iterator end) const{
        const Node* curr_node = start_state;
        
        // Trace through DFA
        // If string is finished or curr_node is null, break
        while(begin != end && curr_node){
            char c = *begin; 
            curr_node = delta(curr_node, c);
            ++begin;
        }

        return curr_node && curr_node->is_accept;
    }

    bool DFA::evaluate(std::string_view s) const{
        // evaluate(s.cbegin(), s.cend());
        return true;
    }

    const DFA::Node* DFA::delta(const Node* n, char c) const{
        // If Node n and character c are both present, return next state
        // Otherwise, return nullptr
        
        // Pain in the butt, but we have to do this because const method
        // cannot use operator[], do manual iterator searching
        const Node* result = nullptr;
        
        // First, check to see if state n has a transition table
        auto state_transitions_it = transitions.find(n);
        if(state_transitions_it != transitions.cend()){
            // Retrieve transition table
            const auto& state_transitions = state_transitions_it->second;

            // Now, check to see if the transition table for n has an entry for character c
            auto next_state_it = state_transitions.find(c);
            if(next_state_it != state_transitions.cend()){
                // So, there is an appropriate next state. Return it
                result = next_state_it->second;
            }
        }

        return result;
    }
};
