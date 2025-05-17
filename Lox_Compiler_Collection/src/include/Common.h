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

namespace lox
{

template <typename T>
inline void hash_combine(const T& val, std::size_t& seed) {
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

} // namespace lox


#endif // COMMON_H