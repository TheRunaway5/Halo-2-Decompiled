#!/usr/bin/env python3
"""Check complete original stream-construction and effect-routing CPU dependencies."""
import argparse
from pathlib import Path
import research_animation_selection_u5 as authority


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();authority.OWNERS=(0x220520,0x2b4970,0x222ca0,0x2215f0)
    authority.BOUNDARIES=();authority.run(args.output)
    authority.save(args.output/'invocation.json',authority.encoded(dict(
        schema='sound-driver-stream-correspondence-v1',tool_sha256=authority.sha(__file__),
        authority_tool_sha256=authority.sha(authority.__file__),
        report_sha256=authority.sha(args.output/'report.json'),source_platform_returns=[],
        scope='Static complete-body correspondence only; source stream/device creation and retained voice state are unproved.')))


if __name__=='__main__':main()
