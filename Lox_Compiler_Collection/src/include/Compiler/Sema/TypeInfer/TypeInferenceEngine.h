#ifndef TYPE_INFERENCE_ENGINE_H
#define TYPE_INFERENCE_ENGINE_H

#include "Compiler/Sema/SymbolTable.h"
#include "Compiler/Sema/TypeInfer/Constraint.h"

class TypeInferenceEngine {
private:
    static std::unique_ptr<PrimitiveType> NumberType;
    static std::unique_ptr<PrimitiveType> StringType;
    static std::unique_ptr<PrimitiveType> BoolType;
    static std::unique_ptr<NilType> NilType;

    SymbolTable symbolTable;
    std::vector<Constraint> constraints;
    std::unordered_map<TypeVariable *, Type *> substitutions;

    void collectTypeDeclarations(const std::vector<std::unique_ptr<StmtBase>> &statements);
    void collectClassDeclarations(ClassDeclStmt *classDecl);
    void collectFunctionDeclarations(FunctionDeclStmt *funcDecl);

    void inferStatements(const std::vector<std::unique_ptr<StmtBase>> &statements);
    void inferStatement(StmtBase *stmt);
    void inferVarDeclStmt(VarDeclStmt *varDecl);
    void inferFunctionDeclStmt(FunctionDeclStmt *funcDecl);
    void inferClassDeclStmt(ClassDeclStmt *classDecl);
    void inferBlockStmt(BlockStmt *blockStmt);

    const Type *inferExpr(ExprBase *expr, const Type *expectedType = nullptr);
    const Type *inferBinaryExpr(BinaryExpr *binaryExpr, const Type *expectedType = nullptr);
    const Type *inferUnaryExpr(UnaryExpr *unaryExpr, const Type *expectedType = nullptr);
    const Type *inferCallExpr(CallExpr *callExpr, const Type *expectedType = nullptr);
    const Type *inferAssignExpr(AssignExpr *assignExpr, const Type *expectedType = nullptr);
    const Type *inferAccessExpr(AccessExpr *accessExpr, const Type *expectedType = nullptr);

    const Type *applySubstitutions(const Type *type);
public:
    TypeInferenceEngine();
    ~TypeInferenceEngine() = default;

    // 添加约束
    void addConstraint(const Type *left, const Type *right, Constraint::ConstraintType relation) {
        constraints.emplace_back(left, right, relation);
    }

    void inferProgramTypes(const std::vector<std::unique_ptr<StmtBase>> &statements);
}

#endif // TYPE_INFERENCE_ENGINE_H
