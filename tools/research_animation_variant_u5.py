#!/usr/bin/env python3
"""Full original animation variant bodies and constant correspondence to u5."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as authority

OWNERS = (0x1ceda0, 0x1e1020, 0x1e0fc0, 0x1de890, 0x1de9b0)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    authority.OWNERS = OWNERS
    authority.BOUNDARIES = ()
    authority.run(args.output)
    authority.save(args.output/'invocation.json', authority.encoded(dict(
        controlling_tool_sha256=authority.sha(__file__),
        reused_authority_tool_sha256=authority.sha(authority.__file__),
        selected_owners=[f'{entry:08x}' for entry in OWNERS],
        overrides='Read-only original-body comparator owner list only; no executable, source, input or return patched.')))


if __name__ == '__main__':
    main()
