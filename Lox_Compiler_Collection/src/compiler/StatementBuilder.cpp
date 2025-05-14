#include "compiler/Parser.h"

namespace lox
{
    std::unordered_set<std::string> ClassDeclStmt::classTable;

    std::unique_ptr<StmtBase> Parser::parseDeclaration() {
        std::unique_ptr<StmtBase> stmt;
        if (this->parseOptional(lox::TokenType::TOKEN_CLASS)) {
            stmt = this->parseClassDecl();
        } else if (this->parseOptional(lox::TokenType::TOKEN_FUN)) {
            stmt = this->parseFunctionDecl();
        } else if (this->parseOptional(lox::TokenType::TOKEN_VAR)) {
            stmt = this->parseVarDecl();
        } else {
            stmt = this->parseStatement();
        }

        this->synchronize();
        return stmt;
    }

    std::unique_ptr<VarDeclStmt> Parser::parseVarDecl() {
        this->parse(lox::TokenType::TOKEN_IDENTIFIER, "Expect a variable name");
        if (this->getPreviousToken() != lox::TokenType::TOKEN_IDENTIFIER) {
            return nullptr;
        }
        std::string name = std::string(this->getPreviousToken().getTokenString());
        std::unique_ptr<ExprBase> initializer;
        if (this->parseOptional(lox::TokenType::TOKEN_EQUAL)) {
            initializer = this->parseExpression();
        }
        this->parse(lox::TokenType::TOKEN_SEMICOLON);
        if (initializer == nullptr) {
            return std::make_unique<VarDeclStmt>(std::move(name), this->getPreviousToken().getLoction());
        }
        return std::make_unique<VarDeclStmt>(std::move(name), std::move(initializer));
    }

    std::unique_ptr<FunctionDecl> Parser::parseFunctionDecl() {
        this->parse(lox::TokenType::TOKEN_IDENTIFIER);
        std::string name = std::string(this->getPreviousToken().getTokenString());
        this->parse(lox::TokenType::TOKEN_LEFT_PAREN);
        std::vector<std::unique_ptr<VariableExpr>> parameters;
        if (!this->parseOptional(lox::TokenType::TOKEN_RIGHT_PAREN)) {
            do {
                this->parse(lox::TokenType::TOKEN_IDENTIFIER);
                std::string paramName = std::string(this->getPreviousToken().getTokenString());
                parameters.push_back(std::make_unique<VariableExpr>(std::move(paramName), this->getPreviousToken().getLoction()));
            } while (this->parseOptional(lox::TokenType::TOKEN_COMMA) && this->hasNext());
            this->parse(lox::TokenType::TOKEN_RIGHT_PAREN);
        }
        this->parse(lox::TokenType::TOKEN_LEFT_BRACE);
        std::unique_ptr<BlockStmt> body = this->parseBlockStmt();
        return std::make_unique<FunctionDecl>(std::move(name), std::move(parameters), std::move(body));
    }

    std::unique_ptr<ClassDeclStmt> Parser::parseClassDecl() {
        this->parse(lox::TokenType::TOKEN_IDENTIFIER);
        std::string name = std::string(this->getPreviousToken().getTokenString());
        std::optional<std::string> superclass;
        if (this->parseOptional(lox::TokenType::TOKEN_LESS)) {
            this->parse(lox::TokenType::TOKEN_IDENTIFIER, "Expect superclass name");
            if (this->getPreviousToken() == lox::TokenType::TOKEN_IDENTIFIER) {
                superclass = std::string(this->getPreviousToken().getTokenString());
                if (name == *superclass) {
                    this->parseError("A class can't inherit from itself.");
                }
                else if (!ClassDeclStmt::validSuperclass(*superclass)) {
                    this->parseError("Invalid superclass.");
                }
            }
        }
        std::unordered_map<std::string, std::unique_ptr<FunctionDecl>> methods;
        // std::unordered_map<std::string, std::unique_ptr<VarDeclStmt>> fields;
        this->parse(lox::TokenType::TOKEN_LEFT_BRACE);
        while (!this->parseOptional(lox::TokenType::TOKEN_RIGHT_BRACE) && this->hasNext()) {
            // if (this->parseOptional(lox::TokenType::TOKEN_VAR)) {
            //     std::unique_ptr<VarDeclStmt> field = this->parseVarDecl();
            //     fields.insert({field->getName(), std::move(field)});
            // } else
            if (this->parseOptional(lox::TokenType::TOKEN_FUN) ||
                (this->match(TokenType::TOKEN_IDENTIFIER) && this->getCurrentToken() == "init")) {
                std::unique_ptr<FunctionDecl> method = this->parseFunctionDecl();
                methods.insert({method->getName(), std::move(method)});
            } else {
                this->parseError("Expect `var` or `fun` or constructor.");
                this->synchronize(lox::TokenType::TOKEN_RIGHT_BRACE);
            }
        }

        if (!superclass) {
            return std::make_unique<ClassDeclStmt>(std::move(name), std::move(methods), this->getPreviousToken().getLoction());
        }
        return std::make_unique<ClassDeclStmt>(std::move(name), std::move(superclass), std::move(methods), this->getPreviousToken().getLoction());
    }

    std::unique_ptr<StmtBase> Parser::parseStatement() {
        if (this->parseOptional(lox::TokenType::TOKEN_LEFT_BRACE)) {
            return this->parseBlockStmt();
        } else if (this->parseOptional(lox::TokenType::TOKEN_IF)) {
            return this->parseIfStmt();
        } else if (this->parseOptional(lox::TokenType::TOKEN_RETURN)) {
            return this->parseReturnStmt();
        } else if (this->parseOptional(lox::TokenType::TOKEN_WHILE)) {
            return this->parseWhileStmt();
        } else if (this->parseOptional(lox::TokenType::TOKEN_FOR)) {
            return this->parseForStmt();
        } else {
            return this->parseExpressionStmt();
        }
    }

    std::unique_ptr<BlockStmt> Parser::parseBlockStmt() {
        std::vector<std::unique_ptr<StmtBase>> statements;
        while (!this->parseOptional(lox::TokenType::TOKEN_RIGHT_BRACE) && this->hasNext()) {
            std::unique_ptr<StmtBase> stmt = this->parseDeclaration();
            if (stmt != nullptr) {
                statements.push_back(std::move(stmt));
            }
        }

        return std::make_unique<BlockStmt>(std::move(statements), this->getPreviousToken().getLoction());
    }

    std::unique_ptr<ExpressionStmt> Parser::parseExpressionStmt() {
        std::unique_ptr<ExprBase> expr = this->parseExpression();
        this->parse(lox::TokenType::TOKEN_SEMICOLON);
        if (!expr) {
            return nullptr;
        }
        return std::make_unique<ExpressionStmt>(std::move(expr));
    }

    std::unique_ptr<IfStmt> Parser::parseIfStmt() {
        this->parse(lox::TokenType::TOKEN_LEFT_PAREN);
        std::unique_ptr<ExprBase> condition = this->parseExpression();
        this->parse(lox::TokenType::TOKEN_RIGHT_PAREN);
        std::unique_ptr<BlockStmt> thenBranch = nullptr;
        if (this->parseOptional(lox::TokenType::TOKEN_LEFT_BRACE)) {
            thenBranch = this->parseBlockStmt();
        }
        else {
            std::unique_ptr<StmtBase> stmt = this->parseStatement();
            std::vector<std::unique_ptr<StmtBase>> statements;
            statements.push_back(std::move(stmt));
            thenBranch = std::make_unique<BlockStmt>(std::move(statements), this->getPreviousToken().getLoction());
        }

        if (thenBranch == nullptr) {
            this->parseError("Expect a statement or a block after `if`.");
            return nullptr;
        }

        std::unique_ptr<BlockStmt> elseBranch;
        if (this->parseOptional(lox::TokenType::TOKEN_ELSE)) {
            if (this->parseOptional(lox::TokenType::TOKEN_LEFT_BRACE)) {
                elseBranch = this->parseBlockStmt();
            }
            else {
                std::unique_ptr<StmtBase> stmt = this->parseStatement();
                std::vector<std::unique_ptr<StmtBase>> statements;
                statements.push_back(std::move(stmt));
                elseBranch = std::make_unique<BlockStmt>(std::move(statements), this->getPreviousToken().getLoction());
            }
        }

        return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), this->getPreviousToken().getLoction());
    }

    std::unique_ptr<ReturnStmt> Parser::parseReturnStmt() {
        std::unique_ptr<ExprBase> value;
        if (!this->parseOptional(lox::TokenType::TOKEN_SEMICOLON)) {
            value = this->parseExpression();
            this->parse(lox::TokenType::TOKEN_SEMICOLON);
            return std::make_unique<ReturnStmt>(std::move(value));
        }
        return std::make_unique<ReturnStmt>(this->getPreviousToken().getLoction());
    }

    std::unique_ptr<ForStmt> Parser::parseForStmt() {
        this->parse(lox::TokenType::TOKEN_LEFT_PAREN);
        std::unique_ptr<StmtBase> initializer;
        if (this->parseOptional(lox::TokenType::TOKEN_SEMICOLON)) {
            initializer = nullptr;
        } else if (this->parseOptional(lox::TokenType::TOKEN_VAR)) {
            initializer = this->parseVarDecl();
        } else {
            initializer = this->parseExpressionStmt();
        }

        if (this->getPreviousToken() != lox::TokenType::TOKEN_SEMICOLON) {
            this->synchronize();
        }

        std::unique_ptr<ExprBase> condition;
        if (!this->parseOptional(lox::TokenType::TOKEN_SEMICOLON)) {
            condition = this->parseExpression();
            this->parse(lox::TokenType::TOKEN_SEMICOLON);
        }

        if (this->getPreviousToken() != lox::TokenType::TOKEN_SEMICOLON) {
            this->synchronize();
        }

        std::unique_ptr<ExprBase> increment;
        if (!this->parseOptional(lox::TokenType::TOKEN_RIGHT_PAREN)) {
            increment = this->parseExpression();
            this->parse(lox::TokenType::TOKEN_RIGHT_PAREN);
        }

        std::unique_ptr<BlockStmt> body;
        if (this->parseOptional(lox::TokenType::TOKEN_LEFT_BRACE)) {
            body = this->parseBlockStmt();
        }
        else {
            std::unique_ptr<StmtBase> stmt = this->parseStatement();
            std::vector<std::unique_ptr<StmtBase>> statements;
            statements.push_back(std::move(stmt));
            body = std::make_unique<BlockStmt>(std::move(statements), this->getPreviousToken().getLoction());
        }
    
        return std::make_unique<ForStmt>(std::move(initializer), std::move(condition), std::move(increment), std::move(body));
    }

    std::unique_ptr<WhileStmt> Parser::parseWhileStmt() {
        this->parse(lox::TokenType::TOKEN_LEFT_PAREN);
        std::unique_ptr<ExprBase> condition = this->parseExpression();
        this->parse(lox::TokenType::TOKEN_RIGHT_PAREN);
        std::unique_ptr<BlockStmt> body;
        if (this->parseOptional(lox::TokenType::TOKEN_LEFT_BRACE)) {
            body = this->parseBlockStmt();
        }
        else {
            std::unique_ptr<StmtBase> stmt = this->parseStatement();
            std::vector<std::unique_ptr<StmtBase>> statements;
            if (stmt != nullptr)
                statements.push_back(std::move(stmt));
            body = std::make_unique<BlockStmt>(std::move(statements), this->getPreviousToken().getLoction());
        }

        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }
}