#!/usr/bin/env python3
"""Unchanged post-PRT4B5D0/3DB00 instruction admission, no query result stub.

Actual retained Zanzibar owner records/definition bytes are cloned. Initial or
explicit retained cache states exercise admission only; execution stops before
the collision/lightmap query D1850 or conversion D4080 service. The earlier
geometry-cache/PRT producer is an explicit caller boundary, never stubbed.
"""
import hashlib
import json
import struct
import sys
from pathlib import Path
from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
sys.path.insert(0, str(ROOT / "tools"))
from xbe import Xbe
from audit_rendering import Cache
PIN = "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
BASE, STACK, STOP = 0x2100000, 0x22f9000, 0x3000000
POOL, HANDLES, CACHE_POOL, CACHE_RECORDS, TAGS, CONTEXT, OWNER_PTR, BSP, PROBES, DEFAULT = [
    BASE + i for i in (0, 0x10000, 0x20000, 0x30000, 0x40000, 0x90000, 0xa0000, 0xb0000, 0xc0000, 0xd0000)]


def main():
    x = Xbe(ROOT / "assets/disc/default.xbe")
    assert hashlib.sha256(x.data).hexdigest() == PIN
    u = Uc(UC_ARCH_X86, UC_MODE_32)
    u.mem_map(0x10000, 0x600000)
    for section in x.sections:
        u.mem_write(section.va, x.section_bytes(section))
    u.mem_map(BASE, 0x200000)
    u.mem_map(STOP, 0x1000)
    def put(at, *values):u.mem_write(at, struct.pack("<" + "I" * len(values), *(v & 0xffffffff for v in values)))
    def read(at):return struct.unpack("<I", u.mem_read(at, 4))[0]
    put(0x4e0300, POOL);put(POOL + 0x44, HANDLES)
    put(0x509434, CACHE_POOL);put(CACHE_POOL + 0x44, CACHE_RECORDS)
    put(0x4e3b44, TAGS);put(0x4e0348, BSP);put(BSP + 0x230, PROBES)
    put(0x4de2f4, DEFAULT);put(0x4ba050, -1)
    active, reached, end, complete_conversion = [], None, STOP, False
    boundaries = {0xd1850: "D1850 query", 0xd4080: "D4080 conversion"}
    def hook(g, address, size, user):
        nonlocal reached
        if address in (0x3dd10, 0x3db00, 0xbaf80, 0x3ddd0, 0x3dfe0, 0xd4080):active.append(address)
        if address == 0xd1850 or (address == 0xd4080 and not complete_conversion) or address == end:
            reached = address;g.emu_stop()
    u.hook_add(UC_HOOK_CODE, hook)
    retained = ROOT / "analysis/multiplayer-shading/original-xemu"
    source = json.loads((retained / "zanzibar-object-contacts.json").read_text())
    c = Cache(ROOT / "assets/disc/maps/zanzibar.map")
    # The same-frame first-view fixture executes the complete real converter.
    # Its source curves/records are relocated from this original cache.
    SCENARIO, GLDF, RECORDS = BASE + 0xf0000, BASE + 0xe0000, BASE + 0xe1000
    _, scenario = c.tag(c.scenario, "scnr")
    lighting_datum = c.u32(scenario + 0x33c)
    if lighting_datum == 0xffffffff:
        globals_tag = next(t for t in c.tags.values() if t["group"] == "matg")
        _, globals_root = c.tag(globals_tag["datum"], "matg")
        lighting_datum = c.u32(globals_root + 0x184)
    _, lighting_root = c.tag(lighting_datum, "gldf")
    lighting_records = c.block(lighting_root, 144)
    put(0x4e0350, SCENARIO);put(SCENARIO + 0x33c, lighting_datum)
    put(TAGS + (lighting_datum & 0xffff) * 16 + 8, GLDF)
    put(GLDF, len(lighting_records), RECORDS)
    cursor = BASE + 0xe8000
    for index, original in enumerate(lighting_records):
        relocated = bytearray(c.data[original:original + 144])
        for offset in (36, 96, 128, 136):
            size = c.u32(original + offset)
            if size:
                at = c.pointer(c.u32(original + offset + 4), c.magic, size)
                u.mem_write(cursor, c.data[at:at + size])
                struct.pack_into("<I", relocated, offset + 4, cursor)
                cursor += (size + 15) & ~15
        u.mem_write(RECORDS + index * 144, bytes(relocated))
    u.mem_write(0x467464, bytes([0])) # Explicit caller interpolation-disabled state.
    reports = []
    for ordinal, owner in enumerate(o for o in source["contacts"] if o["definition"] in (0xebe20a6d, 0xec080a93, 0xe93107bc)):
        raw = (retained / owner["record"]).read_bytes()
        handle = owner["datum"];cache_handle = struct.unpack_from("<I", raw, 0xcc)[0]
        assert cache_handle != 0xffffffff and struct.unpack_from("<I", raw, 0x14)[0] == 0xffffffff
        assert struct.unpack_from("<h", raw, 0xd0)[0] == -1 and not raw[7] & 1
        address = BASE + 0x60000 + ordinal * 0x1000
        definition = BASE + 0x70000 + ordinal * 0x1000
        _, original_definition = c.tag(owner["definition"], "mach" if owner["object_type"] == 7 else "bloc")
        definition_bytes = c.data[original_definition:original_definition + 0x80]
        assert not definition_bytes[0x1c] & 1 # No change-color update service in this tail.
        u.mem_write(definition, definition_bytes)
        put(TAGS + (owner["definition"] & 0xffff) * 16 + 8, definition)
        put(HANDLES + (handle & 0xffff) * 12 + 8, address)
        record = CACHE_RECORDS + (cache_handle & 0xffff) * 256
        for case, valid, last, frame, frozen, reuse, context, succeeded in (
            ("initial", 0, -1, 1, raw[0xc1] & 1, False, True, True),
            ("initial even frozen", 0, -1, 1, 1, False, True, True),
            ("same query frame", 1, 1, 1, 0, False, True, True),
            ("retained frozen", 1, 0, 1, 1, False, True, True),
            ("context absent", 0, -1, 1, 0, False, False, True),
            ("geometry cache reused", 0, -1, 1, 0, True, True, True),
            ("failed PRT return still reaches owner query", 0, -1, 1, 0, False, True, False),
        ):
            u.mem_write(address, raw);u.mem_write(address + 0xc1, bytes([frozen]))
            u.mem_write(record, bytes(256));put(record + 4, handle, last, -1)
            u.mem_write(record + 2, bytes([valid, valid]))
            u.mem_write(record + 0x10, raw[0x30:0x3c])
            u.mem_write(CONTEXT, bytes(0x200));put(CONTEXT + 0x44, record + 0x44 if context else 0)
            put(OWNER_PTR, handle);put(0x4ba034, frame)
            u.mem_write(STACK, bytes(0x4000));put(STACK + 0x28, OWNER_PTR)
            put(STACK + 0x3414, 0x3f800000) # Explicit source normalized distance1.
            u.mem_write(STACK + 0x13, bytes([reuse]))
            u.reg_write(UC_X86_REG_ESP, STACK);u.reg_write(UC_X86_REG_EBP, CONTEXT)
            u.reg_write(UC_X86_REG_EDI, 0);u.reg_write(UC_X86_REG_EBX, int(succeeded))
            u.reg_write(UC_X86_REG_FPCW, 0x027f);u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
            u.reg_write(UC_X86_REG_FPTAG, 0xffff);u.reg_write(UC_X86_REG_FPSW, 0)
            active, reached, end = [], None, 0x4ba6f
            entry = 0x4b987 if reuse else 0x4ba01
            try:u.emu_start(entry, STOP, count=100000)
            except Exception as e:raise RuntimeError(f"{case} failed at {u.reg_read(UC_X86_REG_EIP):x}") from e
            assert reached is not None
            queried = reached == 0xd1850
            expected = not reuse and context and not valid
            assert queried == expected, (case, reached, active)
            assert (0x3db00 in active) == (context and not reuse)
            if queried:
                sp = u.reg_read(UC_X86_REG_ESP)
                assert read(sp + 8) == record + 0x30
                assert u.reg_read(UC_X86_REG_EDI) == handle
            reports.append(dict(owner=owner["definition_name"], full_handle=handle,
                retained_cache_handle=cache_handle, original_frozen=bool(raw[0xc1] & 1),
                record_sha256=hashlib.sha256(raw).hexdigest(), case=case,
                entry=entry, stopped=reached, calls=active, queried=queried,
                last_query_frame=read(record + 8), raw_cache_state_is_fixture=True))

        # First view: elapsed4 is below interval60 at distance1, so the real
        # converter returns without changing last_query_frame. Second view:
        # the same frame and moved source sphere at distance401 has interval3
        # and reaches the real query. No query result/converter is substituted.
        u.mem_write(address, raw);u.mem_write(address + 0xc1, bytes([0]))
        u.mem_write(record, bytes(256));put(record + 4, handle, 0, 3)
        u.mem_write(record + 2, bytes([1, 1]))
        anchor = list(struct.unpack_from("<3f", raw, 0x30));anchor[0] -= .1
        u.mem_write(record + 0x10, struct.pack("<3f", *anchor))
        sample = struct.pack("<3f2I", .2, .3, .4, 0x607080, 0x405060)
        u.mem_write(record + 0x1c, sample);u.mem_write(record + 0x30, sample)
        u.mem_write(CONTEXT, bytes(0x200));put(CONTEXT + 0x44, record + 0x44)
        put(OWNER_PTR, handle);put(0x4ba034, 4)
        for case, distance, expected in (
            ("same-frame first view cadence suppresses query", 1., False),
            ("same-frame later view cadence admits query", 401., True),
        ):
            u.mem_write(STACK, bytes(0x4000));put(STACK + 0x28, OWNER_PTR)
            u.mem_write(STACK + 0x3414, struct.pack("<f", distance))
            u.reg_write(UC_X86_REG_ESP, STACK);u.reg_write(UC_X86_REG_EBP, CONTEXT)
            u.reg_write(UC_X86_REG_EDI, 0);u.reg_write(UC_X86_REG_EBX, 1)
            u.reg_write(UC_X86_REG_FPCW, 0x027f);u.reg_write(UC_X86_REG_MXCSR, 0x1f80)
            u.reg_write(UC_X86_REG_FPTAG, 0xffff);u.reg_write(UC_X86_REG_FPSW, 0)
            active, reached, end, complete_conversion = [], None, 0x4ba6f, True
            u.emu_start(0x4ba01, STOP, count=100000)
            assert reached is not None and (reached == 0xd1850) == expected
            assert read(record + 8) == (4 if expected else 0)
            if not expected:assert 0xd4080 in active
            reports.append(dict(owner=owner["definition_name"], full_handle=handle,
                retained_cache_handle=cache_handle, original_frozen=bool(raw[0xc1] & 1),
                record_sha256=hashlib.sha256(raw).hexdigest(), case=case,
                entry=0x4ba01, stopped=reached, calls=active, queried=expected,
                normalized_distance=distance, last_query_frame=read(record + 8),
                raw_cache_state_is_fixture=True, complete_original_converter=True))
        complete_conversion = False
    c.close()
    result = dict(xbe_sha256=PIN, cases=len(reports), records=reports,
        scope="Actual retained pure-PRT owner records and definition bytes, with explicit initial/retained cache states. Unchanged4B5D0 post-PRT tail or cached-reuse branch, complete3DD10/3DB00/BAF80/valid-existing3DDD0/3DFE0 instructions up to query or return. Same-frame paired views execute completeD4080 with actual authoredgldf and explicit interpolation-disabled input. No collision result or callee stub. Earlier4BCC0/4C190/137800/23690 services, caller last-draw stamp, and actual current geometry-cache reuse history are separate caller boundaries; cache fixture state is not claimed to be the paused owner's literal cached payload.")
    (ROOT / "analysis/multiplayer-shading/prt-object-light-admission-original.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(dict(cases=len(reports), calls=sum(0x3db00 in r["calls"] for r in reports),
        queries=sum(r["queried"] for r in reports), errors=0), indent=2))


if __name__ == "__main__":main()
