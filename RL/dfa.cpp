#include <algorithm>
#include <functional>
#include <iterator>
#include <stdexcept>
#include <set>

#include "dfa.h"

namespace FSA{
    // Static helper functions and the like
    namespace{
        // Replicating typedefs from DFA for use in static functions, etc.
        using node_id = DFA::node_id_t;
        using transition_tb_t = DFA::transition_tb_t;
        // `transition_tb_entry_t` is the type of the entries in `transition_tb_t`
        using transition_tb_entry_t = DFA::transition_tb_t::value_type; // std::pair<node_id, std::unordered_map<char, node_id>>;
        // `state_transition_tb_t` is the table mapping char -> node for a given state q
        using state_transition_tb_t = DFA::transition_tb_t::mapped_type; // std::unordered_map<char, node_id>;
        using state_transition_tb_entry_t = state_transition_tb_t::value_type; // std::pair<node_id, char>;

        // Note: retrieves index in sorted ascending order
        std::vector<DFA::node_id_t> retrieve_accepting_indices(const std::vector<bool>& F){
            std::vector<DFA::node_id_t> result;
            result.reserve(F.size());

            for(size_t i = 0; i < F.size(); i++){
                if(F[i]){
                    result.push_back(i);
                }
            }

            return result;
        }

        /* Retrieves the characters common to two states q_a, q_b
         * Specifically, generates set C = {c in Sigma | delta1(q_a, c) is defined and delta2(q_b, c) is defined}
         * Given the transition tables for states q_a, q_b
         */
        template <typename K, typename V>
        std::vector<K> keys_intersection(const std::unordered_map<K, V>& tb_a, const std::unordered_map<K, V>& tb_b){
            std::vector<K> out;
            out.reserve(std::max(tb_a.size(), tb_b.size()));

            // Loop over both tables and add all the characters we see
            // Use std::set because std::intersection requires sorted values
            std::set<K> domain_a, domain_b;

            std::transform(tb_a.cbegin(), tb_a.cend(), std::inserter(domain_a, domain_a.begin()),
                [](const std::pair<K, V>& p) -> K { return p.first; });
            std::transform(tb_b.cbegin(), tb_b.cend(), std::inserter(domain_b, domain_b.begin()),
                [](const std::pair<K, V>& p) -> K { return p.first; });

            // Now, simply perform set intersection between them
            std::set_intersection(
                domain_a.cbegin(),
                domain_a.cend(),
                domain_b.cbegin(),
                domain_b.cend(),
                std::back_inserter(out)
            );

            return out;
        }

        // Used in cross_product_construction()
        transition_tb_t construct_delta_intersection(
            const transition_tb_t& tb_a,
            const transition_tb_t& tb_b,
            std::function<node_id(node_id, node_id)> flatten_indices)
        {
            transition_tb_t out;

            // Add only the shared transitions i.e. delta'((q_a, q_b), c) = (delta1(q_a, c), delta2(q_b, c))
            // where both delta1(q_a, c) and delta2(q_b, c) are valid

            // For each (q_a, q_b) in Q'
            for(const transition_tb_entry_t& qa_transition_tb_entry : tb_a){
                for(const transition_tb_entry_t& qb_transition_tb_entry : tb_b){
                    node_id q_a = qa_transition_tb_entry.first;
                    node_id q_b = qb_transition_tb_entry.first;

                    // Retrieve the state transition tables for q_a, q_b
                    const state_transition_tb_t& qa_state_transition_tb = qa_transition_tb_entry.second;
                    const state_transition_tb_t& qb_state_transition_tb = qb_transition_tb_entry.second;

                    // By definition,
                    // delta'((q_a, q_b), c) = (delta1(q_a, c), delta2(q_b, c))
                    // However, because we only add valid transitions, this means that both
                    // delta1(q_a, c) and delta2(q_b, c) must be valid
                    // We extract valid q_a, q_b so that leaves c.
                    // c must be in the domain of both delta functions else get reject state -> not both valid.
                    // So, we only need to compute delta'((q_a, q_b), c) where c is defined in both functions
                    // i.e. c defined in the intersection of the domains
                    std::vector<char> shared_characters = keys_intersection(
                        qa_state_transition_tb,
                        qb_state_transition_tb
                    );

                    // Now, iterate over the shared characters
                    node_id q_prime = flatten_indices(q_a, q_b);
                    for(char c : shared_characters){
                        // Compute delta1(q_a, c) and delta2(q_b, c)
                        node_id qa_result = qa_state_transition_tb.at(c);
                        node_id qb_result = qb_state_transition_tb.at(c);

                        node_id q_prime_result = flatten_indices(qa_result, qb_result);

                        // delta'((q_a, q_b), c) = (delta1(q_a, c), delta2(q_b, c))
                        out[q_prime][c] = q_prime_result;
                    }
                }
            }

            return out;
        }

        transition_tb_t construct_delta_union(
            const transition_tb_t& tb_a,
            const transition_tb_t& tb_b,
            std::function<node_id(node_id, node_id)> flatten_indices,
            size_t SA,
            size_t SB
        )
        {   
            // Use the delta intersection function for both valid transitions so we only have to do non-valids
            transition_tb_t out = construct_delta_intersection(tb_a, tb_b, flatten_indices);

            // Double nested loop over each transition table
            // i.e. one for all valid delta1() with any delta2() and one for valid delta2() with any delta1()

            // For each q_a in Q_a (valid, that is)
            for(const transition_tb_entry_t& qa_transition_tb_entry : tb_a){
                node_id q_a = qa_transition_tb_entry.first;
                const state_transition_tb_t& qa_state_transition_tb = qa_transition_tb_entry.second;
                
                // Loop over characters c in delta1(q_a, c)
                for(const state_transition_tb_entry_t& qa_state_transition_tb_entry : qa_state_transition_tb){
                    char c = qa_state_transition_tb_entry.first;
                    node_id qa_result = qa_state_transition_tb_entry.second;

                    // For any q_b in Q_b (only handle invalid, valid already handled)
                    for(size_t j = 0; j < SB; j++){
                        // Valid means delta1(q_a, c) and delta2(q_b, c) are both defined
                        // So, q_b and c must be in the domain of the delta2 function
                        node_id q_b = j;

                        if(tb_b.count(q_b) && tb_b.at(q_b).count(c)){
                            continue;
                        }

                        // Now we know for sure that delta2(q_b, c) is undefined here
                        // delta'((q_a, q_b), c) = (delta1(q_a, c), delta2(q_b, c))
                        
                    }
                }
            }

            // For each q_b in Q_b (valid, that is)
            for(const transition_tb_entry_t& qb_transition_tb_entry : tb_b){
                node_id q_b = qb_transition_tb_entry.first;
                const state_transition_tb_t& qb_state_transition_tb = qb_transition_tb_entry.second;

                // For any q_a in Q_b (valid or not)
            }

            return out;
        }
    };

    // Since intersection & union constructions are identical except for accepting states,
    // put into helper function
    DFA cross_product_construction(const DFA& M1, const DFA& M2, bool intersection_construction){
        // Given DFA M1 = (Q1, delta1, q0_1, F1), M2 = (Q2, delta2, q0_2, F2)
        // (Alphabet Sigma assumed to be same, omitted for brevity)
        // Construct new DFA M' = (Q', delta', q0', F') as follows:
        // Q' = Q1 x Q2
        // delta'((q_a, q_b), c) = (delta1(q_a, c), delta2(q_b, c))
        // q0' = (q0_1, q0_2)
        // F' = {(q_a, q_b) | q_a in F1 OP q_b in F2}
        // where OP = AND if intersection, OR if union

        DFA M_prime;

        // First, construct Q'. If |Q1| = S1, |Q2| = S2, then |Q'| = S1 * S2
        size_t S1 = M1.num_states;
        size_t S2 = M2.num_states;
        M_prime.num_states = S1 * S2;

        // Helper function to compute flattened indices:
        // (a, b) := (a * S2) + b
        std::function<node_id(node_id, node_id)> flatten_indices(
            [=](node_id a, node_id b) constexpr -> node_id {
                return (a * S2) + b;
        });

        // Now, construct delta'
        // Nested-for iterate over both tables
        // In intersection, if one state is invalid (i.e. nonexistent transition), automatic reject (no transition needed)
        // However, in union, if one state is invalid, the other should proceed still. Only if both are invalid then does it reject.
        // As such, union requires extra transitions to be added for all states (q_a, q_b) where one of q_a or q_b is not invalid

        if(intersection_construction){
            M_prime.transitions = construct_delta_intersection(M1.transitions, M2.transitions, flatten_indices);
        }else{
            M_prime.transitions = construct_delta_union(M1.transitions, M2.transitions, flatten_indices, S1, S2);
        }

        // Construct q0' = (q0_1, q0_2)
        node_id q0_1 = M1.start_state;
        node_id q0_2 = M2.start_state;

        // Compute and assign flattened index i'. (i1, i2) -> i' = (i1 * S2) + i2    
        node_id i_prime = flatten_indices(q0_1, q0_2);
        M_prime.start_state = i_prime;

        // Finally, compute F' using the indices of accepting states
        // AND for intersection, OR for union
        std::vector<node_id> M1_accept_indices = retrieve_accepting_indices(M1.accept_states);
        std::vector<node_id> M2_accept_indices = retrieve_accepting_indices(M2.accept_states);

        // Add accepting states to M'
        if(intersection_construction){
            // Intersection
            // Add (q_a, q_b) for q_a in F1 AND q_b in F2
            for(node_id i_qa : M1_accept_indices){
                for(node_id i_qb : M2_accept_indices){
                    // Compute flattened index: (i1, i2) -> i' = (i1 * S2) + i2
                    node_id i_prime = flatten_indices(i_qa, i_qb);
                    M_prime.accept_states[i_prime] = true;
                }
            }
        }else{
            // Union
            // Add (q_a, q_b) for q_a in F2 OR q_b in F2
            // For each q_a, q_b:
            // 1. (q_a, j) for 0 <= j < S2
            // 2. (i, q_b) for 0 <= i < S1

            // Requires 2 nested for loops, 1 for each direction
            // Compute flattened index: (i1, i2) -> i' = (i1 * S2) + i2
            for(size_t i_qa : M1_accept_indices){
                for(size_t j = 0; j < S2; j++){
                    // (q_a, j)
                    node_id i_prime = flatten_indices(i_qa, j);
                    M_prime.accept_states[i_prime] = true;
                }
            }
            for(size_t i_qb : M2_accept_indices){
                for(size_t i = 0; i < S1; i++){
                    // (i, q_b)
                    node_id i_prime = flatten_indices(i, i_qb);
                    M_prime.accept_states[i_prime] = true;
                }
            }
        }

        return M_prime;
    }

    void DFA::clear(){
        num_states = 0;
        transitions.clear();
        accept_states.clear();
    }

    DFA DFA::empty_string(){
        DFA out;

        // We do this by creating one accept state with no transitions
        out.num_states = 1;
        out.accept_states.push_back(true);
        out.start_state = 0;

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
        // delta'((q_a, q_b), c) = (delta1(q_a, c), delta2(q_b, c))
        // q0' = (q0_1, q0_2)
        // F' = {(q_a, q_b) | q_a in F1 AND q_b in F2}
        return cross_product_construction(*this, other, true);
    }

    DFA DFA::complement() const{
        // Simply flip the accepting and reject states
        DFA comp = *this;
        comp.accept_states.flip();
        return comp;
    }

    std::optional<DFA::node_id_t> DFA::delta(node_id_t id, char c) const{
        // If Node n and character c are both present, return next state
        // Otherwise, return null
        
        std::optional<node_id_t> result;
        
        // First, check to see if state n has a transition table
        if(transitions.count(id)){
            const state_transition_tb_t& n_transition_tb = transitions.at(id);
            
            // Now, check to see if state transition table has entry for character c
            if(n_transition_tb.count(c)){
                result = n_transition_tb.at(c);
            }
        }

        return result;
    }
};