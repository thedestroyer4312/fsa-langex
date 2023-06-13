#include <algorithm>
#include <stdexcept>

#include "dfa.h"

namespace FSA{
    // Since intersection & union constructions are identical except for accepting states,
    // put into helper function
    DFA cross_product_construction(const DFA& M1, const DFA& M2, bool intersection_construction){
        using Node = DFA::Node;

        // Given DFA M1 = (Q1, delta1, q0_1, F1), M2 = (Q2, delta2, q0_2, F2)
        // (Alphabet Sigma assumed to be same, omitted for brevity)
        // Construct new DFA M' = (Q', delta', q0', F') as follows:
        // Q' = Q1 x Q2
        // delta'((qa, qb), c) = (delta1(qa, c), delta2(qb, c))
        // q0' = (q0_1, q0_2)
        // F' = {(qa, qb) | qa in F1 OP qb in F2}
        // where OP = AND if intersection, OR if union

        DFA M_prime;

        // First, construct Q'. If |Q1| = S1, |Q2| = S2, then |Q'| = S1 * S2
        size_t S1 = M1.states.size();
        size_t S2 = M2.states.size();
        M_prime.states.reserve(S1 * S2);
        for(size_t i = 0; i < S1 * S2; i++){
            M_prime.states.push_back(std::make_shared<Node>());
        }

        // Now, construct delta'
        // Reconstruct each delta function mapping pointers to their index location in `states`
        // Basically, replace pointers in the maps with indexes
        // i.e. if in map ptr p -> ptr q, change to map int i -> int j where p = states[i], q = states[j]

        // To make this easier, first map all the pointers in the transition table to indices
        // Then, cycle through the transition table and replace those references with indices
        // Then, we can make the combined transition table delta'
        

        // Construct q0'
        // Compute the indices for q0_1, q0_2
        // i.e. i1, i2 s.t. q0_1 = states[i1], q0_2 = states[i2]
        auto i1_it = std::find(M1.states.cbegin(), M1.states.cend(), M1.start_state);
        auto i2_it = std::find(M2.states.cbegin(), M2.states.cend(), M2.start_state);

        size_t i1 =  i1_it - M1.states.cbegin();
        size_t i2 = i2_it - M2.states.cbegin();
        
        // Compute and assign flattened index i'. (i1, i2) -> i' = (i1 * S2) + i2
        size_t i_prime = (i1 * S2) + i2;
        M_prime.start_state = M_prime.states[i_prime].get();

        // Finally, compute F'
        // First, gather the indices for the states which accept
        // Then, use those to construct F'. AND for intersection, OR for union
        std::vector<size_t> M1_accept_indices, M2_accept_indices;
        M1_accept_indices.reserve(S1);
        M2_accept_indices.reserve(S2);
        
        // Linear search through `states` and add all indices for accepting states
        for(size_t i = 0; i < S1; i++){
            if(M1.states[i]->is_accept){
                M1_accept_indices.push_back(i);
            }
        }
        for(size_t i = 0; i < S2; i++){
            if(M2.states[i]->is_accept){
                M2_accept_indices.push_back(i);
            }
        }

        // Now, add accepting states to M'
        if(intersection_construction){
            // Add (qa, qb) for qa in F1 AND qb in F2
            for(size_t i_qa : M1_accept_indices){
                for(size_t i_qb : M2_accept_indices){
                    // Compute flattened index: (i1, i2) -> i' = (i1 * S2) + i2
                    size_t i_prime = (i_qa * S2) + i_qb;
                    M_prime.states[i_prime]->is_accept = true;
                }
            }
        }else{
            // Union
            // Add (qa, qb) for qa in F2 OR qb in F2
            // For each qa, qb:
            // 1. (qa, j) for 0 <= j < S2
            // 2. (i, qb) for 0 <= i < S1

            // Requires 2 nested for loops, 1 for each direction
            // Compute flattened index: (i1, i2) -> i' = (i1 * S2) + i2
            for(size_t i_qa : M1_accept_indices){
                for(size_t j = 0; j < S2; j++){
                    // (qa, j)
                    size_t i_prime = (i_qa * S2) + j;
                    M_prime.states[i_prime]->is_accept = true;
                }
            }
            for(size_t i_qb : M2_accept_indices){
                for(size_t i = 0; i < S1; i++){
                    // (i, qb)
                    size_t i_prime = (i * S2) + i_qb;
                    M_prime.states[i_prime]->is_accept = true;
                }
            }
        }

        return M_prime;
    }

    void DFA::clear(){
        states.clear();
        transitions.clear();
        start_state = nullptr;
    }

    DFA DFA::empty_string(){
        DFA out;

        // We do this by creating one accept state with no transitions
        auto state = std::make_shared<Node>(true);
        
        // Add to states and set as start state
        out.start_state = state.get();
        out.states.push_back(state);

        return out;
    }
    
    bool DFA::evaluate(std::string_view s) const{
        return evaluate(s.cbegin(), s.cend());
    }

    DFA DFA::intersection(const DFA& other) const{
        // Given DFA M1 = (Q1, delta1, q0_1, F1), M2 = (Q2, delta2, q0_2, F2)
        // (Alphabet Sigma assumed to be same, omitted for brevity)
        // Construct new DFA M' = (Q', delta', q0', F') as follows:
        // Q' = Q1 x Q2
        // delta'((qa, qb), c) = (delta1(qa, c), delta2(qb, c))
        // q0' = (q0_1, q0_2)
        // F' = {(qa, qb) | qa in F1 AND qb in F2}
        return cross_product_construction(*this, other, true);
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