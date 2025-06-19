#include "Compiler/AST/Type.h"

using namespace std;

int64_t calculateMatchScore(const vector<Type*> &params, const vector<Type*> &args) {
    int64_t score = 0;
    for (size_t i = 0; i < params.size(); ++i) {
        if (i >= args.size()) {
            return -1; // Too few arguments
        }
        if (*params[i] == *args[i]) {
            score += 10; // Exact match
        } else if (args[i]->isCompatibleWith(params[i])) {
            score += 5; // Compatible types
        } else {
            return -1; // Incompatible types
        }
    }
    return score;
}


lox::FunctionType::Signature *lox::FunctionType::Signature::resolveOverload(const std::vector<Type*> &argTypes) const {
    priority_queue<pair<int64_t, FunctionType::Signature*>> candidates;

    for (const auto &overload : overloads) {
        if (overload->parameters.size() != argTypes.size()) {
            continue; // Skip if parameter count doesn't match
        }
        int64_t score = calculateMatchScore(overload->parameters, argTypes);
        if (score >= 0) {
            candidates.push(overload);
        }
    }

    if (candidates.empty()) {
        return nullptr; // No matching overload found
    }

    return candidates.top().second;
}