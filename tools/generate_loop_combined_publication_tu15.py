#!/usr/bin/env python3
"""Recover the closed421-instruction original combined-loop gain publisher.

Generate into tools/run_test.py scratch; preserve the existing independent
353-instruction gain reference and its historical admission cohort unchanged.
"""
import hashlib
import json
import os
from pathlib import Path
import sys
import generate_sound_gain_tu15 as scalar


def main():
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    def argument(name):return Path(sys.argv[sys.argv.index(name)+1]).resolve()
    output,manifest=argument('--output'),argument('--manifest')
    assert output.is_relative_to(scratch) and manifest.is_relative_to(scratch)
    scalar.ENTRIES={**scalar.ENTRIES,0x2216d0:68}
    scalar.CALLS={**scalar.CALLS,0x221764:0x220200,0x22176e:0x127cf0}
    scalar.main()
    source=output.read_text().replace('source_sound_gain_tu15','source_loop_combined_publication_tu15')
    source=source.replace('source_run_sound_gain_tu15','source_run_loop_combined_publication_tu15')
    lines=source.splitlines()
    matches=[n for n,line in enumerate(lines) if line.startswith('bool source_loop_combined_publication_tu15_owns_entry(')]
    assert len(matches)==1
    lines[matches[0]]='bool source_loop_combined_publication_tu15_owns_entry(std::uint32_t p) {return p==0x2216d0u;}'
    output.write_text('\n'.join(lines)+'\n')
    result=json.loads(manifest.read_text())
    result.update(configuration_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scalar_configuration_tool_sha256=hashlib.sha256(Path(scalar.__file__).read_bytes()).hexdigest(),
        generated_cpp_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),
        complete_original_bodies=8,complete_original_instructions=421,
        public_entries=[0x2216d0],production_integration_or_behavior_verified=False)
    manifest.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(bodies=8,instructions=421,sha256=result['generated_cpp_sha256'])),flush=True)


if __name__=='__main__':main()
