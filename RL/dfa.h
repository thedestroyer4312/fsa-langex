#pragma once

#include <iterator>
#include <memory>
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
        std::vector<std::unique_ptr<Node>> states;
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

        bool evaluate(std::stringview s) const;
        template <typename Iterator,
            typename value_type = std::iterator_traits<Iterator>::value_type,
            std::enable_if_t<std::is_convertible<value_type, char>>::value>
        bool evaluate(Iterator begin, Iterator end) const;
    };
}

// Method implementations
namespace FSA{
    DFA::DFA(){
        config_empty_string();
    }

    template <typename Iterator,
        typename value_type = std::iterator_traits<Iterator>::value_type,
        std::enable_if_t<std::is_convertible<value_type, char>>::value>
    bool DFA::evaluate(Iterator begin, Iterator end) const{
        const Node* curr_node = start_state;
        
        // Trace through DFA
        while(begin != end){
            char c = *begin; 
            curr_node = delta(curr_node, c);
        }

        return curr_node->is_accept;
    }

    bool DFA::evaluate(std::stringview s) const{
        return evaluate(s.cbegin(), s.cend());
    }

    const DFA::Node* DFA::delta(Node* n, char c) const{
        
    }
};
