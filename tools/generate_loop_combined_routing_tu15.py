#!/usr/bin/env python3
"""Recover complete original2B9450 and its closed12DB00 normalization callee.

Run through tools/run_test.py. Generated source/manifests belong to scratch;
the retained reference is independent of the semantic native implementation.
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
    scalar.ENTRIES={0x2b9450:178,0x12db00:34}
    scalar.CALLS={pc:0x12db00 for pc in
        (0x2b949d,0x2b94e5,0x2b952d,0x2b9577,0x2b95e5,0x2b963b,0x2b9681)}
    scalar.main()
    source=output.read_text().replace('source_sound_gain_tu15','source_loop_combined_routing_tu15')
    source=source.replace('source_run_sound_gain_tu15','source_run_loop_combined_routing_tu15')
    lines=source.splitlines()
    matches=[n for n,line in enumerate(lines) if line.startswith('bool source_loop_combined_routing_tu15_owns_entry(')]
    assert len(matches)==1
    lines[matches[0]]='bool source_loop_combined_routing_tu15_owns_entry(std::uint32_t p) {return p==0x2b9450u;}'
    output.write_text('\n'.join(lines)+'\n')
    result=json.loads(manifest.read_text())
    result.update(configuration_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        scalar_configuration_tool_sha256=hashlib.sha256(Path(scalar.__file__).read_bytes()).hexdigest(),
        generated_cpp_sha256=hashlib.sha256(output.read_bytes()).hexdigest(),
        complete_original_bodies=2,complete_original_instructions=212,
        public_entries=[0x2b9450],production_integration_or_behavior_verified=False)
    manifest.write_text(json.dumps(result,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(bodies=2,instructions=212,sha256=result['generated_cpp_sha256'])),flush=True)


if __name__=='__main__':main()
