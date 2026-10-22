#!/usr/bin/env python3
"""Export original multiplayer metadata and cold publication destinations.

The runtime never reads this executable. File-backed Juggernaut metadata and
loader-zeroed mode globals are bounded by their actual original consumers.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

# Every row is an entire individual source owner, never a section/page import.
REGIONS = ((0x48a138, 4, False), (0x45fce0, 204, True),
           (0x51df9c, 4, False), (0x52d89c, 4, False),
           (0x52d8a0, 4, False), (0x52d8a4, 4, False),
           (0x515dc8, 2, False), (0x515dd0, 16, False),
           (0x46a0d8, 4, True), (0x466164, 10, True), (0x466170, 16, True),
           (0x466180, 8, True), (0x466188, 1, True), (0x248e10, 36, True),
           (0x4772a8, 1024, False), (0x44a834, 52, True), (0x2446b0, 36, True),
           (0x4486a1, 1, True), (0x45f918, 15, True), (0x45f904, 18, True), (0x45f8f0, 18, True),
           (0x44e9f0, 40, True))
CONSUMERS = (0x1981ea, 0x2c6178, 0x2c6f67, 0x2c912f,
             0x2c6f81, 0x2c6fc3, 0x2c6fdb, 0x2c6ff3, 0x2c7c86,
             0x248700, 0x24871d, 0x248727, 0x248731, 0x248cff, 0x17f80b, 0xd7bb7, 0x2445d3,
             0x18bae4, 0x18bb01, 0x18bb1e, 0x18bb3b, 0x22839e, 0x228417,
             0x15c62a, 0x15c69e, 0x15c6bf, 0x15c6e9, 0x15c700)


def sound_class_regions(image):
    #22839E..228417 iterates precisely54 original name pointers. Include
    # each complete NUL-terminated name once, reusing the selectors above.
    pointers = struct.unpack('<54I', image.read(0x47c7a8, 216))
    result = [(0x47c7a8, 216, False)]
    present = {address for address, _, _ in REGIONS}
    for address in sorted(set(pointers) - present):
        raw = image.read(address, 128)
        size = raw.index(0) + 1
        result.append((address, size, True))
    return tuple(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fixture', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--runtime-sound-classes', type=Path)
    args = parser.parse_args()
    assert Path(os.environ['HALO2_TEST_SCRATCH']).is_dir()
    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    regions = REGIONS + sound_class_regions(image)
    assert struct.unpack('<I', image.read(0x47a2f8, 4))[0] == 0x48a138
    assert struct.unpack('<I', image.read(0x48a138, 4))[0] == 0x45fce0
    code = _Code(image, '.text')
    consumers = []
    for pc in CONSUMERS:
        instruction = code.at(pc)
        consumers.append(dict(pc=f'{pc:08x}', bytes=instruction.bytes.hex(),
                              operation=instruction.mnemonic + ' ' + instruction.op_str))
    # The initializer accepts precisely marker groups 11..18, deduplicates
    # them, and stores eight at most two-byte group indices in 515DD0.
    assert code.at(0x2c6f9f).op_str == 'cx, 0xb'
    assert code.at(0x2c6fa5).op_str == 'cx, 0x12'
    assert code.at(0x2c6fdb).size == 8
    for address, size, read_only in REGIONS[9:13]:
        raw = image.read(address, size)
        assert read_only and raw[-1] == 255 and 255 not in raw[:-1]
    rows = []
    lines = ['// Generated from authenticated original TU1.5 data.',
             '// Reference SHA256: ' + NEW_PIN]
    sound_lines = lines.copy()
    for index, (address, size, read_only) in enumerate(regions):
        section = next(s for s in image.sections if s.va <= address < address + size <= s.va + s.vsize)
        raw = image.read(address, size)
        zero = address >= section.va + section.rsize
        if 2 <= index < 8:
            assert zero and not any(raw) and section.name == '.data'
        assert bool(section.flags & 1) != read_only
        rows.append(dict(address=address, bytes=size, read_only=read_only,
                         loader_zeroed=zero, section=section.name,
                         sha256=hashlib.sha256(raw).hexdigest()))
        encoded = [f'constexpr std::array<std::byte,{size}> original_mp_data_{index}{{']
        for offset in range(0, size, 16):
            encoded.append('    ' + ','.join(f'std::byte{{0x{v:02x}}}' for v in raw[offset:offset+16]) + ',')
        encoded.append('};')
        lines.extend(encoded)
        if index >= len(REGIONS):
            sound_lines.extend(encoded)
    methods = struct.unpack('<51I', image.read(0x45fce0, 204))
    assert all(code.inside(p) and code.at(p) for p in methods)
    lines += ['struct OriginalMultiplayerDataRegion {',
              '    std::uint32_t address; bool read_only; std::span<const std::byte> bytes;',
              '};', 'constexpr std::array original_multiplayer_data_regions{']
    for index, (address, size, read_only) in enumerate(regions):
        lines.append(f'    OriginalMultiplayerDataRegion{{0x{address:x}u,{str(read_only).lower()},original_mp_data_{index}}},')
    lines.append('};')
    args.fixture.write_text('\n'.join(lines) + '\n')
    if args.runtime_sound_classes:
        sound_lines += ['struct NativeMultiplayerSoundClassRegionTu15 {',
                        '    std::uint32_t address; bool read_only; std::span<const std::byte> bytes;',
                        '};', 'constexpr std::array multiplayer_sound_class_regions{']
        for index in range(len(REGIONS), len(regions)):
            address, _, read_only = regions[index]
            sound_lines.append(f'    NativeMultiplayerSoundClassRegionTu15{{0x{address:x}u,{str(read_only).lower()},original_mp_data_{index}}},')
        sound_lines.append('};')
        args.runtime_sound_classes.write_text('\n'.join(sound_lines) + '\n')
    report = dict(passed=True, original_sha256=NEW_PIN, exporter_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  regions=rows, consumers=consumers, total_bytes=sum(row['bytes'] for row in rows),
                  fixture_sha256=hashlib.sha256(args.fixture.read_bytes()).hexdigest(),
                  runtime_executable_input=False, supplied_gameplay_results=[], scope=__doc__)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, regions=len(rows), bytes=report['total_bytes'])))


if __name__ == '__main__':
    main()
