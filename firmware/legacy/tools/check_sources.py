#!/usr/bin/env python3
"""Check reconstruction syntax and known layouts, without linking or execution."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import json
import os
import re
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
COMPILER = shlex.split(os.environ.get("CC", "gcc"))
FLAGS = [
    "-m32", "-std=gnu11", "-ffreestanding", "-fno-builtin", "-fsyntax-only",
    "-Werror=implicit-function-declaration", "-Wno-error=int-conversion",
    "-Wno-error=incompatible-pointer-types", "-Iinclude",
]


def check_file(path):
    relative = str(path.relative_to(ROOT))
    result = subprocess.run(COMPILER + FLAGS + [relative], cwd=ROOT,
                            capture_output=True, text=True)
    return {"file": relative, "exit_code": result.returncode,
            "diagnostics": result.stdout + result.stderr}


def main():
    sources = sorted((ROOT / "src").rglob("*.c"))
    with ThreadPoolExecutor(max_workers=min(8, os.cpu_count() or 1)) as pool:
        results = list(pool.map(check_file, sources))

    # These checks validate only established offsets, not the meaning of fields.
    layout_source = '''
#include "pico/core_layouts.h"
#include "pico/pid_controller_layout.h"
#include <stddef.h>
_Static_assert(sizeof(void *) == 4, "32-bit pointer model required");
_Static_assert(sizeof(PicoMenu) == 92, "menu size");
_Static_assert(offsetof(PicoMenu, count) == 88, "menu count");
_Static_assert(sizeof(PicoTextScreen) == 28, "text screen size");
_Static_assert(offsetof(PicoTextScreen, lines) == 4, "text lines");
_Static_assert(sizeof(PicoScreenState) == 68, "screen state size");
_Static_assert(sizeof(PicoProgramContext) == 88, "program size");
_Static_assert(offsetof(PicoProgramContext, actions) == 80, "program actions");
_Static_assert(sizeof(PicoEncoder) == 44, "encoder size");
_Static_assert(offsetof(PicoEncoder, position) == 32, "encoder position");
_Static_assert(sizeof(PicoPageCarousel) == 76, "carousel size");
'''
    layout = subprocess.run(COMPILER + FLAGS + ["-x", "c", "-"], cwd=ROOT,
                            input=layout_source, capture_output=True, text=True)
    results.append({"file": "<known layout assertions>",
                    "exit_code": layout.returncode,
                    "diagnostics": layout.stdout + layout.stderr})

    functions = json.loads((ROOT / "analysis/function_map.json").read_text())
    addresses = [item["address"] for item in functions]
    names = [item["name"] for item in functions]
    structure_ok = (len(functions) == 948 and len(set(addresses)) == 948
                    and len(set(names)) == 948
                    and all((ROOT / item["file"]).is_file() for item in functions))
    definition_issues = []
    for item in functions:
        source = (ROOT / item["file"]).read_text()
        pattern = (r"(?m)^[A-Za-z_][A-Za-z_\d \t\n*]*\b"
                   + re.escape(item["name"]) + r"\s*\([^;{}]*\)\s*\{")
        count = len(re.findall(pattern, source))
        if count != 1:
            definition_issues.append({"name": item["name"], "definitions": count})
    structure_ok = structure_ok and not definition_issues
    failed = [item for item in results if item["exit_code"]]
    warning_count = sum(item["diagnostics"].count("warning:") for item in results)
    version = subprocess.run(COMPILER + ["--version"], capture_output=True,
                             text=True).stdout.splitlines()[0]
    report = {
        "scope": "Syntax, declarations, known layouts and function-index coverage only",
        "limitations": "No linking, execution, behavioral equivalence or hardware validation",
        "compiler": version, "flags": FLAGS, "source_files": len(sources),
        "functions_indexed": len(functions), "function_index_valid": structure_ok,
        "definition_issues": definition_issues,
        "warning_count": warning_count, "failed_checks": len(failed), "results": results,
    }
    (ROOT / "analysis/syntax_check.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"{len(sources)} source files; {len(failed)} failed syntax/layout checks; "
          f"{warning_count} compiler warnings retained.")
    print(f"948-function index: {'PASS' if structure_ok else 'FAIL'}")
    print("Diagnostic details: analysis/syntax_check.json")
    print("This does not establish executable firmware correctness.")
    for item in failed:
        print(item["file"] + "\n" + item["diagnostics"], file=sys.stderr)
    return 1 if failed or not structure_ok else 0


if __name__ == "__main__":
    raise SystemExit(main())
