#include "Compiler/AST/Type.h"
#include "Compiler/Sema/Scope.h"

#include <queue>

using namespace std;
using namespace lox;

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


const lox::FunctionType::Signature *lox::FunctionType::resolveOverload(const std::vector<Type*> &argTypes) const {
    priority_queue<pair<int64_t, const FunctionType::Signature*>> candidates;

    for (const auto &overload : overloads) {
        if (overload->parameters.size() != argTypes.size()) {
            continue; // Skip if parameter count doesn't match
        }
        int64_t score = calculateMatchScore(overload->parameters, argTypes);
        if (score >= 0) {
            candidates.push(make_pair(score, overload));
        }
    }

    if (candidates.empty()) {
        return nullptr; // No matching overload found
    }

    return candidates.top().second;
}

lox::Type* lox::ClassType::getPropertyType(const std::string &propertyName) const {
    if (properties) {
      auto prop = properties->lookupLocal(propertyName);
      if (prop) {
        return prop->getType();
      }
    }
    return nullptr;
}

const std::vector<lox::Type*> lox::ClassType::getPropertyTypes() const {
    std::vector<lox::Type*> types;
    if (properties) {
      for (const auto &symbol : properties->getSymbols()) {
        types.push_back(symbol->getType());
      }
    }
    return types;
  }