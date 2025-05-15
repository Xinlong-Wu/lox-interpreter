#ifndef EXPR_H
#define EXPR_H

#include <iostream>
#include <vector>
#include <memory>

namespace lox
{
    class ExprBase {
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
    };

    class GroupingExpr : public ExprBase {
    protected:
        std::unique_ptr<ExprBase> expression;
    public:
        GroupingExpr(std::unique_ptr<ExprBase> expression) : ExprBase(expression->getLoc()), expression(std::move(expression)) {}

        const ExprBase* getExpression() const { return expression.get(); }

        virtual bool isValidLValue() const override { return expression->isValidLValue(); }
        virtual bool isCallable() const override { return expression->isCallable(); }
        void print(std::ostream &os) const override {
            os << "( ";
            expression->print(os);
            os << " )";
        }
    };

    class CallExpr : public ExprBase {
    protected:
        std::unique_ptr<ExprBase> callee;
        std::vector<std::unique_ptr<ExprBase>> arguments;
    public:
        CallExpr(std::unique_ptr<ExprBase> callee, std::vector<std::unique_ptr<ExprBase>> arguments)
            : ExprBase(callee->getLoc()), callee(std::move(callee)), arguments(std::move(arguments)) {}

        virtual bool isValidLValue() const override { return callee->isValidLValue(); }
        // Depending on the return type of the callee, we don't know if the call is callable or not.
        // So we return true here.
        virtual bool isCallable() const override { return true; }
        const ExprBase* getCallee() const { return callee.get(); }
        const ExprBase* getArgument(size_t index) const {
            if (index < arguments.size()) {
                return arguments[index].get();
            }
            return nullptr;
        }

        void print(std::ostream &os) const override {
            os << "Call: [";
            callee->print(os);
            os << "(";
            for (const auto& arg : arguments) {
                arg->print(os);
                os << ", ";
            }
            os << ")]";
        }
    };

    class VariableExpr : public ExprBase {
    protected:
        std::string name;
    public:
        VariableExpr(std::string name, Location location) : ExprBase(location), name(std::move(name)) {}
        VariableExpr(lox::Token token) : VariableExpr(std::string(token.getTokenString()), token.getLoction()) {}

        const std::string &getSymName() const { return name; }
        virtual bool isValidLValue() const override { return true; }
        // depending on the return type of the variable, we don't know if the variable is callable or not.
        // So we return true here.
        virtual bool isCallable() const override { return true; }

        void print(std::ostream &os) const override {
            os << "Variable: [" << name << "]";
        }
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
    };

    class StringExpr : public LiteralExpr {
    public:
        StringExpr(lox::Token token) : LiteralExpr(token) {}

        void print(std::ostream &os) const override {
            os << "String: [" << getValue() << "]";
        }
    };

    class UnaryExpr : public ExprBase {
    protected:
        TokenType kind;
        std::unique_ptr<ExprBase> right;
    public:
        UnaryExpr(TokenType kind, std::unique_ptr<ExprBase> right) : ExprBase(right->getLoc()), right(std::move(right)), kind(kind) {}

        const ExprBase* getRight() const { return right.get(); }
        TokenType getKind() const { return kind; }

        void print(std::ostream &os) const override {
            os << "Unary: [" << convertTokenTypeToString(kind) << " ";
            right->print(os);
            os << "]";
        }
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
    };

    class BinaryExpr : public ExprBase {
    protected:
        TokenType kind;
        std::unique_ptr<ExprBase> left;
        std::unique_ptr<ExprBase> right;
    public:
        BinaryExpr(TokenType kind, std::unique_ptr<ExprBase> left, std::unique_ptr<ExprBase> right)
            : ExprBase(right->getLoc()), left(std::move(left)), right(std::move(right)), kind(kind) {}

        virtual const ExprBase* getLeft() const { return left.get(); }
        virtual const ExprBase* getRight() const { return right.get(); }

        // depending on the return type of the left and right
        // we return true here.
        virtual bool isCallable() const override { return true; }

        virtual void print(std::ostream &os) const override {
            os << "Binary: [";
            left->print(os);
            os << " " << convertTokenTypeToString(kind) << " ";
            right->print(os);
            os << "]";
        }
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
    };
} // namespace lox


#endif // EXPR_H