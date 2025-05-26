#ifndef EXPR_H
#define EXPR_H

#include "Common.h"
#include "Compiler/AST/Type.h"
#include "Compiler/AST/ASTNode.h"
#include "Compiler/Scanner/Token.h"
#include "Compiler/Sema/Symbol.h"

#include <iostream>
#include <vector>
#include <memory>

namespace lox
{
    class ExprBase : public ASTNode {
    protected:
        enum class Kind {
            ThisExpr,
            SuperExpr,
            GroupingExpr,
            CallExpr,
            VariableExpr,
            LiteralExpr,
            NumberExpr,
            StringExpr,
            UnaryExpr,
            AccessExpr,
            BinaryExpr,
            AssignExpr
        };

        std::shared_ptr<Type> type = nullptr;

    public:
        ExprBase(Location location) : loc(location) {};
        virtual ~ExprBase() = default;

        virtual const Location &getLoc() const { return loc; };
        virtual bool isValidLValue() const { return false; }
        virtual bool isCallable() const { return false; }
        virtual void print(std::ostream &os) const = 0;
        virtual void dump() const {
            this->print(std::cout);
            std::cout << std::endl;
        }

        virtual const std::shared_ptr<Type>& getType() const {
            return type;
        }
        virtual void setType(std::shared_ptr<Type> t) {
            type = std::move(t);
        }

        // bool isNumericType() const;
        // bool isPointerType() const;

        virtual Kind getKind() const = 0;
        virtual void accept(ASTVisitor& visitor) = 0;

    protected:
        Location loc;
    };

    class ThisExpr : public ExprBase {
    public:
        ThisExpr(lox::Token token) : ExprBase(token.getLoction()) {}


        virtual bool isValidLValue() const override { return true; }
        virtual bool isCallable() const override { return true; }
        void print(std::ostream &os) const override {
            os << "this";
        }

        TYPEID_SYSTEM(ExprBase, ThisExpr);
        ACCEPT_DECL();
    };

    class SuperExpr : public ExprBase {
    protected:
        std::string method;
    public:
        SuperExpr(lox::Token token) : ExprBase(token.getLoction()) {}

        virtual bool isValidLValue() const override { return true; }
        virtual bool isCallable() const override { return true; }
        void print(std::ostream &os) const override {
            os << "super";
        }

        TYPEID_SYSTEM(ExprBase, SuperExpr);
        ACCEPT_DECL();
    };

    class GroupingExpr : public ExprBase {
    protected:
        std::unique_ptr<ExprBase> expression;
    public:
        GroupingExpr(std::unique_ptr<ExprBase> expression) : ExprBase(expression->getLoc()), expression(std::move(expression)) {}

        ExprBase* getExpression() const { return expression.get(); }

        virtual bool isValidLValue() const override { return expression->isValidLValue(); }
        virtual bool isCallable() const override { return expression->isCallable(); }
        void print(std::ostream &os) const override {
            os << "( ";
            expression->print(os);
            os << " )";
        }

        TYPEID_SYSTEM(ExprBase, GroupingExpr);
        ACCEPT_DECL();
    };

    class VariableExpr : public ExprBase {
    private:
        std::string name;
        Symbol* symbol = nullptr;
    public:
        VariableExpr(std::string name, Location location) : ExprBase(location), name(std::move(name)){}
        VariableExpr(lox::Token token) : VariableExpr(std::string(token.getTokenString()), token.getLoction()) {}

        virtual const std::string& getName() const { return name; }
        virtual const std::shared_ptr<Type>& getType() const override {
            if (symbol != nullptr) {
                return symbol->getType();
            }
            return this->type;
        }
        virtual Symbol* getSymbol() const {
            return symbol;
        }
        virtual void setSymbol(Symbol* sym) {
            assert(symbol == nullptr && "Symbol has already been set");
            this->symbol = sym;
        }
        virtual void setType(std::shared_ptr<Type> t) override {
            if (symbol) {
                symbol->setType(t);
            }
            this->type = std::move(t);
        }
        virtual bool isValidLValue() const override { return true; }
        // depending on the return type of the variable, we don't know if the variable is callable or not.
        // So we return true here.
        virtual bool isCallable() const override { return true; }

        void print(std::ostream &os) const override {
            os << "Variable: [";
            if (symbol) {
                symbol->print(os);
            }
            else {
                os << name;
                if (type) {
                    os << " : ";
                    type->print(os);
                }
            }
            os << "]";
        }

        TYPEID_SYSTEM(ExprBase, VariableExpr);
        ACCEPT_DECL();
    };

    class CallExpr : public ExprBase {
    protected:
        std::unique_ptr<VariableExpr> callee;
        std::vector<std::unique_ptr<ExprBase>> arguments;

        // after resolving the call function, we store the resolved function type here.
        std::shared_ptr<FunctionType::Signature> resolvedCalleeSignature = nullptr;
    public:
        CallExpr(std::unique_ptr<VariableExpr> callee, std::vector<std::unique_ptr<ExprBase>> arguments)
            : ExprBase(callee->getLoc()), callee(std::move(callee)), arguments(std::move(arguments)) {}

        virtual bool isValidLValue() const override { return callee->isValidLValue(); }
        // Depending on the return type of the callee, we don't know if the call is callable or not.
        // So we return true here.
        virtual bool isCallable() const override { return true; }
        VariableExpr* getCallee() const { return callee.get(); }
        virtual const std::shared_ptr<Type>& getType() const override {
            assert_not_reached("CallExpr::getType() should not be called before resolving the callee signature");
        }
        const std::vector<std::unique_ptr<ExprBase>>& getArguments() const { return arguments; }

        std::shared_ptr<Type> getReturnType() const {
            if (resolvedCalleeSignature) {
                return resolvedCalleeSignature->returnType;
            }
            return nullptr; // or throw an error if not resolved
        }

        void setCalleeSignature(FunctionType::Signature signature) {
            resolvedCalleeSignature = std::make_shared<FunctionType::Signature>(std::move(signature));
            if (resolvedCalleeSignature) {
                this->type = resolvedCalleeSignature->returnType;
            }
        }
        std::shared_ptr<FunctionType::Signature> getCalleeSignature() const {
            return resolvedCalleeSignature;
        }

        ExprBase* getArgument(size_t index) const {
            if (index < arguments.size()) {
                return arguments[index].get();
            }
            return nullptr;
        }

        void print(std::ostream &os) const override {
            os << "Call: [Variable: [" << callee->getName();

            if (resolvedCalleeSignature) {
                os << ": ";
                resolvedCalleeSignature->print(os);
            }

            os << "](";
            for (const auto& arg : arguments) {
                arg->print(os);
                os << ", ";
            }
            os << ")]";
        }

        TYPEID_SYSTEM(ExprBase, CallExpr);
        ACCEPT_DECL();
    };

    class LiteralExpr : public ExprBase {
    protected:
        std::string value;
    public:
        LiteralExpr(std::string value, Location location) : ExprBase(location), value(std::move(value)) {}
        LiteralExpr(lox::Token token) : LiteralExpr(std::string(token.getTokenString()), token.getLoction()) {}

        const std::string& getValue() const { return value; }

        void print(std::ostream &os) const override {
            os << "Literal: [" << value << "]";
        }

        TYPEID_SYSTEM(ExprBase, LiteralExpr);
        ACCEPT_DECL();
    };

    class NumberExpr : public LiteralExpr {
    public:
        NumberExpr(lox::Token token) : LiteralExpr(token) {}

        const double getValue() const {
            return std::stod(LiteralExpr::getValue());
        }

        void print(std::ostream &os) const override {
            os << "Number: [" << getValue() << "]";
        }

        TYPEID_SYSTEM(ExprBase, NumberExpr);
        ACCEPT_DECL();
    };

    class StringExpr : public LiteralExpr {
    public:
        StringExpr(lox::Token token) : LiteralExpr(token) {}

        void print(std::ostream &os) const override {
            os << "String: [" << getValue() << "]";
        }

        TYPEID_SYSTEM(ExprBase, StringExpr);
        ACCEPT_DECL();
    };

    class UnaryExpr : public ExprBase {
    protected:
        TokenType opKind;
        std::unique_ptr<ExprBase> right;
    public:
        UnaryExpr(TokenType opKind, std::unique_ptr<ExprBase> right) : ExprBase(right->getLoc()), right(std::move(right)), opKind(opKind) {}

        ExprBase* getRight() const { return right.get(); }
        TokenType getOpKind() const { return opKind; }

        void print(std::ostream &os) const override {
            os << "Unary: [" << convertTokenTypeToString(opKind) << " ";
            right->print(os);
            os << "]";
        }

        TYPEID_SYSTEM(ExprBase, UnaryExpr);
        ACCEPT_DECL();
    };

    class AccessExpr : public ExprBase {
    protected:
        std::unique_ptr<ExprBase> left;
        std::string property;
    public:
        AccessExpr(std::unique_ptr<ExprBase> left, Token symName) : ExprBase(symName.getLoction()), left(std::move(left)), property(symName.getTokenString()) {}

        const ExprBase* getLeft() const { return left.get(); }
        const std::string& getProperty() const { return property; }
        // depending on the return type of the property, we don't know if the property is callable or not.
        // So we return true here.
        virtual bool isValidLValue() const override { return true; }
        virtual bool isCallable() const override { return true; }

        void print(std::ostream &os) const override {
            os << "Access: [ ";
            left->print(os);
            os << "." << property << " ]";
        }

        TYPEID_SYSTEM(ExprBase, AccessExpr);
        ACCEPT_DECL();
    };

    class BinaryExpr : public ExprBase {
    protected:
        TokenType opKind;
        std::unique_ptr<ExprBase> left;
        std::unique_ptr<ExprBase> right;
    public:
        BinaryExpr(TokenType opKind, std::unique_ptr<ExprBase> left, std::unique_ptr<ExprBase> right)
            : ExprBase(right->getLoc()), left(std::move(left)), right(std::move(right)), opKind(opKind) {}

        virtual ExprBase* getLeft() const { return left.get(); }
        virtual ExprBase* getRight() const { return right.get(); }
        virtual TokenType getOpKind() const { return opKind; }

        // depending on the return type of the left and right
        // we return true here.
        virtual bool isCallable() const override { return true; }

        virtual void print(std::ostream &os) const override {
            os << "Binary: [";
            left->print(os);
            os << " " << convertTokenTypeToString(opKind) << " ";
            right->print(os);
            os << "]";
        }

        TYPEID_SYSTEM(ExprBase, BinaryExpr);
        ACCEPT_DECL();
    };

    class AssignExpr : public BinaryExpr {
    public:
        AssignExpr(std::unique_ptr<ExprBase> left, std::unique_ptr<ExprBase> right)
            : BinaryExpr(lox::TokenType::TOKEN_EQUAL, std::move(left), std::move(right)) {}

        void print(std::ostream &os) const override {
            os << "Assign: [";
            left->print(os);
            os << " = ";
            right->print(os);
            os << "]";
        }

        TYPEID_SYSTEM(ExprBase, AssignExpr);
        ACCEPT_DECL();
    };
} // namespace lox


#endif // EXPR_H