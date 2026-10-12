#!/usr/bin/env python3
"""Retain the actual pre-call world memory from the unchanged original schedule.

This is private oracle input, never a production runtime resource or a recorded
native result. The ordinary captured world prefix must remain byte-identical.
"""
import argparse
import hashlib
import json
import sys
import zlib
from pathlib import Path
from unicorn import x86_const as R
import oracle_physics_world_event_runtime_20261011 as original
from oracle_smg_firing_1_1 import Xbe, PIN

ROOT = original.ROOT
PARENT = ROOT / 'analysis/multiplayer/sandbox-source/physics-world-event-runtime-20261011/disc-original-v5/fixture.json'
PARENT_PIN = '203e132fa7ccc83744682ba113f7d575366f6164cd38bc7e1327b28a8d4d033c'


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    assert sha(PARENT) == PARENT_PIN
    tool_pin = sha(__file__)
    source_tool = Path(original.__file__)
    source_tool_pin = sha(source_tool)
    image_path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(image_path) == PIN
    image = Xbe(image_path)
    text = image.section('.text')
    args.output.mkdir(parents=True, exist_ok=False)
    memory_path = args.output / 'memory'
    memory_path.mkdir()
    captured = []
    ParentTrace = original.Trace

    class CheckpointTrace(ParentTrace):
        def hook(self, g, address, size, user):
            if self.slot_ready and address == 0x317650:
                assert not captured, 'More than one retained world entry needs a separate checkpoint'
                # Do not silently preserve patched instructions as an original
                # executable oracle. Mutable data is retained as actual input.
                assert bytes(g.mem_read(text.va, text.rsize)) == image.section_bytes(text)
                record = dict(entry=address, entry_abi=original.captured_abi(g), regions=[],
                    gdtr=list(g.reg_read(R.UC_X86_REG_GDTR)), ldtr=list(g.reg_read(R.UC_X86_REG_LDTR)),
                    segment_selectors={n: g.reg_read(getattr(R, 'UC_X86_REG_' + n))
                        for n in ('CS', 'DS', 'ES', 'SS', 'FS', 'GS')})
                for index, (start, end, protection) in enumerate(g.mem_regions()):
                    raw = bytes(g.mem_read(start, end - start + 1))
                    compressed = zlib.compress(raw, level=1)
                    path = memory_path / f'{index:04d}.zlib'
                    path.write_bytes(compressed)
                    record['regions'].append(dict(start=start, size=len(raw), protection=protection,
                        file=path.name, sha256=hashlib.sha256(raw).hexdigest(),
                        compressed_sha256=sha(path), compressed_bytes=len(compressed)))
                captured.append(record)
            super().hook(g, address, size, user)

    original.Trace = CheckpointTrace
    old_argv = sys.argv
    try:
        sys.argv = [str(source_tool), '--output', str(args.output / 'measured')]
        original.main()
    finally:
        sys.argv = old_argv
        original.Trace = ParentTrace
    measured = args.output / 'measured/fixture.json'
    assert sha(measured) == PARENT_PIN, 'Read-only checkpoint capture changed the original measurement'
    assert len(captured) == 1
    parent = json.loads(PARENT.read_text())
    assert captured[0]['entry_abi'] == parent['cases'][0]['entry_abi']
    assert sha(source_tool) == source_tool_pin and sha(__file__) == tool_pin
    report = dict(schema=1, tool_sha256=tool_pin, original_capture_tool_sha256=source_tool_pin,
        original_sha256=PIN, parent_fixture_sha256=PARENT_PIN, correspondence_sha256=original.PIN,
        captured_entry=captured[0], unchanged_original_prefix_sha256=sha(measured),
        original_text_unpatched=True, native_expected_data_used=False,
        production_checkpoint_replay=False, live_gameplay_proof=False,
        scope='Full actual entry memory from the retained original factory/player/world schedule; private original-execution input only.')
    path = args.output / 'checkpoint.json'
    path.write_text(json.dumps(report, sort_keys=True, indent=2) + '\n')
    regions = captured[0]['regions']
    print(json.dumps(dict(checkpoint_sha256=sha(path), regions=len(regions),
        raw_bytes=sum(r['size'] for r in regions), compressed_bytes=sum(r['compressed_bytes'] for r in regions))))


if __name__ == '__main__':
    main()
