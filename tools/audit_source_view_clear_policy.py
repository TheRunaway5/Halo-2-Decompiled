#!/usr/bin/env python3
"""Read-only original-byte render-view clear/target producer inventory.

Writes only the named new analysis directory. This is disassembly and immutable
capture-bank inspection, not original execution or a native clear scheduler.
"""
from pathlib import Path
import argparse
import csv
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools/reference/halo2-decompiled/tools"))
from xbe import Xbe, FUNCTIONS_CSV
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM, X86_OP_MEM


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "analysis/multiplayer-shading/view-clear-policy")
    args = parser.parse_args()
    out = args.output.resolve()
    out.mkdir(parents=True, exist_ok=True)
    names = [
        "assets/disc/default.xbe", "tools/audit_source_view_clear_policy.py",
        "tools/reference/halo2-decompiled/tools/xbe.py",
        "tools/reference/halo2-decompiled/config/functions.csv",
        "tools/reference/halo2-decompiled/src/unknown_01cf50.cpp",
        "analysis/multiplayer-shading/sky-stencil-chronology/source-blocks-and-layer-table.json",
        "tools/differential_source_object_admission_snapshot.py",
        "tools/reference/halo2-decompiled/src/unknown_0350e0.cpp",
        "tools/reference/halo2-decompiled/src/unknown_2b116a.cpp",
        "analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained/tree.json",
        "analysis/multiplayer-shading/stipple-coordinate-audit/xemu-retained/hw/xbox/nv2a/nv2a_regs.h",
    ]
    funcs = (0x132D0, 0x13450, 0x13680, 0x13C20, 0x14AC0, 0x14B60, 0x14BC0,
             0x1CD30, 0x28950, 0x28980, 0x289E0, 0x2B5D0, 0x2B790, 0x2BA10, 0x2BCD0,
             0x2C340, 0x2C370, 0x2C560, 0x2C790, 0x2C8A0, 0x2CA00,
             0x25D50, 0x25DC0, 0x335E0, 0x32350, 0x3BD00, 0x3C3C0,
             0x3EDE0, 0x3FDF0, 0x402B0, 0x404B0, 0x40870, 0x41CC0, 0x423C0,
             0x44370, 0x4C690, 0x4D640, 0x12D50, 0x496F0,
             0x2C5224, 0x2C52AF, 0x3F9FC0)
    names += [f"analysis/retail/functions/{va:08x}.c" for va in funcs
              if (ROOT / f"analysis/retail/functions/{va:08x}.c").exists()]
    inputs = {name: {"bytes": (ROOT / name).stat().st_size, "sha256": digest(ROOT / name)}
              for name in names}
    xbe = Xbe(ROOT / "assets/disc/default.xbe")
    assert inputs[names[0]]["sha256"] == "03215919bb7163259257d361f4c7bf802a7ab12aa85e2689436369b5c427935d"
    tree = json.loads((ROOT / names[9]).read_text())
    assert tree["sha"] == "fc24584ce88f0915ad7f04775bb7712c2e3f49ee" and not tree["truncated"]
    nv_path = "hw/xbox/nv2a/nv2a_regs.h"
    blobs = {v["path"]: v["sha"] for v in tree["tree"] if v["type"] == "blob"}
    nv_bytes = (ROOT / names[10]).read_bytes()
    assert hashlib.sha1(b"blob " + str(len(nv_bytes)).encode() + b"\0" + nv_bytes).hexdigest() == blobs[nv_path]
    for token in ("NV097_CLEAR_SURFACE_Z", "NV097_CLEAR_SURFACE_STENCIL", "NV097_CLEAR_SURFACE_A"):
        assert token.encode() in nv_bytes
    rows = {int(row["va"], 16): row for row in csv.DictReader(Path(FUNCTIONS_CSV).open())}
    dis = Cs(CS_ARCH_X86, CS_MODE_32)
    dis.detail = True
    text = []
    blocks = {}
    function_calls = {}
    instructions = {}
    for va in funcs:
        raw = xbe.read(va, int(rows[va]["size"]))
        blocks[f"{va:08X}"] = {"bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest()}
        text.append(f"FUNCTION {va:08X} (original bytes; embedded tables may be displayed as instructions)")
        decoded = list(dis.disasm(raw, va))
        text += [f"{i.address:08X} {i.bytes.hex():24s} {i.mnemonic} {i.op_str}" for i in decoded]
        instructions.update({i.address: i for i in decoded})
        function_calls[f"{va:08X}"] = [{"pc": f"{i.address:08X}", "target": f"{i.operands[0].imm & 0xFFFFFFFF:08X}"}
            for i in decoded if i.mnemonic == "call" and len(i.operands) == 1 and i.operands[0].type == X86_OP_IMM]
    (out / "source-disassembly.txt").write_text("\n".join(text) + "\n")

    # These literal anchors validate the annotations below against executable
    # instructions; they do not execute a caller or supply runtime branch state.
    phase_anchors = {
        0x2BC1B: "call 0x132d0", 0x2BC77: "call 0x13680",
        0x13788: "call 0x2bcd0", 0x2BD3B: "mov edi, 1",
        0x2BE01: "call 0x14b60", 0x2BE6D: "mov dword ptr [0x467130], 3",
        0x2BF69: "mov dword ptr [0x467130], 8", 0x2BFCD: "mov dword ptr [0x467130], 0x14",
        0x2C021: "call 0x335e0", 0x2C026: "mov edi, 7",
        0x2C04D: "mov edi, 2", 0x2C0B2: "mov ebx, 2", 0x2C0BC: "call 0x3ede0",
        0x2C0C1: "xor ebx, ebx", 0x2C0C3: "call 0x15370",
        0x2C0D1: "call 0x25d50", 0x2C0D6: "mov edi, 0x10",
        0x2C109: "mov edi, 6", 0x2C132: "call 0x3c3c0",
        0x3C510: "mov eax, 0x10", 0x3C515: "call 0x14b60",
        0x2C169: "call 0x36b50", 0x2C16E: "mov edi, 0xf",
        0x2C1D9: "mov byte ptr [0x485a76], 1", 0x2C2B3: "call 0x3fdf0",
        0x2C2B9: "call 0x404b0", 0x2C2C2: "call 0x3fdf0",
        0x2C2D3: "mov byte ptr [0x485a76], 0", 0x2C2F8: "mov edi, 0x11",
        0x2C322: "call 0x13870", 0x40058: "mov al, byte ptr [0x485a76]",
        0x40064: "mov eax, ebx", 0x40066: "call 0x14b60", 0x40276: "mov edi, 2",
        0x405ED: "mov dword ptr [0x467130], 0xb", 0x40660: "mov dword ptr [0x467130], 0xe",
        0x406CF: "mov dword ptr [0x467130], 0xc", 0x40755: "mov dword ptr [0x467130], 0xd",
    }
    pinned_anchors = []
    for pc, expected in phase_anchors.items():
        i = instructions[pc]
        assert f"{i.mnemonic} {i.op_str}" == expected, (hex(pc), expected, i.mnemonic, i.op_str)
        pinned_anchors.append({"pc": f"{pc:08X}", "instruction": expected, "bytes": i.bytes.hex()})

    # Restrict the global inventory to direct absolute memory operands decoded
    # at known original function starts. Pointer-alias writes remain a boundary.
    global_refs = []
    callsites = []
    clear_callsites = []
    needles = [struct.pack("<I", a) for a in range(0x4670CC, 0x4670D0)]
    sought_calls = {0x132D0, 0x28950, 0x28980, 0x289E0}
    for va, row in rows.items():
        raw = xbe.read(va, int(row["size"]))
        inspect_global = any(needle in raw for needle in needles)
        inspect_calls = any(f"{c:08x}" in row["calls"].lower().split() for c in sought_calls)
        inspect_clear = any(f"{c:08x}" in row["calls"].lower().split() for c in (0x14B60, 0x3F9FC0))
        if not inspect_global and not inspect_calls and not inspect_clear:
            continue
        decoded = list(dis.disasm(raw, va))
        for ordinal_instruction, i in enumerate(decoded):
            if inspect_global:
                for ordinal, operand in enumerate(i.operands):
                    if operand.type != X86_OP_MEM or operand.mem.base or operand.mem.index:
                        continue
                    address = operand.mem.disp & 0xFFFFFFFF
                    if address < 0x4670D0 and 0x4670CC < address + operand.size:
                        global_refs.append({"function": f"{va:08X}", "pc": f"{i.address:08X}",
                            "address": f"{address:08X}", "size": operand.size,
                            "operand": ordinal, "access": operand.access,
                            "bytes": i.bytes.hex(), "instruction": f"{i.mnemonic} {i.op_str}"})
            if i.mnemonic in ("call", "jmp") and len(i.operands) == 1 and i.operands[0].type == X86_OP_IMM:
                target = i.operands[0].imm & 0xFFFFFFFF
                if target in sought_calls:
                    callsites.append({"function": f"{va:08X}", "pc": f"{i.address:08X}", "kind": i.mnemonic,
                                      "target": f"{target:08X}", "bytes": i.bytes.hex()})
                if target in (0x14B60, 0x3F9FC0):
                    clear_callsites.append({"function": f"{va:08X}", "pc": f"{i.address:08X}", "kind": i.mnemonic,
                        "target": f"{target:08X}", "bytes": i.bytes.hex(),
                        "preceding_instruction_context": [f"{q.address:08X} {q.mnemonic} {q.op_str}"
                            for q in decoded[max(ordinal_instruction - 6, 0):ordinal_instruction]]})
    layer_table = []
    previous = json.loads((ROOT / names[5]).read_text())["layer_table"]
    for layer in range(25):
        raw = xbe.read(0x43FA1C + layer * 36, 36)
        v = struct.unpack("<9I", raw)
        entry = {"layer": layer, "target": v[0], "clear_semantic_flags": v[1],
                 "clear_color": v[2], "clear_depth_bits": v[3],
                 "clear_stencil_byte": v[4] & 255, "collection_index": v[5],
                 "raw": raw.hex()}
        assert raw.hex() == previous[layer]["raw"]
        layer_table.append(entry)
    target_dispatch = []
    for target in range(39):
        table_index = xbe.read(0x14F30 + target, 1)[0]
        pc = struct.unpack("<I", xbe.read(0x14ED0 + table_index * 4, 4))[0]
        target_dispatch.append({"target": target, "table_index": table_index, "source_case_pc": f"{pc:08X}"})

    # These are actual immutable title-state observations at two weather call
    # boundaries, not reconstruction of their earlier clear/caller history.
    sys.path.insert(0, str(ROOT / "tools"))
    from differential_source_object_admission_snapshot import Snapshot
    snapshots = []
    for snapshot_name in ("lockout-weather-full-entry", "lockout-weather-particle-entry"):
        path = ROOT / "analysis/multiplayer-shading/original-weather-20261009/session1" / snapshot_name
        snapshot = Snapshot(path)
        assert snapshot.report["registers_before"] == snapshot.report["registers_after"]
        for suffix in ("export.json", "exports/physical-ram.bin"):
            relative = str((path / suffix).relative_to(ROOT))
            inputs[relative] = {"bytes": (path / suffix).stat().st_size, "sha256": digest(path / suffix)}
        view = snapshot.read(0x485600, 0x298)
        pair = snapshot.read(0x4670CC, 4)
        stack_index = struct.unpack("<i", snapshot.read(0x46701C, 4))[0]
        observations = {"directory": str(path.relative_to(ROOT)), "registers": snapshot.report["registers_before"],
            "global4670CC_word": pair.hex(), "byte4670CD": pair[1],
            "view_header_first24_hex": view[:24].hex(),
            "view_mode_short2": struct.unpack_from("<h", view, 2)[0],
            "view_clear_short8": struct.unpack_from("<h", view, 8)[0],
            "view_stack_index": stack_index,
            "current_main_target4858B8": struct.unpack("<i", snapshot.read(0x4858B8, 4))[0],
            "current_layer_target4858B4": struct.unpack("<i", snapshot.read(0x4858B4, 4))[0],
            "depth_surface509364": snapshot.read(0x509364, 4).hex(),
            "view_camera_viewport_short48": list(struct.unpack_from("<4h", view, 0x48)),
            "layer467130": struct.unpack("<i", snapshot.read(0x467130, 4))[0],
            "overrides485A75_76": snapshot.read(0x485A75, 2).hex(),
            "source_context4BA014_word": snapshot.read(0x4BA014, 4).hex(),
            "source_first_person4BA018_byte": snapshot.read(0x4BA018, 1).hex(),
            "world_collection4C1A50_mask": snapshot.read(0x4C1A50, 4).hex(),
            "first_person_collection4C1A68_mask": snapshot.read(0x4C1A68, 4).hex(),
            "first_person_count509440": struct.unpack("<i", snapshot.read(0x509440, 4))[0],
            "limit": "Actual current banks at captured weather entry, not original20 phase, earlier clear history or a physical Xbox measurement."}
        if stack_index >= 0:
            saved = snapshot.read(0x4850D0 + stack_index * 0x298, 0x298)
            observations["active_saved_view_matches_current_full298"] = saved == view
            observations["saved_clear_short8"] = struct.unpack_from("<h", saved, 8)[0]
        snapshots.append(observations)
    for name, pinned in inputs.items():
        assert digest(ROOT / name) == pinned["sha256"], f"input changed: {name}"
    report = {"scope": "CPU original-byte disassembly/direct-reference inventory only; no emulation, GPU, guest I/O or native activation.",
        "blocks": blocks, "direct_global4670CC_refs": global_refs, "direct_callsites": callsites,
        "function_direct_calls": function_calls, "phase_literal_anchors": pinned_anchors,
        "known_direct_clear_callsites": clear_callsites,
        "ordinary_main_phase_contract": {
            "initialization_order": "2BA10 calls132D0 at2BC1B before13680 at2BC77; early caller failures and saved-view restoration remain actual source branches.",
            "caller": "13680 normal13776..13788 supplies stack arguments (1,1,caller_flags3,caller_flags4,0,0). Alternate source branches remain separate.",
            "ordinary_2BCD0_param1_1_initial_layers": [1, 3, 5, 8, 20],
            "ordinary_following_phases": ["2C370(0)", "335E0", "layer7", "31C80", "2C370(1)", "layer2", "2C370(2..6)",
                "conditional dedicated sky mode2 3EDE0/4C690 then mode0", "25D50/layer16", "conditional layer6/3C3C0", "25DC0/2C340",
                "C3E90/36B50", "scheduled layer15", "conditional25DC0", "485A76=1", "conditional2AFA0", "3FDF0(phase0)",
                "404B0", "3FDF0(phase1)", "conditional40870", "485A76=0", "conditional2DD30", "conditional layer17", "20770/13870"],
            "late_3FDF0_phase0_layers": [1, 3, 8, 20, 7, 2], "late_3FDF0_phase1_layers": [16],
            "first_person_404B0_layers": [11, 14, 12, 13],
            "late_layer1_clear": "Only semantic1 color, gated by both override bytes. 2BCD0 sets485A76=1 throughout the late/FP bracket, suppressing it.",
            "first_person_depth": "404B0 reuses14BC0 with AL1; 4D640 uses12D50 for flagged projection/depth-range state, not attachment clearing.",
            "additional_fog_clear": "3C3C0 conditional3C515 is semantic10 alpha only after actual scenario/resource readiness and caller gates.",
            "clear_inventory_limit": "Known direct executable call/jump sites only, with literal preceding instructions retained. It is not a transitive executed-call trace and does not close indirect/runtime SDK reset or caller histories.",
            "no_pre_sky_DS_clear": "No depth/stencil clear appears in the named 2BCD0/3EDE0/4C690 chronology; the initial layer1/3 shared history survives through dedicated sky.",
            "no_first_person_DS_clear": "The named 3FDF0/402B0/404B0/4D640/12D50 paths have no depth/stencil attachment-clear call. Layer masks, owner/part admission and caller context remain required."
        },
        "reference_inventory_limit": "Known function starts/direct absolute operands only. Pointer aliases, runtime dispatch and actual invocation history are not inferred.",
        "layer_table": layer_table, "layer_table_matches_prior_frozen_audit": True,
        "target_dispatch": target_dispatch, "actual_snapshot_observations": snapshots,
        "verified_main_shared_depth": {"targets": [0, 1, 3, 18], "source_depth_pointer_global": "00509364",
            "case_load_pcs": ["00014BEB", "00014C41", "00014C27", "00014C56"],
            "use_depth_gate": "14D37..14D42 selects the depth pointer only when actual 14BC0 AL input is nonzero.",
            "limit": "Static binding contract and actual captured pointer observations, not a native resource-allocation implementation."},
        "clear_mask_mapping": {"semantic1": "F0 all RGBA color lanes", "semantic10": "80 alpha only",
            "semantic20": "01 depth", "semantic40": "02 stencil", "semantic61": "F3 RGBA/depth/stencil",
            "authority": "14B60 literal bit arithmetic; original3F9FC0 passes flags to NV097_CLEAR_SURFACE; pinned NV2A register labels.",
            "physical_Xbox_precision_claim": False},
        "inputs": inputs, "inputs_unchanged": True}
    (out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps({"report": str(out / "report.json"), "sha256": digest(out / "report.json"),
                      "inputs_unchanged": len(inputs), "global_references": len(global_refs),
                      "callsites": len(callsites), "phase_anchors": len(pinned_anchors), "direct_clear_sites": len(clear_callsites)}))


if __name__ == "__main__":
    main()
