#!/usr/bin/env python3
"""Compare the recovered DSP payloads with an unchanged TU1.5 live upload.

The optional native command also checks the decoder's declared CPU fixtures.
This audits program recovery; it does not prove native DSP/PCM, full cutscene
coverage, audio timing, or physical-console equivalence. Use the match memory
guard and preserve each output directory.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
IMAGE = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def audit(capture, decoded, observer):
    image = Xbe(IMAGE)
    assert hashlib.sha256(image.data).hexdigest() == PIN
    witness = json.loads((capture / 'report.json').read_text())
    fixture = json.loads((decoded / 'report.json').read_text())
    assert witness['xbe_sha256'] == fixture['xbe_sha256'] == PIN
    assert witness['observer_sha256'] == sha(observer)
    assert witness['decoder_report_sha256'] == sha(decoded / 'report.json')
    for key in ('original_code_patches', 'original_memory_writes',
                'original_register_writes', 'supplied_callee_returns'):
        assert witness[key] == [], key
    assert fixture['instruction_patches'] == fixture['supplied_external_results'] == []
    assert witness['stable_readback']
    assert witness['original_constructor_sha256'] == hashlib.sha256(image.read(0x1972c0, 0x120)).hexdigest()
    section = next(s for s in image.sections if s.name == 'DSPImage')
    resource = image.read(section.va, section.vsize)
    assert hashlib.sha256(resource).hexdigest() == fixture['dsp_section_sha256']
    program_words = struct.unpack_from('<I', resource, 0x804)[0]
    assert witness['program_words'] == program_words
    regions = {}
    for region in witness['regions']:
        name = region['name']
        assert name in ('gp_p_loaded', 'gp_x', 'gp_mix', 'gp_y') and name not in regions
        raw = (capture / (name + '.bin')).read_bytes()
        assert len(raw) == region['words'] * 4
        assert hashlib.sha256(raw).hexdigest() == region['sha256'], name
        assert all(v <= 0xffffff for v in struct.unpack('<' + 'I' * region['words'], raw))
        regions[name] = raw
    assert set(regions) == {'gp_p_loaded', 'gp_x', 'gp_mix', 'gp_y'}
    program = regions['gp_p_loaded']
    opcode, entry_word = struct.unpack_from('<2I', program, 0x37 * 4)
    assert opcode == 0x0bf080 and entry_word == witness['program_entry_word'] == 0x171
    assert len(program) == (entry_word + program_words) * 4
    control_program = program[:entry_word * 4]
    control_file_offset = image.data.find(control_program)
    assert control_file_offset >= 0, 'Uploaded GP control program is absent from the original executable'
    assert image.data.find(control_program, control_file_offset + 1) == -1, 'Ambiguous original GP control program'
    control_section = next(s for s in image.sections
                           if s.raw <= control_file_offset and
                           control_file_offset + len(control_program) <= s.raw + s.rsize)
    control_address = control_section.va + control_file_offset - control_section.raw
    descriptor = (capture / 'descriptor.bin').read_bytes()
    count = struct.unpack_from('<I', descriptor)[0]
    assert count == len(fixture['effects']) == len(witness['effects']) == 15
    assert len(descriptor) == 8 + 32 * count
    cursor = entry_word * 4
    rows, flags, scratch_mappings = [], [], []
    for index, (expected, observed) in enumerate(zip(fixture['effects'], witness['effects'])):
        assert expected['index'] == observed['index'] == index
        code, size, state, state_size, *_ = struct.unpack_from('<8I', descriptor, 8 + index * 32)
        raw = (decoded / f'effect-{index:02d}.bin').read_bytes()
        assert len(raw) == size == expected['bytes'] == observed['size']
        assert hashlib.sha256(raw).hexdigest() == expected['sha256'], f'Decoded effect {index} hash differs'
        assert code == observed['descriptor_address'] == 0xfe83a000 + cursor + 8
        assert observed['program_address'] == 0xfe83a000 + cursor
        uploaded = program[cursor:cursor + size]
        if uploaded != raw:
            offset = next(i for i, (a, b) in enumerate(zip(raw, uploaded)) if a != b)
            raise ValueError(f'Uploaded effect {index} differs at GP byte 0x{0xfe83a000 + cursor + offset:08x}: '
                             f'original decoder 0x{raw[offset]:02x}, upload 0x{uploaded[offset]:02x}')
        assert hashlib.sha256(uploaded).hexdigest() == observed['sha256'], f'Uploaded effect {index} hash differs'
        assert (capture / f'effect-{index:02d}-program.bin').read_bytes() == raw, f'Retained effect {index} differs'
        state_offset = state - 0xfe830000
        assert 0 <= state_offset < state_offset + state_size <= 0xc00 * 4
        flags.append(struct.unpack_from('<I', regions['gp_x'], state_offset + 16)[0])
        scratch_offset, scratch_size = struct.unpack_from('<2I', regions['gp_x'], state_offset)
        scratch_address, descriptor_scratch_size = struct.unpack_from('<2I', descriptor, 8 + index * 32 + 24)
        assert scratch_size == descriptor_scratch_size
        scratch_mappings.append((scratch_address - scratch_offset, scratch_offset + scratch_size))
        rows.append(dict(index=index, bytes=size, program_address=observed['program_address'],
                         sha256=expected['sha256']))
        cursor += size
    original_footer = resource[0x818 + cursor - entry_word * 4:0x818 + program_words * 4]
    assert program[cursor:] == original_footer == b'\x0c\0\0\0'
    assert witness['original_footer_hex'] == original_footer.hex()
    assert flags == witness['effect_flags']
    assert witness['all_effects_initialized'] == all(flag & 1 for flag in flags)
    assert all(flag & 1 for flag in flags), 'Capture stopped before all effects initialized'
    scratch_verified = False
    if 'scratch' in witness:
        scratch = (capture / 'gp_scratch.bin').read_bytes()
        metadata = witness['scratch']
        assert len({base for base, _ in scratch_mappings}) == 1
        assert metadata.get('mapping') == 'apu_scatter_gather', 'Scratch prefix needs the actual APU mapping'
        assert metadata['effect_cpu_offset_bias'] == scratch_mappings[0][0]
        assert metadata['bytes'] == len(scratch) == max(end for _, end in scratch_mappings)
        assert hashlib.sha256(scratch).hexdigest() == metadata['sha256']
        assert metadata['descriptor_mapping_verified'] and metadata['stable_readback']
        table = (capture / 'gp_scratch_sg_table.bin').read_bytes()
        assert hashlib.sha256(table).hexdigest() == metadata['table_sha256']
        assert len(table) == len(metadata['pages']) * 8
        assert [list(struct.unpack_from('<2I', table, i)) for i in range(0, len(table), 8)] == metadata['pages']
        assert len(metadata['pages']) == (len(scratch) + 4095) // 4096 <= metadata['max_sge'] + 1
        assert scratch[0x800:0x818] == regions['gp_x'][:0x18]
        assert scratch[:entry_word * 4] == control_program
        assert scratch[0x818:0x818 + program_words * 4] == program[entry_word * 4:]
        scratch_verified = True
    encoder_capture_verified = False
    shared_encoder_pages = []
    if 'encoder' in witness and witness['encoder']['enabled']:
        encoder = witness['encoder']
        bootstrap = (capture / 'ep_bootstrap_program.bin').read_bytes()
        assert len(bootstrap) == encoder['bootstrap_program']['words'] * 4 == 0x2000
        assert hashlib.sha256(bootstrap).hexdigest() == encoder['bootstrap_program']['sha256']
        assert all(v <= 0xffffff for v in struct.unpack('<2048I', bootstrap))
        metadata = encoder['scratch']
        ep_scratch = (capture / 'ep_scratch.bin').read_bytes()
        ep_table = (capture / 'ep_scratch_sg_table.bin').read_bytes()
        assert hashlib.sha256(ep_scratch).hexdigest() == metadata['sha256']
        assert hashlib.sha256(ep_table).hexdigest() == metadata['table_sha256']
        assert len(ep_table) == (metadata['max_sge'] + 1) * 8
        pages = [list(struct.unpack_from('<2I', ep_table, i)) for i in range(0, len(ep_table), 8)]
        assert pages == metadata['pages']
        populated = metadata['mapped_page_count']
        assert 0 < populated <= len(pages) and len(ep_scratch) == metadata['bytes'] == populated * 4096
        assert all(0 <= a < a + 4096 <= 0x4000000 for a, _ in pages[:populated])
        assert all(not (0 <= a < a + 4096 <= 0x4000000) for a, _ in pages[populated:])
        assert metadata['unmapped_entries'] == [dict(index=i, address=pages[i][0], control=pages[i][1])
                                                 for i in range(populated, len(pages))]
        assert metadata['stable_readback']
        # The immutable EP boot instructions precede its separate command
        # table and loader. Runtime PRAM counters/overlays are not normalized.
        assert image.data.find(ep_scratch[:0x320]) >= 0, 'Original EP boot instructions differ'
        for gi, (gp_address, _) in enumerate(witness['scratch']['pages']):
            for ei, (ep_address, _) in enumerate(pages[:populated]):
                if gp_address == ep_address:
                    shared_encoder_pages.append(dict(gp_page=gi, ep_page=ei, physical_address=gp_address))
        encoder_capture_verified = True
    return dict(scope=__doc__, xbe_sha256=PIN, effects=rows,
                effect_payload_bytes=sum(row['bytes'] for row in rows),
                original_footer_bytes=len(original_footer),
                complete_uploaded_effect_program_bytes=program_words * 4,
                gp_control_program_bytes=len(control_program),
                gp_control_program_original_address=control_address,
                gp_control_program_sha256=hashlib.sha256(control_program).hexdigest(),
                complete_uploaded_gp_program_bytes=len(program),
                all_effects_initialized=True, stable_capture_readback=True,
                captured_scratch_data_verified=scratch_verified,
                encoder_capture_verified=encoder_capture_verified,
                shared_gp_encoder_pages=shared_encoder_pages,
                capture_report_sha256=sha(capture / 'report.json'),
                decoder_report_sha256=sha(decoded / 'report.json'),
                observer_sha256=sha(observer), all_uploaded_program_bytes_match=True,
                native_dsp_pcm_verified=False, full_engine_verified=False,
                physical_console_verified=False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--capture', type=Path, required=True)
    parser.add_argument('--decoded', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--observer', type=Path, default=ROOT / 'tools/capture_live_sound_dsp_tu15.py')
    parser.add_argument('--native', type=Path,
                        help='Native campaign_audio_tu15_tests-compatible decoder fixture reader')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    try:
        report = audit(args.capture, args.decoded, args.observer)
        if args.native:
            command = [str(args.native.resolve()), str(args.decoded.resolve())]
            result = subprocess.run(command, capture_output=True, text=True, timeout=30)
            (args.output / 'native.log').write_text(result.stdout + result.stderr)
            report['native_decoder_fixture_command'] = command
            report['native_decoder_fixture_status'] = result.returncode
            report['native_decoder_sha256'] = sha(args.native)
            assert result.returncode == 0, 'Native decoder fixture comparison failed'
        report['passed'] = True
    except (AssertionError, KeyError, OSError, ValueError, subprocess.SubprocessError) as error:
        report = dict(passed=False, scope=__doc__, error=str(error) or type(error).__name__,
                      full_engine_verified=False, native_dsp_pcm_verified=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report), flush=True)
    return int(not report['passed'])


if __name__ == '__main__':
    raise SystemExit(main())
