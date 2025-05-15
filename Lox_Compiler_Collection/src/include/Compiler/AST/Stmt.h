#ifndef STMT_H
#define STMT_H

#include "Compiler/AST/Expr.h"

#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace lox
{
    class StmtBase
    {
    private:
        Location loc;

    public:
        StmtBase(Location location) : loc(location) {};
        virtual ~StmtBase() = default;

        virtual const Location &getLoc() const { return loc; };

        virtual void print(std::ostream &os) const = 0;
        virtual void dump() const
        {
            this->print(std::cout);
            std::cout << std::endl;
        }
    };

    class ExpressionStmt : public StmtBase
    {
    private:
        std::unique_ptr<ExprBase> expression;

    public:
        ExpressionStmt(std::unique_ptr<ExprBase> expression) : StmtBase(expression->getLoc().getNextColumn()), expression(std::move(expression)) {}

        virtual void print(std::ostream &os) const override
        {
            expression->print(os);
            os << ";";
        }
    };

    class DeclarationStmt : public StmtBase
    {
    protected:
        std::string name;
    public:
        DeclarationStmt(std::string name, Location loc) : StmtBase(loc), name(std::move(name)) {}
        ~DeclarationStmt() override = default;

        const std::string &getName() const { return name; }
    };

    class VarDeclStmt : public DeclarationStmt
    {
    private:
        std::unique_ptr<ExprBase> initializer;
    public:
        VarDeclStmt(std::string name, std::unique_ptr<ExprBase> initializer) : DeclarationStmt(std::move(name), initializer->getLoc()), initializer(std::move(initializer)) {}
        VarDeclStmt(std::string name, Location loc) : DeclarationStmt(std::move(name), loc) {}
        ~VarDeclStmt() override = default;

        virtual void print(std::ostream &os) const override
        {
            os << "var " << name;
            if (initializer) {
                os << " = ";
                initializer->print(os);
            }
            os << ";";
        }
    };

    class BlockStmt : public StmtBase
    {
    private:
        std::vector<std::unique_ptr<StmtBase>> statements;

    public:
        BlockStmt(std::vector<std::unique_ptr<StmtBase>> statements, Location location) : StmtBase(location), statements(std::move(statements)) {}
        BlockStmt(std::vector<std::unique_ptr<StmtBase>> statements) : BlockStmt(std::move(statements), statements[statements.size() - 1]->getLoc().getNextColumn()) {}
        ~BlockStmt() override = default;

        virtual void print(std::ostream &os) const override
        {
            os << "BlockStmt: " << "{" << std::endl;
            for (const auto &stmt : statements)
            {
                stmt->print(os);
                os << std::endl;
            }
            os << "}" << std::endl;
        }
    };

    class FunctionDecl : public DeclarationStmt
    {
    private:
        std::vector<std::unique_ptr<VariableExpr>> parameters;
        std::unique_ptr<BlockStmt> body;
    public:
        FunctionDecl(std::string name, std::vector<std::unique_ptr<VariableExpr>> parameters, std::unique_ptr<BlockStmt> body) : DeclarationStmt(std::move(name), body->getLoc()), parameters(std::move(parameters)), body(std::move(body)) {}
        ~FunctionDecl() override = default;

        virtual void print(std::ostream &os) const override
        {
            os << "function " << name << "(";
            for (const auto &param : parameters)
            {
                param->print(os);
                os << ", ";
            }
            os << ") " << std::endl;
            body->print(os);
            os << std::endl;
        }
    };

    class ClassDeclStmt : public DeclarationStmt
    {
    private:
        static std::unordered_set<std::string> classTable;

        std::optional<std::string> superclass;
        std::unordered_map<std::string, std::unique_ptr<FunctionDecl>> methods;
        std::unordered_map<std::string, std::unique_ptr<VarDeclStmt>> fields;
    public:
        ClassDeclStmt(std::string name, std::optional<std::string> superclass, std::unordered_map<std::string, std::unique_ptr<FunctionDecl>> methods, std::unordered_map<std::string, std::unique_ptr<VarDeclStmt>> fields, Location loc) : DeclarationStmt(std::move(name), loc), superclass(std::move(superclass)), methods(std::move(methods)), fields(std::move(fields)) {
            // assert(!this->superclass || validSuperclass(*this->superclass) && "Invalid superclass");
            classTable.insert(this->name);
        }
        ClassDeclStmt(std::string name, std::unordered_map<std::string, std::unique_ptr<FunctionDecl>> methods, std::unordered_map<std::string, std::unique_ptr<VarDeclStmt>> fields, Location loc) : ClassDeclStmt(std::move(name), std::nullopt, std::move(methods), std::move(fields), loc) {}

        // no fields constructor
        ClassDeclStmt(std::string name, std::optional<std::string> superclass, std::unordered_map<std::string, std::unique_ptr<FunctionDecl>> methods, Location loc) : DeclarationStmt(std::move(name), loc), superclass(std::move(superclass)), methods(std::move(methods)) {
            // assert(!this->superclass || validSuperclass(*this->superclass) && "Invalid superclass");
            classTable.insert(this->name);
        }
        ClassDeclStmt(std::string name, std::unordered_map<std::string, std::unique_ptr<FunctionDecl>> methods, Location loc) : ClassDeclStmt(std::move(name), std::nullopt, std::move(methods), loc) {}
        ~ClassDeclStmt() override = default;

        static bool validSuperclass(const std::string &name)
        {
            return classTable.find(name) != classTable.end();
        }

        virtual void print(std::ostream &os) const override
        {
            os << "class " << name;
            if (superclass) {
                os << " < " << *superclass;
            }
            os << " {" << std::endl;
            for (const auto &method : methods) {
                method.second->print(os);
                os << std::endl;
            }
            os << "}" << std::endl;
        }
    };

    class IfStmt : public StmtBase
    {
    private:
        std::unique_ptr<ExprBase> condition;
        std::unique_ptr<BlockStmt> thenBlock;
        std::unique_ptr<BlockStmt> elseBlock;

    public:
        IfStmt(std::unique_ptr<ExprBase> condition, std::unique_ptr<BlockStmt> thenBlock, std::unique_ptr<BlockStmt> elseBlock, Location location) : StmtBase(location), condition(std::move(condition)), thenBlock(std::move(thenBlock)), elseBlock(std::move(elseBlock)) {}
        ~IfStmt() override = default;

        virtual void print(std::ostream &os) const override
        {
            os << "if (";
            condition->print(os);
            os << ") " << std::endl;
            thenBlock->print(os);
            if (elseBlock)
            {
                os << " else ";
                elseBlock->print(os);
            }
        }
    };

    class ReturnStmt : public StmtBase
    {
    private:
        std::unique_ptr<ExprBase> value;
    public:
        ReturnStmt(std::unique_ptr<ExprBase> value) : StmtBase(value->getLoc()), value(std::move(value)) {}
        ReturnStmt(Location loc) : StmtBase(loc) {}
        ~ReturnStmt() override = default;

        virtual void print(std::ostream &os) const override
        {
            os << "return ";
            if (value)
            {
                value->print(os);
            }
            os << ";";
        }
    };

    class ForStmt : public StmtBase
    {
    protected:
        std::unique_ptr<StmtBase> initializer;
        std::unique_ptr<ExprBase> condition;
        std::unique_ptr<ExprBase> increment;
        std::unique_ptr<BlockStmt> body;
    public:
        ForStmt(std::unique_ptr<StmtBase> initializer, std::unique_ptr<ExprBase> condition, std::unique_ptr<ExprBase> increment, std::unique_ptr<BlockStmt> body) : StmtBase(body->getLoc()), initializer(std::move(initializer)), condition(std::move(condition)), increment(std::move(increment)), body(std::move(body)) {}
        ~ForStmt() override = default;

        virtual void print(std::ostream &os) const override
        {
            os << "for (" << std::endl;
            if (initializer)
            {
                os << "initializer: ";
                initializer->print(os);
                os << std::endl;
            }
            if (condition)
            {
                os << "condition: ";
                condition->print(os);
                os << "; " << std::endl;
            }
            if (increment)
            {
                os << "increment: ";
                increment->print(os);
                os << std::endl;
            }
            os << ") ";
            body->print(os);
        }
    };

    class WhileStmt : public ForStmt
    {
    public:
        WhileStmt(std::unique_ptr<ExprBase> condition, std::unique_ptr<BlockStmt> body) : ForStmt(nullptr, std::move(condition), nullptr, std::move(body)) {}
        ~WhileStmt() override = default;

        virtual void print(std::ostream &os) const override
        {
            os << "while (" << std::endl;
            os << "condition: ";
            condition->print(os);
            os << std::endl << ") ";
            body->print(os);
        }
    };

} // namespace lox

#endif // STMT_H