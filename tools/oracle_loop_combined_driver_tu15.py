#!/usr/bin/env python3
"""Capture the unchanged combined driver packet and routing call in TU1.5.

The component starts with a declared driver voice record and initialized
routing plugin. It executes 2B9450 and 2B96B0 and stops at the SDK call
instruction after the complete route list is prepared. No instruction or
callee result is substituted.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn import x86_const as R


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--publication", type=Path, default=Path("tests/fixtures/sound_gain_envelope_tu15.json"))
    args = parser.parse_args()
    scratch = Path(os.environ["HALO2_TEST_SCRATCH"]).resolve(strict=True)
    output = args.output.resolve()
    if not output.is_relative_to(scratch) or output.exists():
        parser.error("Use a fresh fixture path in owned test scratch")

    image = Xbe(NEW_DEFAULT)
    assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, ".text")
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    cpu.mem_map(0x10000, 0x800000)
    for section in image.sections:
        cpu.mem_write(section.va, image.section_bytes(section))
    bank = 0x1000000
    cpu.mem_map(bank, 0x10000)
    driver, record, system, plugin, stack = (bank + n for n in (0x100, 0x200, 0x1000, 0x4000, 0x8000))
    start, finish = 0x2266DA, 0x226B46
    bits = lambda value: struct.unpack("<I", struct.pack("<f", value))[0]

    def words(address, *values):
        cpu.mem_write(address, struct.pack(f"<{len(values)}I", *values))

    def read(address, count):
        return struct.unpack(f"<{count}I", cpu.mem_read(address, count * 4))

    assert read(0x4633FC + 0x18, 1) == (0x2B9450,)
    assert read(0x4633FC + 0x14, 1) == (0x2B96B0,)
    assert read(0x4633FC + 0x10, 1) == (0x2B9330,)
    constants = {f"{at:08x}": read(at, 1)[0]
                 for at in (0x455B0C, 0x455B14, 0x45B41C, 0x46A0D0, 0x46A0E8, 0x46A118, 0x46A12C, 0x46A53C, 0x46A7D8)}
    words(0x52D674, system)
    words(system + 0x2AB4, plugin)
    words(plugin, 0x4633FC)

    sites = {}
    dispatch = []

    def observe(_, pc, size, __):
        instruction = code.at(pc)
        assert instruction and bytes(cpu.mem_read(pc, size)) == image.read(pc, size)
        sites[f"{pc:08x}"] = instruction.bytes.hex()
        if pc == 0x2B9450:
            sp = cpu.reg_read(R.UC_X86_REG_ESP)
            ret, params, layout, category, routes = read(sp, 5)
            assert ret == 0x226B09 and params == stack + 0x1C and routes == stack + 0x44
            assert layout == cpu.mem_read(system + 1, 1)[0]
            assert cpu.reg_read(R.UC_X86_REG_ECX) == plugin
            dispatch.append((category, read(params, 10)))

    cpu.hook_add(UC_HOOK_CODE, observe)
    gains = ((-64, -64, -64, -64), (0, -6, -12, -18), (-1, -1, -1, -1),
             (-24, -32, -40, -48), (-120, -98, -72, -65), (4, 5, -2, -19),
             (-63.9999, -64.0001, -6.0206, -7.13), (-0.0, 0.0, -64, -17))
    rows = []
    def capture(cw, mxcsr, category, flags, voice_gain, channels, plugin_gain,
                publication_index=None, mode=0, layout=3, auxiliary=0xFFFF):
        cpu.mem_write(stack - 0x400, b"\x00" * 0x800)
        cpu.mem_write(driver, b"\x00" * 0x80)
        cpu.mem_write(driver + 0x12, struct.pack("<H", auxiliary))
        words(driver + 0x14, voice_gain)
        words(driver + 0x24, 0xFFFF)
        words(driver + 0x30, flags)
        words(driver + 0x34, mode)
        words(driver + 0x38, *channels)
        words(plugin + 0x18, plugin_gain)
        cpu.mem_write(system + 1, bytes([layout]))
        for reg, value in ((R.UC_X86_REG_ESP, stack), (R.UC_X86_REG_EDI, driver),
                           (R.UC_X86_REG_EBX, record), (R.UC_X86_REG_EDX, 0),
                           (R.UC_X86_REG_ESI, category),
                           (R.UC_X86_REG_FPCW, cw), (R.UC_X86_REG_FPSW, 0),
                           (R.UC_X86_REG_FPTAG, 0xFFFF), (R.UC_X86_REG_MXCSR, mxcsr),
                           (R.UC_X86_REG_EFLAGS, 2)):
            cpu.reg_write(reg, value)
        dispatch.clear()
        cpu.emu_start(start, finish, count=4000)
        assert cpu.reg_read(R.UC_X86_REG_EIP) == finish
        [(actual_category, packet)] = dispatch
        assert actual_category == category and packet[6:] == channels
        count, = read(stack + 0x84, 1)
        assert read(stack - 4, 1) == (stack + 0x84,)
        assert read(stack + 0x88, 1) == (stack + 0x44,)
        assert 4 <= count <= 8
        routes = [read(stack + 0x44 + 8 * j, 2) for j in range(count)]
        row = dict(control_word=cw, mxcsr=mxcsr, driver_flags=flags, voice_gain=voice_gain,
                   channel_gains=channels, category=category, plugin_gain=plugin_gain,
                   packet=packet, routes=routes, mode=mode, layout=layout, auxiliary=auxiliary)
        if publication_index is not None:
            row["publication_index"] = publication_index
        rows.append(row)

    for rounding in range(4):
        cw = 0x027F | (rounding << 10)
        mxcsr = 0x1F80 | (rounding << 13)
        for category in (0, 1, 2, 255):
            for flags in (0x20, 0x22, 0x28, 0x7A):
                for n, case in enumerate(gains):
                    voice_gain, plugin_gain = bits((-14.25, 3, -64, 0)[n % 4]), bits((-2.75, -6, 0)[n % 3])
                    capture(cw, mxcsr, category, flags, voice_gain, tuple(map(bits, case)), plugin_gain)

    # Modes that fill remaining slots, including the skipped four-entry
    # branch, bypass vs. processed destinations, and the auxiliary callback.
    for rounding in range(4):
        for layout in (0, 3):
            for flags in (0x20, 0x28):
                for mode in (1, 2, 3, 4):
                    for category in (0, 1, 2):
                        capture(0x027F | (rounding << 10), 0x1F80 | (rounding << 13), category,
                                flags, bits(-11), tuple(map(bits, gains[3])), bits(-7),
                                mode=mode, layout=layout)
                for category in (0, 1, 2):
                    capture(0x027F | (rounding << 10), 0x1F80 | (rounding << 13), category,
                            flags, bits(-11), tuple(map(bits, gains[3])), bits(-7), auxiliary=29,
                            layout=layout)

    # Compose retained outputs of the independently qualified original
    # publisher with a fresh original driver invocation.
    publication_bytes = args.publication.read_bytes()
    publication = json.loads(publication_bytes)
    assert publication["original_xbe_sha256"] == NEW_PIN and not publication["substituted_calls"]
    assert not publication["source_instruction_patches"] and len(publication["combined"]) == 120
    for index, row in enumerate(publication["combined"]):
        capture(row["fp_control"], row["mxcsr"], (0, 1, 2)[index % 3], (0x20, 0x28)[index % 2],
                bits(-5), tuple(row["expected"]), bits(-7), index)
    result = dict(original_xbe_sha256=NEW_PIN, source_start=start, source_stop=finish,
                  executed_instructions=sites, constants=constants, cases=rows,
                  publication_fixture_sha256=hashlib.sha256(publication_bytes).hexdigest(),
                  supplied_callee_results=[], instruction_patches=[])
    output.write_text(json.dumps(result, separators=(",", ":")) + "\n")
    print(f"{len(rows)} combined driver packets and route lists, {len(sites)} original instruction sites")


if __name__ == "__main__":
    main()
