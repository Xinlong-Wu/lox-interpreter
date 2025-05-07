import lit
import lit.formats
from lit.llvm import llvm_config
from lit.llvm.subst import FindTool
from lit.llvm.subst import ToolSubst
import os

config.name = 'LoxTest'
# 1. 设置测试格式为 shell 测试
config.test_format = lit.formats.ShTest()

# 2. 设置测试目录（当前目录）
config.test_source_root = os.path.dirname("/workspaces/lox-interpreter/tests/")

config.suffixes = ['.lox']

config.excludes = [
    "lit.cfg.py",
    "lit.site.cfg.py",
]

llvm_config.use_default_substitutions()

tool_dirs = [
    config.lox_build_path+"/bin",
]

tools = [
    config.lox_bin_name,
]

llvm_config.add_tool_substitutions(tools, tool_dirs)
config.substitutions.append(("%lox", os.path.join(config.lox_build_path, "bin", config.lox_bin_name)))
config.substitutions.append(("%parser", os.path.join(config.lox_build_path, "bin", "lox-parser")))
