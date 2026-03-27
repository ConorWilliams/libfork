import re
import os

files = [f for f in os.listdir("src/core") if f.endswith(".cxx") and f != "core.cxx"]
deps = {}

for f in files:
    partition = f.split(".")[0]
    with open(f"src/core/{f}", "r") as file:
        content = file.read()
        imports = re.findall(r"import :([^;]+);", content)
        deps[partition] = imports

print("digraph \"libfork.core\" {")
print("  node [shape=box];")
for p, i in sorted(deps.items()):
    for dep in sorted(i):
        print(f"  \"{p}\" -> \"{dep}\";")
print("}")
