#!/usr/bin/env python3
"""Authenticate codec registration, callbacks and teardown; offline inputs only.

This is source-intent research, not a native completion/IRQ implementation.
Declared kernel outcomes and physical status inputs are test fixture inputs.
No instruction is patched and no Xbox kernel body is claimed to execute.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R
from oracle_smg_firing_1_1 import Xbe, _Code, _trace
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from oracle_sound_sdk_native_prefix_tu15 import GP, FS, GDT, STOP, segment

CONTROLLER, STACK, SYNC_RETURN = 0x18000000, 0x2018000, STOP + 0x100
ENTRIES = [0x38f45f, 0x38ed2c, 0x38ecff, 0x38ed90, 0x38edad,
           0x38f378, 0x38f24b, 0x38f389, 0x38f31f, 0x38eee3,
           0x38ed7c, 0x38f1f4, 0x38edf4, 0x38f443, 0x38f3d4,
           0x38f395, 0x383d70, 0x383d92]


def run(image, authenticated, fill, kind, flags=1, status=0, channel_status=0,
        route_cookie=0x36, irql=3, connect=True):
    cpu = Uc(UC_ARCH_X86, UC_MODE_32)
    for address, size in [(0x10000, 0x600000), (0x2010000, 0x10000),
                          (CONTROLLER, 0x4000), (STOP, 0x1000),
                          (0x80000000, 0x1000), (FS, 0x1000),
                          (GDT, 0x1000), (0xfec00000, 0x1000)]:
        cpu.mem_map(address, size)
    for section in image.sections:
        raw = image.section_bytes(section)
        if section.va + len(raw) <= 0x610000:
            cpu.mem_write(section.va, raw)
    cpu.mem_write(0x2010000, bytes([fill]) * 0x10000)
    cpu.mem_write(GDT + 8, segment(0, 0x9b) + segment(0, 0x93) + segment(FS, 0x93))
    cpu.reg_write(R.UC_X86_REG_GDTR, (0, GDT, 4095, 0))
    for reg, value in [(R.UC_X86_REG_CS, 8), (R.UC_X86_REG_DS, 16),
                       (R.UC_X86_REG_ES, 16), (R.UC_X86_REG_SS, 16),
                       (R.UC_X86_REG_FS, 24)]:
        cpu.reg_write(reg, value)
    gp = [0x21000000 + i * 0x111111 for i in range(8)]
    gp[1], gp[4] = CONTROLLER, STACK
    for reg, value in zip(GP, gp):
        cpu.reg_write(reg, value)
    cpu.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
    cpu.mem_write(0xfec00130, struct.pack('<I', 0x100 if kind == 'constructor' else status))
    cpu.mem_write(CONTROLLER + 8, struct.pack('<I', flags))
    if kind in ('isr', 'deferred') and channel_status:
        for index, offset in enumerate([0x10, 0x70]):
            channel, descriptors = CONTROLLER + 0x100 + index * 0x100, CONTROLLER + 0x1000 + index * 0x100
            cpu.mem_write(CONTROLLER + index * 4, struct.pack('<I', channel))
            cpu.mem_write(channel, struct.pack('<I', index))
            cpu.mem_write(channel + 0x18, struct.pack('<I', descriptors))
            cpu.mem_write(channel + 0x24, bytes([3, 0]))
            cpu.mem_write(descriptors + 4, struct.pack('<I', 0x100 + index * 0x80))
            cpu.mem_write(0xfec00106 + offset, struct.pack('<H', channel_status))
    if kind == 'shutdown':
        cpu.mem_write(0x391208, struct.pack('<2I', 0x38f378, CONTROLLER))
        cpu.mem_write(CONTROLLER + 0xc, struct.pack('<I', 0x38f443))
        if not flags & 1:
            cpu.mem_write(CONTROLLER + 0x1c, struct.pack('<H', 0x13))
    instructions, stores, calls, sync = [], [], [], []
    def word(p): return struct.unpack('<I', cpu.mem_read(p, 4))[0]
    def returned(sp, count):
        cpu.reg_write(R.UC_X86_REG_ESP, sp + count)
        cpu.reg_write(R.UC_X86_REG_EIP, word(sp))
    def hook(m, pc, size, _):
        sp = m.reg_read(R.UC_X86_REG_ESP)
        if pc == SYNC_RETURN:
            original = sync.pop()
            assert m.reg_read(R.UC_X86_REG_EAX) & 255 == 1
            returned(original, 16)
            return
        if pc >= 0x80000000:
            target, ret = pc & 0x7fffffff, word(sp)
            counts = {44: 12, 47: 12, 98: 8, 100: 8, 107: 16, 109: 32,
                      119: 16, 137: 8, 153: 16, 160: 4, 161: 4}
            assert target in counts, (hex(pc), hex(ret))
            args = [word(sp + i) for i in range(0, counts[target], 4)]
            calls.append(dict(ordinal=target, frame=args, ecx=m.reg_read(R.UC_X86_REG_ECX)))
            if target == 44:
                assert args[:2] == [0x38f4ca, 6]
                m.mem_write(args[2], bytes([irql])); m.reg_write(R.UC_X86_REG_EAX, route_cookie)
            elif target == 107:
                assert args == [0x38f4bd, CONTROLLER + 0x1c, 0x38f389, CONTROLLER]
                m.mem_write(args[1], struct.pack('<H', 0x13))
            elif target == 109:
                assert args == [0x38f4e4, 0x391208, 0x38f378, CONTROLLER, route_cookie, irql, 0, 1]
                m.mem_write(0x391208, struct.pack('<2I', args[2], args[3]))
            elif target == 98:
                assert args == [0x38f4eb, 0x391208]
                m.reg_write(R.UC_X86_REG_EAX, (m.reg_read(R.UC_X86_REG_EAX) & 0xffffff00) | int(connect))
            elif target == 153:
                assert args == [0x38f337, 0x391208, 0x38eee3, CONTROLLER]
                sync.append(sp); m.mem_write(sp - 8, struct.pack('<2I', SYNC_RETURN, CONTROLLER))
                m.reg_write(R.UC_X86_REG_ESP, sp - 8); m.reg_write(R.UC_X86_REG_EIP, 0x38eee3)
                return
            elif target == 160:
                assert m.reg_read(R.UC_X86_REG_ECX) & 255 == 2
                previous = bytes(m.mem_read(FS + 0x24, 1))[0]
                m.mem_write(FS + 0x24, b'\x02')
                m.reg_write(R.UC_X86_REG_EAX, (m.reg_read(R.UC_X86_REG_EAX) & 0xffffff00) | previous)
            elif target == 161:
                m.mem_write(FS + 0x24, bytes([m.reg_read(R.UC_X86_REG_ECX) & 255]))
            elif target == 119:
                assert args == [0x38f307, CONTROLLER + 0x1c, 0, 0]
                m.reg_write(R.UC_X86_REG_EAX, (m.reg_read(R.UC_X86_REG_EAX) & 0xffffff00) | 1)
            elif target == 100:
                assert args == [0x38f3fd, 0x391208]
                m.reg_write(R.UC_X86_REG_EAX, (m.reg_read(R.UC_X86_REG_EAX) & 0xffffff00) | 1)
            elif target == 137:
                assert args == [0x38f414, CONTROLLER + 0x1c]
                m.reg_write(R.UC_X86_REG_EAX, (m.reg_read(R.UC_X86_REG_EAX) & 0xffffff00) | 1)
            elif target == 47:
                assert args in ([0x38f535, CONTROLLER + 0xc, 1], [0x38f426, CONTROLLER + 0xc, 0])
            returned(sp, counts[target]); return
        assert pc in authenticated, (kind, hex(pc))
        assert image.read(pc, size) == bytes(m.mem_read(pc, size))
        instructions.append(pc)
    def write(m, access, address, size, value, _):
        stores.append(dict(instruction=m.reg_read(R.UC_X86_REG_EIP), address=address,
                           size=size, value=value & ((1 << (size * 8)) - 1)))
    cpu.hook_add(UC_HOOK_CODE, hook); cpu.hook_add(UC_HOOK_MEM_WRITE, write)
    entry = {'constructor': 0x38f45f, 'isr': 0x38f378, 'deferred': 0x38f378, 'shutdown': 0x38f443}[kind]
    if kind == 'constructor': frame = [STOP, flags]
    elif kind == 'shutdown': frame = [STOP, CONTROLLER + 0xc]
    else: frame = [STOP, 0x391208, CONTROLLER]
    cpu.mem_write(STACK, struct.pack('<' + 'I' * len(frame), *frame))
    cpu.emu_start(entry, STOP, count=50000)
    assert cpu.reg_read(R.UC_X86_REG_EIP) == STOP and not sync
    isr_instructions = len(instructions)
    if kind == 'deferred':
        assert not flags & 1
        cpu.mem_write(STACK, struct.pack('<5I', STOP, CONTROLLER + 0x1c, CONTROLLER, 0, 0))
        cpu.reg_write(R.UC_X86_REG_ESP, STACK)
        cpu.emu_start(0x38f389, STOP, count=50000)
        assert cpu.reg_read(R.UC_X86_REG_EIP) == STOP and not sync
    result = dict(kind=kind, fill=fill, source_flags=flags, offline_global_status=status,
                  offline_channel_status=channel_status, offline_vector=route_cookie,
                  offline_irql=irql, offline_connect_result=connect,
                  instructions=instructions, source_stores=stores, kernel_interface_frames=calls,
                  final_gp=[cpu.reg_read(r) for r in GP], final_flags=cpu.reg_read(R.UC_X86_REG_EFLAGS),
                  controller_hex=bytes(cpu.mem_read(CONTROLLER, 0x3c)).hex(),
                  channels_hex=[bytes(cpu.mem_read(CONTROLLER + o, 0x30)).hex() for o in [0x100, 0x200]],
                  interrupt_cleanup_hex=bytes(cpu.mem_read(0x391208, 8)).hex(),
                  global_acknowledgements=[s for s in stores if s['address'] == 0xfec00130],
                  isr_instructions=isr_instructions)
    if kind == 'constructor':
        assert result['final_gp'][0] == (0 if connect else 0x80004005)
        assert any(c['ordinal'] == 47 for c in calls) == connect
    elif kind == 'shutdown':
        assert bytes(cpu.mem_read(0x391208, 4)) == bytes(4)
        assert bytes(cpu.mem_read(CONTROLLER + 0xc, 4)) == bytes(4)
    elif kind == 'deferred':
        assert bytes(cpu.mem_read(CONTROLLER + 0x38, 4)) == bytes(4)
        assert [word(CONTROLLER + o + 4) for o in [0x100, 0x200]] == [0x100, 0x180]
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args(); assert not args.output.exists()
    image = Xbe(NEW_DEFAULT); assert hashlib.sha256(image.data).hexdigest() == NEW_PIN
    code = _Code(image, 'DSOUND'); authenticated = set(); listings = []
    args.output.mkdir(parents=True)
    for entry in ENTRIES:
        body = _trace(code, entry, set(ENTRIES)); rows = []
        for pc in sorted(body.code):
            instruction = code.at(pc); assert bytes(instruction.bytes) == image.read(pc, instruction.size)
            authenticated.add(pc)
            rows.append(f'{pc:08x} {bytes(instruction.bytes).hex()} {instruction.mnemonic} {instruction.op_str}')
        name = f'original-{entry:08x}.asm'; (args.output / name).write_text('\n'.join(rows) + '\n')
        listings.append(dict(entry=entry, sites=len(body.code), file=name,
                             sha256=hashlib.sha256((args.output / name).read_bytes()).hexdigest()))
    literal_consumers = []
    for section in image.sections:
        raw = image.section_bytes(section)
        at = raw.find(struct.pack('<I', 0x391208))
        while at >= 0:
            address = section.va + at
            containing = [pc for pc in authenticated if pc <= address and address + 4 <= pc + code.at(pc).size]
            assert len(containing) == 1, (section.name, hex(address), containing)
            literal_consumers.append(dict(section=section.name, literal=address, instruction=containing[0]))
            at = raw.find(struct.pack('<I', 0x391208), at + 1)
    assert sorted(r['instruction'] for r in literal_consumers) == [0x38f32c, 0x38f3ea, 0x38f3f2, 0x38f3fd, 0x38f4d1]
    assert image.read(0x390ba0, 8) == struct.pack('<2I', 0x40, 0x10)
    cases = []
    for fill in [0, 0x5a, 0xa5]:
        cases += [run(image, authenticated, fill, 'constructor', 1),
                  run(image, authenticated, fill, 'constructor', 0, route_cookie=0xf9074321, irql=0),
                  run(image, authenticated, fill, 'constructor', 1, connect=False),
                  run(image, authenticated, fill, 'isr', 1),
                  run(image, authenticated, fill, 'isr', 1, 0x50, 8),
                  run(image, authenticated, fill, 'deferred', 0, 0x50, 8),
                  run(image, authenticated, fill, 'shutdown', 1),
                  run(image, authenticated, fill, 'shutdown', 0)]
    report = dict(passed=True, original_xbe_sha256=NEW_PIN,
                  tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  scope=__doc__, listings=listings, cases=cases, source_controller=CONTROLLER,
                  complete_image_interrupt_literal_consumers=literal_consumers,
                  interrupt_identity_uses='Returned vector is forwarded only to initialize in this constructor. IRQ object is subsequently used by synchronization/disconnect.',
                  channel_interrupt_masks=[0x40, 0x10], descriptor_completion_bit=8,
                  no_instruction_patches=True, native_hardware_or_irq_results_supplied=False,
                  kernel_bodies_executed=False, actual_native_registration_tested=False,
                  actual_descriptor_producer_tested=False, full_startup_completed=False)
    (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=True, original_cases=len(cases), bodies=len(listings),
                          native_registration=False, native_descriptor_producer=False)))


if __name__ == '__main__': main()
