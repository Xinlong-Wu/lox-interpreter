#ifndef COMMON_H
#define COMMON_H

#include <cassert>
#include <utility>
#include <functional>
#include <memory>

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
bool isa(const std::shared_ptr<From>& from) {
    return To::classof(from);
}

template <typename To, typename From>
bool isa(const std::unique_ptr<From>& from) {
    return To::classof(from.get());
}

template <typename To, typename From>
To* dyn_cast(From* from) {
    return isa<To>(from) ? static_cast<To*>(from) : nullptr;
}

template <typename To, typename From>
std::shared_ptr<To> dyn_cast(const std::shared_ptr<From>& from) {
    return isa<To>(from) ? std::static_pointer_cast<To>(from) : nullptr;
}

template <typename To, typename From>
std::unique_ptr<To> dyn_cast(const std::unique_ptr<From>& from) {
    return isa<To>(from.get()) ? std::unique_ptr<To>(static_cast<To*>(from.release())) : nullptr;
}

template <typename To, typename From>
To* cast(From* from) {
    assert(isa<To>(from) && "Invalid cast");
    return static_cast<To*>(from);
}

template <typename To, typename From>
std::shared_ptr<To> cast(const std::shared_ptr<From>& from) {
    assert(isa<To>(from) && "Invalid cast");
    return std::static_pointer_cast<To>(from);
}

template <typename To, typename From>
std::unique_ptr<To> cast(std::unique_ptr<From>&& from) {
    assert(isa<To>(from.get()) && "Invalid cast");
    return std::unique_ptr<To>(static_cast<To*>(from.release()));
}

} // namespace lox


#endif // COMMON_H