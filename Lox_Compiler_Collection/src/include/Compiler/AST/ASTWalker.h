#ifndef ASTWALKER_H
#define ASTWALKER_H

#include <typeindex>
#include <unordered_map>
#include <functional>

#include "Common.h"

namespace lox
{

class ASTNode;

// 遍历控制结果
enum class WalkResult {
    Advance,    // 继续遍历子节点（默认行为）
    Skip,       // 跳过当前节点的子节点，但继续遍历兄弟节点
    Interrupt   // 中断整个遍历过程
};

// 遍历顺序
enum class WalkOrder {
    PreOrder,   // 前序遍历：先访问节点，再访问子节点
    PostOrder   // 后序遍历：先访问子节点，再访问节点
};

// Walker回调函数类型定义
template<typename T>
using WalkCallback = std::function<WalkResult(T*)>;

// 无返回值的回调函数类型
template<typename T>
using VoidWalkCallback = std::function<void(T*)>;

// Walker类，用于存储和调用不同类型的回调函数
class Walker {
private:
    // 使用type_info作为key来存储不同类型的回调函数
    std::unordered_map<std::type_index, std::function<WalkResult(ASTNode*)>> callbacks;
    WalkOrder order;

public:
    explicit Walker(WalkOrder order = WalkOrder::PreOrder) : order(order) {}

    // 注册特定类型的回调函数（返回WalkResult）
    template<typename T>
    void registerCallback(WalkCallback<T> callback) {
        callbacks[typeid(T)] = [callback](ASTNode* node) -> WalkResult {
            // if (auto* typed_node = dynamic_cast<T*>(node)) {
            if (auto* typed_node = dyn_cast<T>(node)) {
                return callback(typed_node);
            }
            return WalkResult::Advance;
        };
    }

    // 注册特定类型的回调函数（无返回值）
    template<typename T>
    void registerCallback(VoidWalkCallback<T> callback) {
        callbacks[typeid(T)] = [callback](ASTNode* node) -> WalkResult {
            // if (auto* typed_node = dynamic_cast<T*>(node)) {
            if (auto* typed_node = dyn_cast<T>(node)) {
                callback(typed_node);
            }
            return WalkResult::Advance;
        };
    }

    // 执行特定类型的回调
    WalkResult executeCallback(ASTNode* node);

    // 检查是否有特定类型的回调
    template<typename T>
    bool hasCallback() const {
        return callbacks.find(std::type_index(typeid(T))) != callbacks.end();
    }

    // 获取遍历顺序
    WalkOrder getOrder() const { return order; }

    // 设置遍历顺序
    // void setOrder(WalkOrder order) { order_ = order; }
};

} // namespace lox


#endif // ASTWALKER_H