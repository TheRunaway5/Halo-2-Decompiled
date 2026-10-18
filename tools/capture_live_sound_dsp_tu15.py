"""Read the DSP uploaded by an unchanged, cold-booted TU1.5 executable.

Run this file with GDB's Python after attaching to an isolated xemu started
with -S. Set H2_DSP_CAPTURE_OUTPUT to a new directory. The optional
H2_DSP_DECODED_REFERENCE selects an oracle_sound_dsp_image_tu15 output.
Hardware breakpoints observe the complete original environment constructor.
No guest instruction, register, memory, service return or readiness value is
supplied. This is an emulator capture, not native or physical-console PCM proof.
"""
import hashlib
import json
import os
from pathlib import Path
import struct
import sys

import gdb

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
from xbe import Xbe

PIN = '21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
IMAGE = Path('/home/eric/.local/share/halo2-native/imports/halo2-dlc-927a4b262a57/Halo 2 DLC/content/u5English/default.xbe')
GP_P, GP_X, GP_Y, GP_MIX = 0xfe83a000, 0xfe830000, 0xfe836000, 0xfe835000


def capture():
    image = Xbe(IMAGE)
    assert hashlib.sha256(image.data).hexdigest() == PIN
    reference = Path(os.environ.get('H2_DSP_DECODED_REFERENCE',
                                    str(ROOT / 'tests/fixtures/sound_dsp_image_tu15')))
    decoded_report = json.loads((reference / 'report.json').read_text())
    assert decoded_report['xbe_sha256'] == PIN
    assert not decoded_report['instruction_patches']
    assert not decoded_report['supplied_external_results']
    section = next(s for s in image.sections if s.name == 'DSPImage')
    resource = image.read(section.va, section.vsize)
    assert hashlib.sha256(resource).hexdigest() == decoded_report['dsp_section_sha256']
    program_words = struct.unpack_from('<I', resource, 0x804)[0]

    output = Path(os.environ['H2_DSP_CAPTURE_OUTPUT'])
    output.mkdir(parents=True, exist_ok=False)
    inferior = gdb.selected_inferior()

    def read(address, size):
        return bytes(inferior.read_memory(address, size))

    def word(address):
        return struct.unpack('<I', read(address, 4))[0]

    def reg(name):
        return int(gdb.parse_and_eval('$' + name)) & 0xffffffff

    def breakpoint(address):
        return gdb.Breakpoint(f'*0x{address:x}', type=gdb.BP_HARDWARE_BREAKPOINT,
                              internal=True)

    def dsp_region(name, address, words):
        raw = b''.join(read(address + 4 * i, 4) for i in range(words))
        assert len(raw) == words * 4
        assert all(v <= 0xffffff for v in struct.unpack('<' + 'I' * words, raw))
        (output / (name + '.bin')).write_bytes(raw)
        return raw, dict(name=name, address=address, words=words,
                         sha256=hashlib.sha256(raw).hexdigest())

    entry = 0x1972c0
    stop = breakpoint(entry)
    gdb.execute('continue')
    assert reg('eip') == entry
    stop.delete()
    # Entire constructor, including the original final RET 4. Hardware
    # breakpoints leave all these instruction bytes unchanged.
    original_body = image.read(entry, 0x120)
    assert read(entry, len(original_body)) == original_body
    sp = reg('esp')
    return_pc, device = word(sp), word(sp + 4)
    before = dict(pc=entry, sp=sp, return_pc=return_pc, device=device)
    (output / 'before.json').write_text(json.dumps(before, indent=2) + '\n')
    stop = breakpoint(return_pc)
    gdb.execute('continue')
    assert reg('eip') == return_pc
    stop.delete()
    assert reg('esp') == sp + 8 and reg('eax') & 255
    actual_return = reg('eax')

    environment = word(0x51df90)
    descriptor_address = word(environment)
    count = word(descriptor_address)
    assert count == len(decoded_report['effects']) == 15
    descriptor = read(descriptor_address, 8 + count * 32)
    (output / 'descriptor.bin').write_bytes(descriptor)
    (output / 'environment.bin').write_bytes(read(environment, 0x48))
    (output / 'device.bin').write_bytes(read(device, 0x80))

    # Locate the code from the actual GP control program's JSR operand.
    # Descriptor code pointers alone omit the two-word R5 prologue. Retain
    # that observed relationship separately rather than silently correcting
    # or treating the descriptor pointers as execution entry points.
    opcode, first_program_word = struct.unpack('<2I', read(GP_P + 0x37 * 4, 8))
    assert opcode == 0x0bf080 and first_program_word == 0x171
    assert first_program_word + program_words <= 0x1000
    program, program_region = dsp_region('gp_p_loaded', GP_P,
                                         first_program_word + program_words)
    cursor = first_program_word * 4
    effects = []
    for expected in decoded_report['effects']:
        index = expected['index']
        fields = struct.unpack_from('<8I', descriptor, 8 + index * 32)
        code_address, code_size, state_address, state_size, y_address, y_size, scratch_address, scratch_size = fields
        decoded = (reference / f'effect-{index:02d}.bin').read_bytes()
        assert hashlib.sha256(decoded).hexdigest() == expected['sha256']
        assert code_size == expected['bytes'] == len(decoded)
        uploaded = program[cursor:cursor + code_size]
        (output / f'effect-{index:02d}-program.bin').write_bytes(uploaded)
        row = dict(index=index, descriptor_address=code_address,
                   program_address=GP_P + cursor,
                   descriptor_entry_offset=code_address - (GP_P + cursor),
                   size=code_size, sha256=hashlib.sha256(uploaded).hexdigest(),
                   decoded_sha256=expected['sha256'], matches_decoder=uploaded == decoded,
                   state_address=state_address, state_size=state_size,
                   y_address=y_address, y_size=y_size,
                   scratch_address=scratch_address, scratch_size=scratch_size)
        effects.append(row)
        (output / 'effects-progress.json').write_text(json.dumps(effects, indent=2) + '\n')
        assert uploaded == decoded, f'Original uploaded effect {index} differs from decoded data'
        assert row['descriptor_entry_offset'] == 8
        cursor += code_size
        print(f'PASS original uploaded DSP effect {index}: {code_size} exact bytes', flush=True)
    footer = program[cursor:]
    resource_cursor = 0x818 + sum(row['size'] for row in effects)
    assert footer == resource[resource_cursor:0x818 + program_words * 4]
    assert footer == b'\x0c\0\0\0'  # Original, unencrypted final RTS.

    x, x_region = dsp_region('gp_x', GP_X, 0x1000)
    mix, mix_region = dsp_region('gp_mix', GP_MIX, 0x400)
    y, y_region = dsp_region('gp_y', GP_Y, 0x800)
    # The installed emulator quiesces the APU on a VM stop. Confirm stable
    # readback as well; do not silently use a moving memory snapshot.
    for address, raw in ((GP_X, x), (GP_MIX, mix), (GP_Y, y)):
        assert raw == b''.join(read(address + i, 4) for i in range(0, len(raw), 4))
    flags = []
    for row in effects:
        at = row['state_address'] - GP_X
        assert 0 <= at < at + row['state_size'] <= 0xc00 * 4
        flags.append(struct.unpack_from('<I', x, at + 16)[0])
    # 388309 relocates the CPU scratch pointers from the image offsets.
    # The GP state retains those byte offsets in each effect's first word.
    # Require one consistent mapping across all effect allocations. This
    # describes the effect-delay allocation only, not the separately allocated
    # GP control/image area at scratch offsets below 0xc000.
    scratch_base = None
    scratch_extent = 0
    for row in effects:
        at = row['state_address'] - GP_X
        scratch_offset = struct.unpack_from('<I', x, at)[0]
        base = row['scratch_address'] - scratch_offset
        if scratch_base is None:
            scratch_base = base
        assert scratch_base == base
        assert struct.unpack_from('<I', x, at + 4)[0] == row['scratch_size']
        row['scratch_offset'] = scratch_offset
        scratch_extent = max(scratch_extent, scratch_offset + row['scratch_size'])
    assert scratch_base is not None and 0 < scratch_extent <= 0x100000
    sge_base, max_sge = word(0xfe802040), word(0xfe8020d4)
    page_count = (scratch_extent + 4095) // 4096
    assert 0 < sge_base < 0x8000000 and page_count <= max_sge + 1
    # QEMU's documented physical read mode avoids inventing a CPU virtual
    # alias for the actual APU scatter/gather pages. Only read operations
    # follow; restore the debugger's normal address-translation mode.
    mode_log = [gdb.execute('maintenance packet qqemu.PhyMemMode', to_string=True)]
    assert 'received: "0"' in mode_log[-1]
    mode_log.append(gdb.execute('maintenance packet Qqemu.PhyMemMode:1', to_string=True))
    assert 'received: "OK"' in mode_log[-1]
    try:
        sge_table = read(sge_base, page_count * 8)
        pages = [struct.unpack_from('<2I', sge_table, i * 8) for i in range(page_count)]
        assert all(0 <= address < address + 4096 <= 0x8000000 for address, _ in pages)
        def read_scratch():
            return b''.join(read(address, min(4096, scratch_extent - i * 4096))
                            for i, (address, _) in enumerate(pages))
        scratch = read_scratch()
        assert scratch == read_scratch()
        assert sge_table == read(sge_base, len(sge_table))
    finally:
        mode_log.append(gdb.execute('maintenance packet Qqemu.PhyMemMode:0', to_string=True))
        assert 'received: "OK"' in mode_log[-1]
    assert scratch[0x800:0x818] == x[:0x18], 'GP scratch header differs from the actual GP header'
    assert scratch[:first_program_word * 4] == program[:first_program_word * 4]
    assert scratch[0x818:0x818 + program_words * 4] == program[first_program_word * 4:]
    for row in effects:
        if row['scratch_size']:
            raw = read(row['scratch_address'], row['scratch_size'])
            assert raw == scratch[row['scratch_offset']:row['scratch_offset'] + row['scratch_size']]
    (output / 'gp_scratch_sg_table.bin').write_bytes(sge_table)
    (output / 'debugger-memory-mode.log').write_text(''.join(mode_log))
    (output / 'gp_scratch.bin').write_bytes(scratch)
    ep_reset_control = word(0xfe85fffc)
    encoder = dict(reset_control=ep_reset_control, enabled=(ep_reset_control & 3) == 3)
    if encoder['enabled']:
        # The actual reset/bootstrap path loads and masks the first 0x800
        # program words. Never read the unallocated, poisoned PRAM tail.
        ep_program, ep_program_region = dsp_region('ep_bootstrap_program', 0xfe85a000, 0x800)
        encoder['bootstrap_program'] = ep_program_region
        ep_sge_base, ep_max_sge = word(0xfe802048), word(0xfe8020dc)
        encoder['scratch_controls'] = dict(sge_base=ep_sge_base, max_sge=ep_max_sge)
        (output / 'encoder-progress.json').write_text(json.dumps(encoder, indent=2) + '\n')
        assert 0 < ep_sge_base < 0x8000000 and 0 <= ep_max_sge < 1024, f'EP scratch controls: {ep_sge_base:08x}, {ep_max_sge:08x}'
        assert 'received: "OK"' in gdb.execute('maintenance packet Qqemu.PhyMemMode:1', to_string=True)
        try:
            ep_table = read(ep_sge_base, (ep_max_sge + 1) * 8)
            ep_pages = [struct.unpack_from('<2I', ep_table, i * 8) for i in range(ep_max_sge + 1)]
            (output / 'ep_scratch_sg_table.bin').write_bytes(ep_table)
            encoder['scratch_controls']['pages'] = ep_pages
            (output / 'encoder-progress.json').write_text(json.dumps(encoder, indent=2) + '\n')
            # The published limit can include an unpopulated final entry.
            # Preserve it, but never dereference an address outside RAM or
            # manufacture its contents. A hole before a valid page stops us.
            mapped_pages = []
            unmapped_entries = []
            for index, (address, control) in enumerate(ep_pages):
                if not (0 <= address < address + 4096 <= 0x4000000):
                    unmapped_entries.append(dict(index=index, address=address, control=control))
                else:
                    assert not unmapped_entries, 'Nonterminal hole in EP scratch mapping'
                    mapped_pages.append((address, control))
            assert mapped_pages, 'EP scratch has no populated pages'
            ep_scratch = b''.join(read(address, 4096) for address, _ in mapped_pages)
            assert ep_scratch == b''.join(read(address, 4096) for address, _ in mapped_pages)
            assert ep_table == read(ep_sge_base, len(ep_table))
        finally:
            assert 'received: "OK"' in gdb.execute('maintenance packet Qqemu.PhyMemMode:0', to_string=True)
        (output / 'ep_scratch_sg_table.bin').write_bytes(ep_table)
        (output / 'ep_scratch.bin').write_bytes(ep_scratch)
        encoder['scratch'] = dict(mapping='apu_scatter_gather', sge_base=ep_sge_base,
                                  max_sge=ep_max_sge, pages=ep_pages, bytes=len(ep_scratch),
                                  mapped_page_count=len(mapped_pages), unmapped_entries=unmapped_entries,
                                  table_sha256=hashlib.sha256(ep_table).hexdigest(),
                                  sha256=hashlib.sha256(ep_scratch).hexdigest(), stable_readback=True)
    (output / 'encoder-progress.json').write_text(json.dumps(encoder, indent=2) + '\n')
    report = dict(scope=__doc__, xbe_sha256=PIN,
                  observer_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  decoder_report_sha256=hashlib.sha256((reference / 'report.json').read_bytes()).hexdigest(),
                  original_constructor_sha256=hashlib.sha256(original_body).hexdigest(),
                  entry=entry, return_pc=return_pc, actual_return=actual_return,
                  environment=environment, descriptor=descriptor_address,
                  program_entry_word=first_program_word, program_words=program_words,
                  effects=effects, regions=[program_region, x_region, mix_region, y_region],
                  all_programs_match_decoder=True,
                  decoded_program_bytes=sum(row['size'] for row in effects),
                  original_footer_hex=footer.hex(), effect_flags=flags,
                  encoder=encoder,
                  all_effects_initialized=all(flag & 1 for flag in flags), stable_readback=True,
                  scratch=dict(mapping='apu_scatter_gather', effect_cpu_offset_bias=scratch_base,
                               sge_base=sge_base, max_sge=max_sge, pages=pages,
                               table_sha256=hashlib.sha256(sge_table).hexdigest(), bytes=scratch_extent,
                               sha256=hashlib.sha256(scratch).hexdigest(),
                               descriptor_mapping_verified=True, stable_readback=True),
                  original_code_patches=[], original_memory_writes=[],
                  original_register_writes=[], supplied_callee_returns=[],
                  physical_xbox_verified=False, native_pcm_verified=False,
                  complete_cutscene_engine_verified=False)
    (output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('PASS original DSP upload, complete program footer and stable live memory', flush=True)


capture()
