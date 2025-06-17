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

#define TYPEID_SYSTEM_N(baseClass, thisClass, ...)                                   \
    static bool classof(const baseClass* ptr) {                                      \
        return isOneOf(ptr->getKind(), Kind::thisClass, __VA_ARGS__);                \
    }                                                                                \
    static bool classof(const std::shared_ptr<baseClass>& ptr) {                     \
        return classof(ptr.get());                                                   \
    }                                                                                \
    virtual Kind getKind() const override {                                          \
        return Kind::thisClass;                                                      \
    }

namespace lox
{

template <typename Enum, typename... Enums>
bool isOneOf(Enum value, Enum first, Enums... rest) {
    return ((value == first) || ... || (value == rest));
}

template <typename T>
inline void hash_combine(const T& val, std::size_t& seed) {
    seed ^= std::hash<T>()(val) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename To, typename... Rest, typename From>
bool isa_raw(const From* from) {
    if (!from) return false;

    if constexpr (sizeof...(Rest) == 0) {
        return To::classof(from);
    } else {
        return To::classof(from) || isa_raw<Rest...>(from);
    }
}

template <typename To, typename... Rest, typename From>
bool isa(const From* from) {
    return isa_raw<To, Rest...>(from);
}

template <typename To, typename... Rest, typename From>
bool isa(const std::shared_ptr<From>& from) {
    return isa_raw<To, Rest...>(from.get());
}

template <typename To, typename... Rest, typename From>
bool isa(const std::unique_ptr<From>& from) {
    return isa_raw<To, Rest...>(from.get());
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
std::unique_ptr<To> dyn_cast(std::unique_ptr<From>& from) {
    if (!isa<To>(from.get())) {
        return nullptr;
    }
    // 转移所有权
    To* raw_ptr = static_cast<To*>(from.release());
    return std::unique_ptr<To>(raw_ptr);
}

template <typename To, typename From>
To* cast(From* from) {
    assert(isa<To>(from) && "Invalid cast");
    return static_cast<To*>(from);
}

template <typename To, typename From>
const To* cast(const From* from) {
    return static_cast<const To*>(from);
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