#!/usr/bin/env python3
"""Check complete remaining original driver helper bodies in both owned images."""
import argparse
import json
from pathlib import Path
import research_animation_selection_u5 as authority

OWNERS = (0x2228a0, 0x192e50, 0x2235a0, 0x2215f0, 0x192d30)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    authority.OWNERS = OWNERS
    authority.BOUNDARIES = ()
    authority.run(args.output)
    report = json.loads((args.output/'report.json').read_text())
    authority.save(args.output/'invocation.json', authority.encoded(dict(
        schema='sound-driver-remaining-full-body-correspondence-v1',
        tool_sha256=authority.sha(__file__), authority_tool_sha256=authority.sha(authority.__file__),
        report_sha256=authority.sha(args.output/'report.json'), owners=len(report['owners']),
        source_instruction_patches=[], source_platform_returns=[],
        scope='Static complete-body correspondence only; no device/DSP/listener/voice operation supplied.')))


if __name__ == '__main__':
    main()
