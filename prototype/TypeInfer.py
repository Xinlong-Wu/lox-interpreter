#!/usr/bin/env python3
"""
类型推断系统实现
支持类、函数重载的静态类型语言的类型推断
采用局部类型推断 + 双向类型检查方法
"""
from abc import ABC, abstractmethod
from dataclasses import dataclass
from typing import Dict, List, Set, Optional, Union, Any
from enum import Enum
import copy

# ==================== 类型系统定义 ====================

class TypeKind(Enum):
    PRIMITIVE = "primitive"
    CLASS = "class"
    FUNCTION = "function"
    GENERIC = "generic"
    TYPE_VAR = "type_var"
    VOID = "void"

@dataclass
class Type(ABC):
    """类型基类"""
    kind: TypeKind

    @abstractmethod
    def __str__(self) -> str:
        pass

    def is_assignable_to(self, other: 'Type') -> bool:
        """检查是否可以赋值给另一个类型"""
        return self == other

@dataclass
class PrimitiveType(Type):
    name: str

    def __init__(self, name: str):
        super().__init__(TypeKind.PRIMITIVE)
        self.name = name

    def __str__(self) -> str:
        return self.name

    def __eq__(self, other) -> bool:
        return isinstance(other, PrimitiveType) and self.name == other.name

@dataclass
class ClassType(Type):
    name: str
    super_class: Optional['ClassType']
    type_params: List['Type']

    def __init__(self, name: str, super_class: Optional['ClassType'] = None, type_params: List['Type'] = None):
        super().__init__(TypeKind.CLASS)
        self.name = name
        self.super_class = super_class
        self.type_params = type_params or []

    def __str__(self) -> str:
        if self.type_params:
            params = ", ".join(str(p) for p in self.type_params)
            return f"{self.name}<{params}>"
        return self.name

    def __eq__(self, other) -> bool:
        return (isinstance(other, ClassType) and
                self.name == other.name and
                self.type_params == other.type_params)

    def is_assignable_to(self, other: Type) -> bool:
        if self == other:
            return True
        if isinstance(other, ClassType):
            # 检查继承关系
            current = self.super_class
            while current:
                if current == other:
                    return True
                current = current.super_class
        return False

@dataclass
class FunctionType(Type):
    param_types: List[Type]
    return_type: Type

    def __init__(self, param_types: List[Type], return_type: Type):
        super().__init__(TypeKind.FUNCTION)
        self.param_types = param_types
        self.return_type = return_type

    def __str__(self) -> str:
        params = ", ".join(str(p) for p in self.param_types)
        return f"({params}) -> {self.return_type}"

    def __eq__(self, other) -> bool:
        return (isinstance(other, FunctionType) and
                self.param_types == other.param_types and
                self.return_type == other.return_type)

@dataclass
class TypeVariable(Type):
    name: str
    constraints: Set[Type]
    bound: Optional[Type] = None

    def __init__(self, name: str):
        super().__init__(TypeKind.TYPE_VAR)
        self.name = name
        self.constraints = set()
        self.bound = None

    def __str__(self) -> str:
        return f"'{self.name}"

    def __eq__(self, other) -> bool:
        return isinstance(other, TypeVariable) and self.name == other.name

    def __hash__(self) -> int:
        return hash(self.name)

@dataclass
class VoidType(Type):
    def __init__(self):
        super().__init__(TypeKind.VOID)

    def __str__(self) -> str:
        return "void"

    def __eq__(self, other) -> bool:
        return isinstance(other, VoidType)

# ==================== AST节点定义 ====================

@dataclass(kw_only=True)
class ASTNode(ABC):
    """AST节点基类"""
    inferred_type: Optional[Type] = None

@dataclass
class Expression(ASTNode):
    """表达式基类"""
    pass

@dataclass
class Statement(ASTNode):
    """语句基类"""
    pass

@dataclass
class LiteralExpr(Expression):
    value: Any
    literal_type: Type

@dataclass
class IdentifierExpr(Expression):
    name: str

@dataclass
class BinaryOpExpr(Expression):
    left: Expression
    operator: str
    right: Expression

@dataclass
class CallExpr(Expression):
    callee: Expression
    args: List[Expression]

@dataclass
class MemberAccessExpr(Expression):
    object: Expression
    member: str

@dataclass
class AssignmentExpr(Expression):
    target: Expression
    value: Expression

@dataclass
class NewExpr(Expression):
    """构造函数调用表达式"""
    class_name: str
    args: List[Expression]

@dataclass
class ConstructorDecl(Statement):
    """构造函数声明"""
    class_name: str
    params: List['Parameter']
    body: List[Statement]
    constructor_env: Optional['Environment'] = None

@dataclass
class FieldDecl(Statement):
    """字段声明"""
    name: str
    type: Optional[Type]
    initializer: Optional[Expression]

@dataclass
class VarDecl(Statement):
    name: str
    type_annotation: Optional[Type]
    initializer: Optional[Expression]

@dataclass
class FunctionDecl(Statement):
    name: str
    params: List['Parameter']
    return_type: Optional[Type]
    body: List[Statement]
    function_env: Optional['Environment'] = None

@dataclass
class Parameter:
    name: str
    type: Optional[Type]

@dataclass
class ClassDecl(Statement):
    name: str
    super_class: Optional[str]
    methods: List[FunctionDecl]
    fields: List[VarDecl]
    constructors: List[ConstructorDecl] = None
    class_env: Optional['Environment'] = None

    def __post_init__(self):
        if self.constructors is None:
            self.constructors = []

@dataclass
class ReturnStmt(Statement):
    value: Optional[Expression]

# ==================== 符号表和环境 ====================

@dataclass
class Symbol:
    name: str
    type: Type
    is_mutable: bool = True

class Environment:
    """环境/作用域管理"""
    def __init__(self, parent: Optional['Environment'] = None):
        self.parent = parent
        self.symbols: Dict[str, Symbol] = {}
        self.types: Dict[str, Type] = {}

    def define(self, name: str, symbol: Symbol):
        self.symbols[name] = symbol

    def lookup(self, name: str) -> Optional[Symbol]:
        if name in self.symbols:
            return self.symbols[name]
        if self.parent:
            return self.parent.lookup(name)
        return None

    def define_type(self, name: str, type_def: Type):
        self.types[name] = type_def

    def lookup_type(self, name: str) -> Optional[Type]:
        if name in self.types:
            return self.types[name]
        if self.parent:
            return self.parent.lookup_type(name)
        return None

class OverloadSet:
    """重载集合"""
    def __init__(self, name: str):
        self.name = name
        self.candidates: List[FunctionType] = []
        self.declarations: List[FunctionDecl] = []

    def add_overload(self, func_type: FunctionType, decl: FunctionDecl):
        self.candidates.append(func_type)
        self.declarations.append(decl)

# ==================== 类型推断引擎 ====================

class TypeInferenceEngine:
    def __init__(self):
        self.env_stack: List[Environment] = []
        self.type_var_counter = 0
        self.constraints: List[tuple] = []  # (type1, type2, relation)
        self.substitutions: Dict[TypeVariable, Type] = {}
        self.overload_sets: Dict[str, OverloadSet] = {}
        self.class_definitions: Dict[str, ClassDecl] = {}
        self.constructors: Dict[str, List[ConstructorDecl]] = {}

        # 内置类型
        self.int_type = PrimitiveType("int")
        self.string_type = PrimitiveType("string")
        self.bool_type = PrimitiveType("bool")
        self.void_type = VoidType()

        # 全局环境
        self.global_env = Environment()
        self.push_env(self.global_env)
        self._init_builtin_types()

    def _init_builtin_types(self):
        """初始化内置类型"""
        self.global_env.define_type("int", self.int_type)
        self.global_env.define_type("string", self.string_type)
        self.global_env.define_type("bool", self.bool_type)
        self.global_env.define_type("void", self.void_type)

    def push_env(self, env: Environment):
        self.env_stack.append(env)

    def pop_env(self):
        if len(self.env_stack) > 1:  # 保留全局环境
            self.env_stack.pop()

    def current_env(self) -> Environment:
        return self.env_stack[-1]

    def fresh_type_var(self, prefix: str = "T") -> TypeVariable:
        """生成新的类型变量"""
        self.type_var_counter += 1
        return TypeVariable(f"{prefix}{self.type_var_counter}")

    def add_constraint(self, type1: Type, type2: Type, relation: str = "equal"):
        """添加类型约束"""
        self.constraints.append((type1, type2, relation))

    def infer_program(self, statements: List[Statement]) -> bool:
        """推断整个程序的类型"""
        try:
            # 第一遍：收集类型声明和函数签名
            self._collect_declarations(statements)

            # 第二遍：类型推断和约束生成
            for stmt in statements:
                self._infer_statement(stmt)

            # 约束求解
            success = self._solve_constraints()

            # 应用替换
            if success:
                self._apply_substitutions(statements)

            return success

        except Exception as e:
            print(f"类型推断错误: {e}")
            return False

    def _collect_declarations(self, statements: List[Statement]):
        """收集类型声明和函数签名"""
        for stmt in statements:
            if isinstance(stmt, ClassDecl):
                self._collect_class_decl(stmt)
            elif isinstance(stmt, FunctionDecl):
                self._collect_function_decl(stmt)

    def _collect_constructor(self, constructor: ConstructorDecl, class_name: str):
        """收集构造函数"""
        param_types = []
        for param in constructor.params:
            if param.type:
                param_types.append(param.type)
            else:
                param_types.append(self.fresh_type_var(f"P_{param.name}"))

        # 构造函数返回类实例
        class_type = self.current_env().lookup_type(class_name)
        constructor_type = FunctionType(param_types, class_type)

        # 存储构造函数
        if class_name not in self.constructors:
            self.constructors[class_name] = []
        self.constructors[class_name].append(constructor)

        # 将构造函数作为特殊函数处理
        symbol = Symbol(f"{class_name}.__init__", constructor_type)
        self.current_env().define(f"{class_name}.__init__", symbol)

    def _collect_class_decl(self, decl: ClassDecl):
        """改进的类声明收集"""
        # 存储类定义用于后续查找
        self.class_definitions[decl.name] = decl

        # 创建类类型
        super_type = None
        if decl.super_class:
            super_type = self.current_env().lookup_type(decl.super_class)

        class_type = ClassType(decl.name, super_type)
        self.current_env().define_type(decl.name, class_type)

        # 创建类环境
        class_env = Environment(self.current_env())
        decl.class_env = class_env

        # 收集字段信息
        for field in decl.fields:
            if isinstance(field, FieldDecl):
                field_type = field.type if field.type else self.fresh_type_var(f"F_{field.name}")
                class_env.define(field.name, Symbol(field.name, field_type))

        self.push_env(class_env)

        # 收集方法和构造函数
        for method in decl.methods:
            if isinstance(method, ConstructorDecl):
                self._collect_constructor(method, decl.name)
            else:
                self._collect_function_decl(method)

        self.pop_env()

    def _collect_function_decl(self, decl: FunctionDecl):
        """收集函数声明"""
        # 推断参数类型
        param_types = []
        for param in decl.params:
            if param.type:
                param_types.append(param.type)
            else:
                # 如果没有类型注解，创建类型变量
                param_types.append(self.fresh_type_var(f"P_{param.name}"))

        # 推断返回类型
        return_type = decl.return_type if decl.return_type else self.fresh_type_var("R")

        func_type = FunctionType(param_types, return_type)

        # 处理重载
        if decl.name in self.overload_sets:
            self.overload_sets[decl.name].add_overload(func_type, decl)
        else:
            overload_set = OverloadSet(decl.name)
            overload_set.add_overload(func_type, decl)
            self.overload_sets[decl.name] = overload_set

        # 添加到环境
        symbol = Symbol(decl.name, func_type)
        self.current_env().define(decl.name, symbol)

    def _infer_field_decl(self, decl: FieldDecl):
        """推断字段声明"""
        if decl.initializer:
            init_type = self._infer_expression(decl.initializer)

            if decl.type:
                # 有类型注解，检查兼容性
                self.add_constraint(init_type, decl.type, "assignable")
                field_type = decl.type
            else:
                # 无类型注解，使用推断类型
                field_type = init_type
        else:
            if decl.type:
                field_type = decl.type
            else:
                # 无初始化器且无类型注解，创建类型变量
                field_type = self.fresh_type_var(f"Field_{decl.name}")

        # 将字段添加到当前环境（类环境）
        symbol = Symbol(decl.name, field_type)
        self.current_env().define(decl.name, symbol)
        decl.inferred_type = field_type

        print(f"推断字段 {decl.name}: {field_type}")

    def _infer_constructor_decl(self, decl: ConstructorDecl):
        """推断构造函数声明"""
        # 创建构造函数环境
        constructor_env = Environment(self.current_env())
        self.push_env(constructor_env)

        # 添加参数到环境
        param_types = []
        for param in decl.params:
            if param.type:
                param_type = param.type
            else:
                param_type = self.fresh_type_var(f"ConstructorParam_{param.name}")

            param_types.append(param_type)
            param_symbol = Symbol(param.name, param_type)
            constructor_env.define(param.name, param_symbol)

        # 推断构造函数体
        for stmt in decl.body:
            self._infer_statement(stmt)

        # 构造函数返回当前类的实例
        class_type = self.current_env().parent.parent.lookup_type(decl.class_name)
        if not class_type:
            class_type = self.fresh_type_var(f"Class_{decl.class_name}")

        constructor_type = FunctionType(param_types, class_type)
        decl.inferred_type = constructor_type

        print(f"推断构造函数 {decl.class_name}: {constructor_type}")

        self.pop_env()

    def _infer_statement(self, stmt: Statement):
        """推断语句类型"""
        if isinstance(stmt, VarDecl):
            self._infer_var_decl(stmt)
        elif isinstance(stmt, FunctionDecl):
            self._infer_function_decl(stmt)
        elif isinstance(stmt, ClassDecl):
            self._infer_class_decl(stmt)
        elif isinstance(stmt, ReturnStmt):
            self._infer_return_stmt(stmt)
        elif isinstance(stmt, Expression):
            self._infer_expression(stmt)
        elif isinstance(stmt, FieldDecl):          # ✅ 添加字段声明处理
            self._infer_field_decl(stmt)
        elif isinstance(stmt, ConstructorDecl):    # ✅ 添加构造函数处理
            self._infer_constructor_decl(stmt)
        else:
            print(f"警告: 未处理的语句类型: {type(stmt)}")

    def _infer_var_decl(self, decl: VarDecl):
        """推断变量声明"""
        if decl.initializer:
            init_type = self._infer_expression(decl.initializer)

            if decl.type_annotation:
                # 有类型注解，检查兼容性
                self.add_constraint(init_type, decl.type_annotation, "assignable")
                var_type = decl.type_annotation
            else:
                # 无类型注解，使用推断类型
                var_type = init_type
        else:
            if decl.type_annotation:
                var_type = decl.type_annotation
            else:
                # 无初始化器且无类型注解，创建类型变量
                var_type = self.fresh_type_var(f"V_{decl.name}")

        symbol = Symbol(decl.name, var_type)
        self.current_env().define(decl.name, symbol)
        decl.inferred_type = var_type

    def _infer_function_decl(self, decl: FunctionDecl):
        """推断函数声明"""
        # 创建函数环境
        func_env = Environment(self.current_env())
        self.push_env(func_env)

        # 添加参数到环境
        for i, param in enumerate(decl.params):
            symbol = self.current_env().lookup(decl.name)
            if symbol and isinstance(symbol.type, FunctionType):
                param_type = symbol.type.param_types[i]
                param_symbol = Symbol(param.name, param_type)
                func_env.define(param.name, param_symbol)

        # 推断函数体
        return_type = None
        for stmt in decl.body:
            self._infer_statement(stmt)
            if isinstance(stmt, ReturnStmt) and stmt.value:
                if return_type is None:
                    return_type = stmt.value.inferred_type
                else:
                    # 多个返回语句，需要类型一致
                    self.add_constraint(return_type, stmt.value.inferred_type)

        if return_type is None:
            return_type = self.void_type

        # 更新函数类型
        symbol = self.current_env().parent.lookup(decl.name)
        if symbol and isinstance(symbol.type, FunctionType):
            self.add_constraint(symbol.type.return_type, return_type)

        self.pop_env()

    def _infer_class_decl(self, decl: ClassDecl):
        """推断类声明"""
        class_env = decl.class_env or Environment(self.current_env())
        self.push_env(class_env)

        for field in decl.fields:
            self._infer_statement(field)

        # 推断方法
        for method in decl.methods:
            self._infer_statement(method)

        # 如果有构造函数，也要推断
        if hasattr(decl, 'constructors'):
            for constructor in decl.constructors:
                self._infer_statement(constructor)  # 这会调用 _infer_constructor_decl

        self.pop_env()

    def _infer_return_stmt(self, stmt: ReturnStmt):
        """推断返回语句"""
        if stmt.value:
            stmt.inferred_type = self._infer_expression(stmt.value)
        else:
            stmt.inferred_type = self.void_type

    def _infer_new_expr(self, expr: NewExpr, expected_type: Optional[Type] = None) -> Type:
        """推断构造函数调用"""
        # 查找类类型
        class_type = self.current_env().lookup_type(expr.class_name)
        if not class_type:
            raise Exception(f"未定义的类: {expr.class_name}")

        if not isinstance(class_type, ClassType):
            raise Exception(f"{expr.class_name} 不是类类型")

        # 推断参数类型
        arg_types = [self._infer_expression(arg) for arg in expr.args]

        # 查找匹配的构造函数
        if expr.class_name in self.constructors:
            constructors = self.constructors[expr.class_name]
            best_constructor = self._resolve_constructor_overload(constructors, arg_types)

            if best_constructor:
                # 检查参数类型匹配
                constructor_symbol = self.current_env().lookup(f"{expr.class_name}.__init__")
                if constructor_symbol and isinstance(constructor_symbol.type, FunctionType):
                    for arg_type, param_type in zip(arg_types, constructor_symbol.type.param_types):
                        self.add_constraint(arg_type, param_type, "assignable")

        expr.inferred_type = class_type
        return class_type

    def _resolve_constructor_overload(self, constructors: List[ConstructorDecl], arg_types: List[Type]) -> Optional[ConstructorDecl]:
        """解析构造函数重载"""
        candidates = []

        for constructor in constructors:
            if len(constructor.params) == len(arg_types):
                # 简化的匹配逻辑
                candidates.append(constructor)

        return candidates[0] if candidates else None

    def _infer_expression(self, expr: Expression, expected_type: Optional[Type] = None) -> Type:
        """推断表达式类型（双向类型检查）"""
        if isinstance(expr, LiteralExpr):
            expr.inferred_type = expr.literal_type
            return expr.literal_type

        elif isinstance(expr, IdentifierExpr):
            symbol = self.current_env().lookup(expr.name)
            if symbol:
                expr.inferred_type = symbol.type
                return symbol.type
            else:
                # 未声明的标识符，创建类型变量
                var_type = self.fresh_type_var(f"ID_{expr.name}")
                expr.inferred_type = var_type
                return var_type

        elif isinstance(expr, BinaryOpExpr):
            return self._infer_binary_op(expr, expected_type)

        elif isinstance(expr, CallExpr):
            return self._infer_call_expr(expr, expected_type)

        elif isinstance(expr, MemberAccessExpr):
            return self._infer_member_access(expr, expected_type)

        elif isinstance(expr, AssignmentExpr):
            return self._infer_assignment(expr, expected_type)

        elif isinstance(expr, NewExpr):
            return self._infer_new_expr(expr, expected_type)

        else:
            # 未知表达式类型
            var_type = self.fresh_type_var("UNKNOWN")
            expr.inferred_type = var_type
            return var_type

    def _infer_binary_op(self, expr: BinaryOpExpr, expected_type: Optional[Type] = None) -> Type:
        """推断二元操作表达式"""
        left_type = self._infer_expression(expr.left)
        right_type = self._infer_expression(expr.right)

        # 根据操作符决定结果类型
        if expr.operator in ['+', '-', '*', '/', '%']:
            # 算术运算符
            self.add_constraint(left_type, right_type)  # 操作数类型相同
            if expected_type:
                self.add_constraint(left_type, expected_type)
            result_type = left_type
        elif expr.operator in ['==', '!=', '<', '>', '<=', '>=']:
            # 比较运算符
            self.add_constraint(left_type, right_type)  # 操作数类型相同
            result_type = self.bool_type
        elif expr.operator in ['&&', '||']:
            # 逻辑运算符
            self.add_constraint(left_type, self.bool_type)
            self.add_constraint(right_type, self.bool_type)
            result_type = self.bool_type
        else:
            result_type = self.fresh_type_var("BINOP")

        expr.inferred_type = result_type
        return result_type

    def _infer_call_expr(self, expr: CallExpr, expected_type: Optional[Type] = None) -> Type:
        """推断函数调用表达式"""
        # 推断参数类型
        arg_types = [self._infer_expression(arg) for arg in expr.args]

        # 处理函数重载解析
        if isinstance(expr.callee, IdentifierExpr):
            func_name = expr.callee.name
            if func_name in self.overload_sets:
                # 重载解析
                best_match = self._resolve_overload(self.overload_sets[func_name], arg_types)
                if best_match:
                    expr.inferred_type = best_match.return_type
                    return best_match.return_type

        # 一般函数调用
        callee_type = self._infer_expression(expr.callee)

        if isinstance(callee_type, FunctionType):
            # 检查参数数量
            if len(arg_types) == len(callee_type.param_types):
                # 添加参数类型约束
                for arg_type, param_type in zip(arg_types, callee_type.param_types):
                    self.add_constraint(arg_type, param_type, "assignable")

            result_type = callee_type.return_type
        else:
            # 不是函数类型，可能是类型变量
            result_type = self.fresh_type_var("CALL_RESULT")
            # 创建函数类型约束
            func_type = FunctionType(arg_types, result_type)
            self.add_constraint(callee_type, func_type)

        expr.inferred_type = result_type
        return result_type

    def _infer_member_access(self, expr: MemberAccessExpr, expected_type: Optional[Type] = None) -> Type:
        """改进的成员访问推断"""
        obj_type = self._infer_expression(expr.object)
        obj_type = self._apply_substitution(obj_type)

        if isinstance(obj_type, ClassType):
            # 查找类定义
            if obj_type.name in self.class_definitions:
                class_decl = self.class_definitions[obj_type.name]

                # 查找字段
                for field in class_decl.fields:
                    if isinstance(field, FieldDecl) and field.name == expr.member:
                        member_type = field.type if field.type else self.fresh_type_var(f"F_{field.name}")
                        expr.inferred_type = member_type
                        return member_type

                # 查找方法
                for method in class_decl.methods:
                    if isinstance(method, FunctionDecl) and method.name == expr.member:
                        # 获取方法类型
                        symbol = class_decl.class_env.lookup(f"{method.name}")
                        if symbol:
                            expr.inferred_type = symbol.type
                            return symbol.type

                # 成员不存在
                raise Exception(f"类 {obj_type.name} 没有成员 {expr.member}")

        elif isinstance(obj_type, TypeVariable):
            # 对象类型是类型变量，创建约束
            member_type = self.fresh_type_var(f"MEMBER_{expr.member}")
            # 这里可以添加更复杂的约束逻辑
            expr.inferred_type = member_type
            return member_type

        else:
            raise Exception(f"无法访问类型 {obj_type} 的成员 {expr.member}")

    def _infer_assignment(self, expr: AssignmentExpr, expected_type: Optional[Type] = None) -> Type:
        """推断赋值表达式"""
        target_type = self._infer_expression(expr.target)
        value_type = self._infer_expression(expr.value, target_type)

        # 赋值兼容性约束
        self.add_constraint(value_type, target_type, "assignable")

        expr.inferred_type = target_type
        return target_type

    def _resolve_overload(self, overload_set: OverloadSet, arg_types: List[Type]) -> Optional[FunctionType]:
        """重载解析"""
        candidates = []

        for func_type in overload_set.candidates:
            if len(func_type.param_types) == len(arg_types):
                # 计算匹配度
                score = self._calculate_match_score(func_type.param_types, arg_types)
                if score >= 0:  # 可以匹配
                    candidates.append((func_type, score))

        if not candidates:
            return None

        # 选择最佳匹配
        candidates.sort(key=lambda x: x[1], reverse=True)
        return candidates[0][0]

    def _calculate_match_score(self, param_types: List[Type], arg_types: List[Type]) -> int:
        """计算匹配分数"""
        score = 0
        for param_type, arg_type in zip(param_types, arg_types):
            if param_type == arg_type:
                score += 10  # 精确匹配
            elif arg_type.is_assignable_to(param_type):
                score += 5   # 兼容匹配
            else:
                return -1    # 不匹配
        return score

    def _solve_constraints(self) -> bool:
        """求解类型约束"""
        try:
            for type1, type2, relation in self.constraints:
                if relation == "equal":
                    self._unify(type1, type2)
                elif relation == "assignable":
                    if not self._check_assignable(type1, type2):
                        return False
            return True
        except Exception as e:
            print(f"约束求解失败: {e}")
            return False

    def _unify(self, type1: Type, type2: Type):
        """统一算法"""
        type1 = self._apply_substitution(type1)
        type2 = self._apply_substitution(type2)

        if type1 == type2:
            return

        if isinstance(type1, TypeVariable):
            if self._occurs_check(type1, type2):
                raise Exception(f"循环类型约束: {type1} occurs in {type2}")
            self.substitutions[type1] = type2
            return

        if isinstance(type2, TypeVariable):
            if self._occurs_check(type2, type1):
                raise Exception(f"循环类型约束: {type2} occurs in {type1}")
            self.substitutions[type2] = type1
            return

        if isinstance(type1, FunctionType) and isinstance(type2, FunctionType):
            if len(type1.param_types) != len(type2.param_types):
                raise Exception(f"函数参数数量不匹配: {type1} vs {type2}")

            for p1, p2 in zip(type1.param_types, type2.param_types):
                self._unify(p1, p2)

            self._unify(type1.return_type, type2.return_type)
            return

        if isinstance(type1, ClassType) and isinstance(type2, ClassType):
            if type1.name != type2.name:
                raise Exception(f"类类型不匹配: {type1} vs {type2}")

            if len(type1.type_params) != len(type2.type_params):
                raise Exception(f"类型参数数量不匹配: {type1} vs {type2}")

            for t1, t2 in zip(type1.type_params, type2.type_params):
                self._unify(t1, t2)
            return

        raise Exception(f"无法统一类型: {type1} vs {type2}")

    def _check_assignable(self, from_type: Type, to_type: Type) -> bool:
        """检查赋值兼容性"""
        from_type = self._apply_substitution(from_type)
        to_type = self._apply_substitution(to_type)

        return from_type.is_assignable_to(to_type)

    def _occurs_check(self, var: TypeVariable, type: Type) -> bool:
        """occurs检查，防止无限类型"""
        type = self._apply_substitution(type)

        if var == type:
            return True

        if isinstance(type, FunctionType):
            return (any(self._occurs_check(var, p) for p in type.param_types) or
                    self._occurs_check(var, type.return_type))

        if isinstance(type, ClassType):
            return any(self._occurs_check(var, p) for p in type.type_params)

        return False

    def _apply_substitution(self, type: Type) -> Type:
        """应用类型替换"""
        if isinstance(type, TypeVariable) and type in self.substitutions:
            # 递归应用替换
            return self._apply_substitution(self.substitutions[type])

        if isinstance(type, FunctionType):
            return FunctionType(
                [self._apply_substitution(p) for p in type.param_types],
                self._apply_substitution(type.return_type)
            )

        if isinstance(type, ClassType):
            return ClassType(
                type.name,
                type.super_class,
                [self._apply_substitution(p) for p in type.type_params]
            )

        return type

    def _apply_substitutions(self, statements: List[Statement]):
        """将替换应用到AST节点"""
        def apply_to_node(node):
            if hasattr(node, 'inferred_type') and node.inferred_type:
                node.inferred_type = self._apply_substitution(node.inferred_type)

            # 递归处理子节点
            for attr_name in dir(node):
                if not attr_name.startswith('_'):
                    attr_value = getattr(node, attr_name)
                    if isinstance(attr_value, list):
                        for item in attr_value:
                            if isinstance(item, ASTNode):
                                apply_to_node(item)
                    elif isinstance(attr_value, ASTNode):
                        apply_to_node(attr_value)

        for stmt in statements:
            apply_to_node(stmt)

# ==================== 测试代码 ====================

def test_type_inference():
    """测试类型推断系统"""
    engine = TypeInferenceEngine()

    # 创建测试AST
    statements = [
        # var x = 42;
        VarDecl("x", None, LiteralExpr(42, engine.int_type)),

        # var y: string = "hello";
        VarDecl("y", engine.string_type, LiteralExpr("hello", engine.string_type)),

        # function add(a: int, b: int): int { return a + b; }
        FunctionDecl(
            "add",
            [Parameter("a", engine.int_type), Parameter("b", engine.int_type)],
            engine.int_type,
            [ReturnStmt(BinaryOpExpr(IdentifierExpr("a"), "+", IdentifierExpr("b")))]
        ),

        # var z = add(x, 10);
        VarDecl("z", None, CallExpr(IdentifierExpr("add"), [IdentifierExpr("x"), LiteralExpr(10, engine.int_type)])),
    ]

    # 执行类型推断
    success = engine.infer_program(statements)

    print(f"类型推断{'成功' if success else '失败'}")
    print("\n推断结果:")

    for stmt in statements:
        if isinstance(stmt, VarDecl):
            print(f"变量 {stmt.name}: {stmt.inferred_type}")
        elif isinstance(stmt, FunctionDecl):
            symbol = engine.current_env().lookup(stmt.name)
            if symbol:
                print(f"函数 {stmt.name}: {symbol.type}")

    print(f"\n生成的约束数量: {len(engine.constraints)}")
    print("约束详情:")
    for i, (t1, t2, rel) in enumerate(engine.constraints):
        print(f"  {i+1}. {t1} {rel} {t2}")

    print(f"\n类型替换: {len(engine.substitutions)}")
    for var, type_val in engine.substitutions.items():
        print(f"  {var} -> {type_val}")

def test_class_instances():
    """测试类实例处理"""
    engine = TypeInferenceEngine()

    # 定义Point类
    point_class = ClassDecl(
        name="Point",
        super_class=None,
        methods=[
            FunctionDecl("getX", [], engine.int_type, [
                ReturnStmt(MemberAccessExpr(IdentifierExpr("this"), "x"))
            ]),
            FunctionDecl("getY", [], engine.int_type, [
                ReturnStmt(MemberAccessExpr(IdentifierExpr("this"), "y"))
            ])
        ],
        fields=[
            FieldDecl("x", engine.int_type, None),
            FieldDecl("y", engine.int_type, None)
        ],
        constructors=[
            ConstructorDecl("Point", [
                Parameter("x", engine.int_type),
                Parameter("y", engine.int_type)
            ], [])
        ]
    )

    statements = [
        point_class,
        # var p = new Point(10, 20);
        VarDecl("p", None, NewExpr("Point", [
            LiteralExpr(10, engine.int_type),
            LiteralExpr(20, engine.int_type)
        ])),
        # var x = p.x;
        VarDecl("x", None, MemberAccessExpr(IdentifierExpr("p"), "x")),
        # var y = p.getY();
        VarDecl("y", None, CallExpr(
            MemberAccessExpr(IdentifierExpr("p"), "getY"),
            []
        ))
    ]

    success = engine.infer_program(statements)
    print(f"类实例测试: {'成功' if success else '失败'}")

    for stmt in statements:
        if isinstance(stmt, VarDecl):
            print(f"  {stmt.name}: {stmt.inferred_type}")

if __name__ == "__main__":
    # test_type_inference()
    test_class_instances()

# ==================== 高级特性扩展 ====================

class GenericTypeInference:
    """泛型类型推断扩展"""

    def __init__(self, base_engine: TypeInferenceEngine):
        self.base_engine = base_engine

    def infer_generic_call(self, func_type: FunctionType, arg_types: List[Type]) -> Optional[Dict[str, Type]]:
        """推断泛型函数调用的类型参数"""
        type_param_bindings = {}

        # 收集所有类型变量
        type_vars = self._collect_type_variables(func_type)

        # 尝试从参数推断类型参数
        for param_type, arg_type in zip(func_type.param_types, arg_types):
            self._match_type_parameters(param_type, arg_type, type_param_bindings)

        # 验证所有类型参数都被推断出来
        for var in type_vars:
            if var.name not in type_param_bindings:
                return None  # 推断失败

        return type_param_bindings

    def _collect_type_variables(self, type: Type) -> Set[TypeVariable]:
        """收集类型中的所有类型变量"""
        if isinstance(type, TypeVariable):
            return {type}
        elif isinstance(type, FunctionType):
            vars = set()
            for param in type.param_types:
                vars.update(self._collect_type_variables(param))
            vars.update(self._collect_type_variables(type.return_type))
            return vars
        elif isinstance(type, ClassType):
            vars = set()
            for param in type.type_params:
                vars.update(self._collect_type_variables(param))
            return vars
        else:
            return set()

    def _match_type_parameters(self, pattern: Type, concrete: Type, bindings: Dict[str, Type]):
        """匹配类型参数"""
        if isinstance(pattern, TypeVariable):
            if pattern.name in bindings:
                # 已有绑定，检查一致性
                if bindings[pattern.name] != concrete:
                    raise Exception(f"类型参数绑定冲突: {pattern.name}")
            else:
                bindings[pattern.name] = concrete
        elif isinstance(pattern, FunctionType) and isinstance(concrete, FunctionType):
            if len(pattern.param_types) == len(concrete.param_types):
                for p_param, c_param in zip(pattern.param_types, concrete.param_types):
                    self._match_type_parameters(p_param, c_param, bindings)
                self._match_type_parameters(pattern.return_type, concrete.return_type, bindings)
        elif isinstance(pattern, ClassType) and isinstance(concrete, ClassType):
            if pattern.name == concrete.name and len(pattern.type_params) == len(concrete.type_params):
                for p_param, c_param in zip(pattern.type_params, concrete.type_params):
                    self._match_type_parameters(p_param, c_param, bindings)

class ErrorRecoveryInference:
    """错误恢复类型推断"""

    def __init__(self, base_engine: TypeInferenceEngine):
        self.base_engine = base_engine
        self.errors: List[str] = []

    def infer_with_recovery(self, statements: List[Statement]) -> tuple[bool, List[str]]:
        """带错误恢复的类型推断"""
        self.errors.clear()

        try:
            # 尝试正常推断
            success = self.base_engine.infer_program(statements)
            if success:
                return True, []
        except Exception as e:
            self.errors.append(str(e))

        # 错误恢复：为失败的节点分配错误类型
        self._recover_from_errors(statements)

        return False, self.errors

    def _recover_from_errors(self, statements: List[Statement]):
        """从错误中恢复，继续推断其他部分"""
        error_type = PrimitiveType("ERROR")

        def recover_node(node):
            if hasattr(node, 'inferred_type') and node.inferred_type is None:
                node.inferred_type = error_type

            # 递归处理子节点
            for attr_name in dir(node):
                if not attr_name.startswith('_'):
                    attr_value = getattr(node, attr_name)
                    if isinstance(attr_value, list):
                        for item in attr_value:
                            if isinstance(item, ASTNode):
                                recover_node(item)
                    elif isinstance(attr_value, ASTNode):
                        recover_node(attr_value)

        for stmt in statements:
            recover_node(stmt)

class IncrementalInference:
    """增量类型推断"""

    def __init__(self, base_engine: TypeInferenceEngine):
        self.base_engine = base_engine
        self.dependency_graph: Dict[str, Set[str]] = {}
        self.cached_results: Dict[str, Type] = {}

    def update_declaration(self, name: str, new_decl: Statement):
        """更新声明并进行增量推断"""
        # 找到受影响的节点
        affected = self._find_affected_nodes(name)

        # 清除缓存
        for affected_name in affected:
            if affected_name in self.cached_results:
                del self.cached_results[affected_name]

        # 重新推断受影响的部分
        self._reinfer_affected(affected, new_decl)

    def _find_affected_nodes(self, changed_name: str) -> Set[str]:
        """找到受变化影响的节点"""
        affected = {changed_name}
        worklist = [changed_name]

        while worklist:
            current = worklist.pop()
            for node, deps in self.dependency_graph.items():
                if current in deps and node not in affected:
                    affected.add(node)
                    worklist.append(node)

        return affected

    def _reinfer_affected(self, affected: Set[str], new_decl: Statement):
        """重新推断受影响的节点"""
        # 这里简化实现，实际需要更复杂的逻辑
        pass

# ==================== 诊断和错误报告 ====================

@dataclass
class TypeError:
    message: str
    location: Optional[tuple] = None  # (line, column)
    suggestions: List[str] = None

class DiagnosticEngine:
    """诊断引擎"""

    def __init__(self):
        self.errors: List[TypeError] = []
        self.warnings: List[TypeError] = []

    def add_error(self, message: str, location: Optional[tuple] = None, suggestions: List[str] = None):
        """添加错误"""
        error = TypeError(message, location, suggestions or [])
        self.errors.append(error)

    def add_warning(self, message: str, location: Optional[tuple] = None):
        """添加警告"""
        warning = TypeError(message, location)
        self.warnings.append(warning)

    def generate_suggestions(self, expected: Type, actual: Type) -> List[str]:
        """生成类型错误的修复建议"""
        suggestions = []

        if isinstance(expected, PrimitiveType) and isinstance(actual, PrimitiveType):
            if expected.name == "int" and actual.name == "string":
                suggestions.append("使用 parseInt() 将字符串转换为整数")
            elif expected.name == "string" and actual.name == "int":
                suggestions.append("使用 toString() 将整数转换为字符串")

        if isinstance(expected, ClassType) and isinstance(actual, ClassType):
            if actual.is_assignable_to(expected):
                suggestions.append(f"类型 {actual} 可以向上转型为 {expected}")

        return suggestions

    def format_diagnostics(self) -> str:
        """格式化诊断信息"""
        result = []

        if self.errors:
            result.append("错误:")
            for error in self.errors:
                location_str = f" at {error.location}" if error.location else ""
                result.append(f"  {error.message}{location_str}")
                for suggestion in error.suggestions or []:
                    result.append(f"    建议: {suggestion}")

        if self.warnings:
            result.append("警告:")
            for warning in self.warnings:
                location_str = f" at {warning.location}" if warning.location else ""
                result.append(f"  {warning.message}{location_str}")

        return "\n".join(result)

# ==================== 完整测试示例 ====================

def comprehensive_test():
    """全面测试类型推断系统"""
    print("=== 类型推断系统综合测试 ===\n")

    engine = TypeInferenceEngine()
    diagnostic = DiagnosticEngine()

    # 测试1: 基本类型推断
    print("测试1: 基本类型推断")
    statements1 = [
        VarDecl("x", None, None),
        VarDecl("y", None, BinaryOpExpr(IdentifierExpr("x"), "+", LiteralExpr(10, engine.int_type)))
    ]

    success1 = engine.infer_program(statements1)
    print(f"结果: {'成功' if success1 else '失败'}")
    for stmt in statements1:
        if isinstance(stmt, VarDecl):
            print(f"  {stmt.name}: {stmt.inferred_type}")
    print()

    # 测试2: 函数重载
    print("测试2: 函数重载")
    engine2 = TypeInferenceEngine()

    # 定义重载函数
    func1 = FunctionDecl("print", [Parameter("x", engine2.int_type)], engine2.void_type, [])
    func2 = FunctionDecl("print", [Parameter("x", engine2.string_type)], engine2.void_type, [])

    statements2 = [
        func1, func2,
        VarDecl("result1", None, CallExpr(IdentifierExpr("print"), [LiteralExpr(42, engine2.int_type)])),
        VarDecl("result2", None, CallExpr(IdentifierExpr("print"), [LiteralExpr("hello", engine2.string_type)])),
    ]

    success2 = engine2.infer_program(statements2)
    print(f"结果: {'成功' if success2 else '失败'}")
    if "print" in engine2.overload_sets:
        print(f"  print函数有 {len(engine2.overload_sets['print'].candidates)} 个重载")
    print()

    # 测试3: 类型错误处理
    print("测试3: 类型错误处理")
    engine3 = TypeInferenceEngine()
    error_recovery = ErrorRecoveryInference(engine3)

    statements3 = [
        VarDecl("x", engine3.int_type, LiteralExpr("hello", engine3.string_type)),  # 类型错误
        VarDecl("y", None, LiteralExpr(42, engine3.int_type)),  # 正常
    ]

    success3, errors = error_recovery.infer_with_recovery(statements3)
    print(f"结果: {'成功' if success3 else '失败'}")
    if errors:
        print("错误信息:")
        for error in errors:
            print(f"  {error}")
    print()

    print("=== 测试完成 ===")

if __name__ == "__main__":
    comprehensive_test()
