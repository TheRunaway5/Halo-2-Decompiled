#!/usr/bin/env python3
"""Keep host engine tag-directory access inside the shared tag module."""
from pathlib import Path
import re
import sys


def main():
    root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]
    # These format loaders construct the directory. Original runtime adapters
    # operate on literal source memory, not this host directory container.
    loaders = {root / "src/assets/cache.cpp", root / "src/ui/vista/vista_cache.cpp",
               root / "src/assets/mcc_cache.cpp", root / "src/rendering/mcc_native_shaders.cpp"}
    forbidden = re.compile(r"\btags_\b|(?:\.|->)tags\s*\(")
    failures = []
    checked = 0
    for path in sorted((root / "src").rglob("*.cpp")):
        if path in loaders or path.parent == root / "src/tags":
            continue
        checked += 1
        for line, text in enumerate(path.read_text().splitlines(), 1):
            if forbidden.search(text):
                failures.append(f"{path.relative_to(root)}:{line}: use query_tag/select_tags or owning TagSystem views")
    if failures:
        print("\n".join(failures), file=sys.stderr)
        return 1
    print(f"{checked} engine source files use the shared tag directory interface")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
