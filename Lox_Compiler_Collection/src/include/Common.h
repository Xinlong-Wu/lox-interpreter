#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <utility>
#include <functional>

#if __cplusplus >= 202302L
    #define unreachable() std::unreachable()
#elif __cplusplus >= 201703L
    // C++17
    #if defined(__GNUC__) || defined(__clang__)
        #define unreachable() __builtin_unreachable()
    #elif defined(_MSC_VER)
        #define unreachable() __assume(0)
    #else
        #error “Unsupported compiler. Please use GCC, Clang, or MSVC.”
    #endif
#else
    #error “C++ version not supported. Please use C++17 or higher.”
#endif

#define assert_not_reached(msg) do { assert(0 && msg); unreachable(); } while(0)

#define TYPEID_SYSTEM(baseClass, className) \
    static bool classof(const baseClass *ptr) { return ptr->getKind() == Kind::className; } \
    static bool classof(const std::shared_ptr<baseClass> ptr) { return className::classof(ptr.get()); } \
    virtual Kind getKind() const override { return Kind::className; }

namespace lox
{

template <typename T>
inline void hash_combine(const T& val, std::size_t& seed) {
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename To, typename From>
bool isa(const From* from) {
    return To::classof(from);
}

template <typename To, typename From>
To* dyn_cast(From* from) {
    return isa<To>(from) ? static_cast<To*>(from) : nullptr;
}

template <typename To, typename From>
To* cast(From* from) {
    assert(isa<To>(from) && "Invalid cast");
    return static_cast<To*>(from);
}

} // namespace lox


#endif // COMMON_H