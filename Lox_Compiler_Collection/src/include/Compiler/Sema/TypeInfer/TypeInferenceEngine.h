#ifndef TYPE_INFERENCE_ENGINE_H
#define TYPE_INFERENCE_ENGINE_H

#include "Compiler/Sema/SymbolTable.h"
#include "Compiler/Sema/TypeInfer/Constraint.h"

class TypeInferenceEngine {
private:
    static std::shared_ptr<PrimitiveType> NumberType;
    static std::shared_ptr<PrimitiveType> StringType;
    static std::shared_ptr<PrimitiveType> BoolType;
    static std::shared_ptr<NilType> NilType;

    SymbolTable symbolTable;
    std::vector<Constraint> constraints;
    std::unordered_map<TypeVariable *, Type *> typeCache;

    void collectTypeDeclarations(const std::vector<std::unique_ptr<StmtBase>> &statements);
    void collectClassDeclarations(ClassDeclStmt *classDecl);
    void collectFunctionDeclarations(FunctionDeclStmt *funcDecl);

    void inferStatements(const std::vector<std::unique_ptr<StmtBase>> &statements);
    void inferVarDeclStmt(VarDeclStmt *varDecl);
    void inferFunctionDeclStmt(FunctionDeclStmt *funcDecl);
    void inferClassDeclStmt(ClassDeclStmt *classDecl);
    void inferBlockStmt(BlockStmt *blockStmt);
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
