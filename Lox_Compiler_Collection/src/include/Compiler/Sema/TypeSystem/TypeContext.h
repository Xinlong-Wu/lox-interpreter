#ifndef TYPECONTEXT_H
#define TYPECONTEXT_H

#include "Compiler/Sema/TypeSystem/Type.h"

#include <vector>
#include <memory>

namespace lox
{
    class TypeContext
    {
        private:
            PrimitiveType *numberType = nullptr;
            PrimitiveType *stringType = nullptr;
            PrimitiveType *boolType = nullptr;
            NilType *nilType = nullptr;

            // 存储所有类型的映射
            std::vector<std::unique_ptr<Type>> storage;
        public:
            TypeContext()
            {
                // 初始化基本类型
                std::unique_ptr<PrimitiveType> number = std::unique_ptr<PrimitiveType>(new PrimitiveType("Number"));
                std::unique_ptr<PrimitiveType> string = std::unique_ptr<PrimitiveType>(new PrimitiveType("String"));
                std::unique_ptr<PrimitiveType> boolean = std::unique_ptr<PrimitiveType>(new PrimitiveType("Bool"));
                std::unique_ptr<NilType> nil = std::unique_ptr<NilType>(new NilType());
                numberType = number.get();
                stringType = string.get();
                boolType = boolean.get();
                nilType = nil.get();
                // 将基本类型添加到storage
                storage.push_back(std::move(number));
                storage.push_back(std::move(string));
                storage.push_back(std::move(boolean));
                storage.push_back(std::move(nil));
            }
            ~TypeContext() = default;

            TypeContext(const TypeContext&) = delete;
            TypeContext& operator=(const TypeContext&) = delete;
            TypeContext(TypeContext&&) = delete;
            TypeContext& operator=(TypeContext&&) = delete;

            // 获取基本类型
            PrimitiveType* getNumberType() const { return numberType; }
            PrimitiveType* getStringType() const { return stringType; }
            PrimitiveType* getBoolType() const { return boolType; }
            NilType* getNilType() const { return nilType; }

            // 添加类型到arena
            template<typename T, typename... Args>
            T* make(Args&&... args) {
                // donot allow create Type directly
                static_assert(!std::is_same<T, Type>::value, "Cannot create Type directly");
                // donot allow create PrimitiveType directly
                static_assert(!std::is_same<T, PrimitiveType>::value, "Cannot create PrimitiveType directly");
                // donot allow create NilType directly
                static_assert(!std::is_same<T, NilType>::value, "Cannot create NilType directly");
                // T must be a subclass of Type
                static_assert(std::is_base_of<Type, T>::value, "T must be a subclass of Type");
                auto type = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
                T* ptr = type.get();
                storage.push_back(std::move(type));
                return ptr;
            }
    };
} // namespace lox


#endif // TYPECONTEXT_H