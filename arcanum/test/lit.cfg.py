## \file
## lit.cfg.py - Lit configuration for Arcanum tests.

import os
import lit.formats
import lit.util
import lit.llvm

lit.llvm.initialize(lit_config, config)
from lit.llvm import llvm_config

config.name = "Arcanum"
config.test_format = lit.formats.ShTest(preamble_commands=[])
config.suffixes = [".mlir"]
config.test_source_root = os.path.dirname(__file__)
config.test_exec_root = os.path.join(
    config.arcanum_obj_root, "test")

config.substitutions.append(
    ("%arcanum-opt",
     os.path.join(config.arcanum_tools_dir, "arcanum-opt")))

tool_dirs = [config.arcanum_tools_dir, config.llvm_tools_dir]
tools = ["FileCheck"]
llvm_config.add_tool_substitutions(tools, tool_dirs)
