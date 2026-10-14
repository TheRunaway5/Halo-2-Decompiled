#!/usr/bin/env python3
"""Compare converted phmo pre-bind slots against unchanged Xbox TU1.5 code.

Offline reference tooling only. Neither MCC onboarding nor native gameplay
uses Unicorn, this executable, or these comparison packets. No original
instruction or callee result is patched. Procedure inputs are declared, not
captured world/startup state. This checks construction, not simulation parity.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE, UC_HOOK_MEM_READ, UC_HOOK_MEM_WRITE
from unicorn import x86_const as R

PIN = "21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd"
ENTRY, TABLE, STACK, SP, STOP = 0x1f2c70, 0x70000000, 0x72000000, 0x72008000, 0x73000000
REGISTERS = (R.UC_X86_REG_EAX, R.UC_X86_REG_ECX, R.UC_X86_REG_EDX, R.UC_X86_REG_EBX,
             R.UC_X86_REG_ESP, R.UC_X86_REG_EBP, R.UC_X86_REG_ESI, R.UC_X86_REG_EDI)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def rows(packet, address, descriptor, stride):
    count, pointer = struct.unpack_from("<II", packet, descriptor)
    if not count:
        return ()
    offset = pointer - address
    if count > 65536 or offset < 0 or offset + count * stride > len(packet):
        raise ValueError("Physics comparison block escapes its owned packet")
    return tuple(offset + n * stride for n in range(count))


def rebound_slots(packet, address):
    result = []
    layouts = ((0x48, 0x80, (0x20, 0x28, 0x30, 0x38, 0x3c)),
               (0x50, 0xb0, (0x20, 0x28)), (0x58, 0x50, (0x20, 0x28)),
               (0x60, 0x90, (0x20, 0x28, 0x40, 0x48, 0x4c)),
               (0x68, 0x60, (0x20, 0x28)), (0x70, 0x100, (0x20, 0x28, 0x50, 0xf4)),
               (0x90, 0x38, (0, 8, 12)), (0xa0, 0x14, (0, 8, 16)),
               (0x108, 0x20, (0, 8, 20)))
    for descriptor, stride, slots in layouts:
        for row in rows(packet, address, descriptor, stride):
            for slot in slots:
                at = row + slot
                if u32(packet, at):
                    raise ValueError(f"Converted pre-bind record retained a tool/runtime pointer at {descriptor:x}:{row:x}+{slot:x}")
                result.append(at)
    return result


class Oracle:
    def __init__(self, executable):
        image = executable.read_bytes()
        if sha(image) != PIN:
            raise ValueError("Requires the authenticated English Xbox TU1.5 reference")
        self.cpu = Uc(UC_ARCH_X86, UC_MODE_32)
        self.cpu.mem_map(0x10000, 0x7f0000)
        base, table = u32(image, 0x104), u32(image, 0x120)
        for index in range(u32(image, 0x11c)):
            at = table - base + index * 56
            address, extent, offset, size = struct.unpack_from("<IIII", image, at + 4)
            if size > extent or offset + size > len(image):
                raise ValueError("Invalid reference section extent")
            self.cpu.mem_write(address, image[offset:offset + size])
        self.cpu.mem_map(TABLE, 0x100000)
        self.cpu.mem_map(STACK, 0x10000)
        self.cpu.mem_map(STOP, 0x1000)
        self.cpu.mem_write(0x4f0414, struct.pack("<I", TABLE))
        self.mapped = None
        self.state = None
        self.cpu.hook_add(UC_HOOK_CODE, self.code)
        self.cpu.hook_add(UC_HOOK_MEM_READ, self.read)
        self.cpu.hook_add(UC_HOOK_MEM_WRITE, self.write)

    def code(self, cpu, address, size, user):
        self.state["instructions"] += 1

    def read(self, cpu, access, address, size, value, user):
        state = self.state
        if state["base"] <= address < state["base"] + state["mapped_size"]:
            if address + size > state["base"] + state["size"]:
                raise ValueError("Original constructor read beyond the owned physics packet")
            if state["clear"] == 0:
                for at in range(address, address + size):
                    if at in state["rebound"] and at not in state["written"]:
                        raise ValueError(f"Original constructor consumes a cleared pointer before binding at {address:x}")
        elif not (STACK <= address < STACK + 0x10000 or TABLE <= address < TABLE + 0x100000
                  or address == 0x4f0414):
            data = bytes(cpu.mem_read(address, size))
            for offset, byte in enumerate(data):
                key = address + offset
                if key in state["program_reads"] and state["program_reads"][key] != byte:
                    raise ValueError("A reference program operand changed during construction")
                state["program_reads"][key] = byte

    def write(self, cpu, access, address, size, value, user):
        state = self.state
        if state["base"] <= address < state["base"] + state["mapped_size"]:
            if address + size > state["base"] + state["size"]:
                raise ValueError("Original constructor wrote beyond the owned physics packet")
            data = (value & ((1 << (8 * size)) - 1)).to_bytes(size, "little")
            state["stores"].append(dict(instruction=cpu.reg_read(R.UC_X86_REG_EIP), address=address,
                before=bytes(cpu.mem_read(address, size)).hex(), written=data.hex()))
            state["written"].update(range(address, address + size))
        elif not STACK <= address < STACK + 0x10000:
            raise ValueError(f"Original constructor wrote outside its declared procedure outputs: {address:x}")

    def call(self, datum, base, size, clear, slots):
        cpu = self.cpu
        state = dict(base=base, size=size, mapped_size=self.mapped[1], clear=clear,
            rebound={base + at + byte for at in slots for byte in range(4)},
            written=set(), program_reads={}, instructions=0, stores=[])
        self.state = state
        cpu.mem_write(STACK, bytes(0x10000))
        cpu.mem_write(SP, struct.pack("<II", STOP, clear))
        gp = [datum, 0x12340001, 0x12340002, 0x12340003, SP, 0x12340005, 0x12340006, 0x12340007]
        for reg, value in zip(REGISTERS, gp):
            cpu.reg_write(reg, value)
        cpu.reg_write(R.UC_X86_REG_EFLAGS, 0x202)
        cpu.reg_write(R.UC_X86_REG_FPCW, 0x27f)
        cpu.reg_write(R.UC_X86_REG_MXCSR, 0x1f80)
        cpu.emu_start(ENTRY, STOP, count=2000000)
        if cpu.reg_read(R.UC_X86_REG_EIP) != STOP:
            raise ValueError("Original constructor did not return within its instruction bound")
        if clear == 0 and not state["rebound"] <= state["written"]:
            raise ValueError("Original constructor did not replace every cleared runtime pointer")
        reads = []
        for address, value in sorted(state["program_reads"].items()):
            if reads and reads[-1]["address"] + len(reads[-1]["bytes"]) == address:
                reads[-1]["bytes"].append(value)
            else:
                reads.append(dict(address=address, bytes=[value]))
        return dict(clear=clear, initial_gp=gp, returned_gp=[cpu.reg_read(reg) for reg in REGISTERS],
            returned_eflags=cpu.reg_read(R.UC_X86_REG_EFLAGS), instructions=state["instructions"],
            stores=state["stores"], program_reads=[dict(address=r["address"], bytes=bytes(r["bytes"]).hex()) for r in reads],
            after=bytes(cpu.mem_read(base, size)))

    def verify(self, packet, datum, base, export):
        if self.mapped:
            self.cpu.mem_unmap(*self.mapped)
        page, prefix = base & ~0xfff, base & 0xfff
        self.mapped = page, (prefix + len(packet) + 0xfff) & ~0xfff
        self.cpu.mem_map(*self.mapped)
        if prefix:
            raise ValueError("Reference packets must begin on a page for the current oracle mapping")
        slots = rebound_slots(packet, base)
        self.cpu.mem_write(TABLE + (datum & 65535) * 16, struct.pack("<IIII", 0x70686d6f, datum, base, len(packet)))
        expected = []
        for poison in (False, True):
            before = bytearray(packet)
            if poison:
                for index, at in enumerate(slots):
                    struct.pack_into("<I", before, at, 0xad000001 + index)
            self.cpu.mem_write(base, bytes(before))
            phases = []
            for clear in (0, 1):
                phase = self.call(datum, base, len(packet), clear, slots)
                phases.append(phase)
            if not poison:
                expected = phases
                for phase in phases:
                    name = export.name + f".after-{phase['clear']}.bin"
                    (export.parent / name).write_bytes(phase.pop("after"))
                    phase["after_file"] = name
            else:
                for actual, wanted in zip(phases, expected):
                    if actual["after"] != (export.parent / wanted["after_file"]).read_bytes():
                        raise ValueError("Discarded MCC tool pointers change original constructor outputs")
                    if actual["returned_gp"] != wanted["returned_gp"]:
                        raise ValueError("Discarded MCC tool pointers change original constructor registers")
        return dict(runtime_pointer_slots=len(slots), phases=expected)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference-xbe", type=Path, required=True)
    parser.add_argument("--packets", type=Path, nargs="+", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    oracle = Oracle(args.reference_xbe)
    results, seen = [], {}
    for directory in args.packets:
        manifest = json.loads((directory / "packets.json").read_text())
        if manifest["schema"] != "H2_MCC_PHYSICS_REFERENCE_PACKETS_V1":
            raise ValueError("Wrong physics packet manifest")
        for row in manifest["packets"]:
            path = directory / row["file"]
            if path.parent != directory or path.suffix != ".bin":
                raise ValueError("Physics packet filename escapes its directory")
            packet = path.read_bytes()
            digest = sha(packet)
            if len(packet) != row["bytes"]:
                raise ValueError("Physics packet extent changed")
            key = digest, row["address"], row["datum"]
            if key in seen:
                result = dict(**row, map=manifest["map"], packet_sha256=digest, duplicate_of=seen[key])
            else:
                detail = oracle.verify(packet, row["datum"], row["address"], path)
                result = dict(**row, map=manifest["map"], packet_sha256=digest, packet_path=str(path.resolve()), **detail)
                seen[key] = len(results)
            results.append(result)
        print(directory.name, "packets", len(manifest["packets"]), "unique cumulative", len(seen), flush=True)
    report = dict(schema="H2_MCC_PHYSICS_TU15_CONSTRUCTOR_ORACLE_V1", reference_xbe_sha256=PIN,
        original_instruction_patches=False, supplied_callee_results=False, live_world_or_simulation_parity=False,
        cases=len(results), unique_cases=len(seen), rows=results)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n")


if __name__ == "__main__":
    main()
