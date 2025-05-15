#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <utility>

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

#endif // COMMON_H