#!/usr/bin/env python3
"""Serialize paused original menu node banks without changing their float bits.

Root construction executes originalBDC40 with captured object fields. Only
unparented, unmirrored objects without synchronization overrides are accepted.
Original world-node banks are captured outputs, not reconstructed goldens.
"""
import argparse
import hashlib
import json
import struct
from pathlib import Path
from oracle_multiplayer_hud_positions import Retail, ROOT, PIN
from inspect_multiplayer import Cache
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_EBX, UC_X86_REG_ECX

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("snapshots", nargs="+", type=Path)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
r = Retail()
cache = Cache(ROOT/'assets/disc/maps/mainmenu.map')
rows, records, stages = [], [], []
for path in args.snapshots:
    report = json.loads(path.read_text())
    label_prefix = report.get('label', path.stem)
    objects = report.get('objects')
    if objects is None:
        # Render-stage capture has one object and an explicitly selected bank.
        # Its primary/controller bytes and supplied bank are retained inputs,
        # not converted from a timestamp or regenerated from source poses.
        obj = dict(report)
        manager = bytes.fromhex(obj['manager_hex'])
        assert bytes.fromhex(obj['primary_raw_hex']) == manager[:32]
        obj['channels'] = [dict(graph=f'{struct.unpack_from("<I", manager, i*32)[0]:08x}',
            animation=f'{struct.unpack_from("<I", manager, i*32+4)[0]:08x}',
            raw_hex=manager[i*32:(i+1)*32].hex()) for i in range(3)]
        header = bytes.fromhex(obj['object_header_hex'])
        obj['object_root_hex'] = header[0x64:0xa4].hex()
        obj['world_node_bank_hex'] = obj['supplied_or_selected_world_bank_hex']
        obj['node_bytes'] = len(bytes.fromhex(obj['world_node_bank_hex']))
        objects = [obj]
    for obj in objects:
        header = bytes.fromhex(obj["object_header_hex"])
        assert len(header) >= 0x130
        flags, parent = struct.unpack_from("<I", header, 4)[0], struct.unpack_from("<I", header, 0x14)[0]
        sync = struct.unpack_from("<I", header, 0xd4)[0]
        assert parent == 0xffffffff and not flags & 0x400, "parented or mirrored root is not covered"
        assert sync == 0xffffffff or not header[0xd8] & 1, "synchronization override is not covered"
        raw = bytes.fromhex(obj["object_root_hex"])
        assert raw == header[0x64:0xa4]
        source = r.alloc(raw)
        out = r.alloc(bytes(52))
        scale = struct.unpack_from("<I", raw, 0x3c)[0]
        # BD090[BD49B,BD4D1): up=EAX,forward=ECX; position/scale/mirror/
        # parent-matrix/out are the five stack inputs, no root offset tuning.
        r.run(0xbdc40, argv=[source, scale, 0, 0, out],
              registers=[(UC_X86_REG_EAX, source+0x18),
                         (UC_X86_REG_ECX, source+0xc), (UC_X86_REG_EBX, 0)])
        root = bytes(r.g.mem_read(out, 52))
        channel = bytes.fromhex(obj["channels"][0]["raw_hex"])
        graph, packed = struct.unpack_from("<2I", channel)
        assert packed & 0xffff == 0xffff and graph != 0xffffffff, "inherited/empty clip is not covered"
        assert all(c["graph"] == "ffffffff" and c["animation"] == "ffffffff"
                   for c in obj["channels"][1:]), "multiple active controllers are not covered"
        bank = bytes.fromhex(obj["world_node_bank_hex"])
        count = obj["node_count"]
        assert len(bank) == count*52 == obj["node_bytes"]
        frame = struct.unpack_from("<I", channel, 0x1c)[0]
        label = label_prefix+"-"+obj["object"]
        assert all(c.isalnum() or c in "-_" for c in label)
        words = [*struct.unpack("<13I", root), *struct.unpack("<"+str(count*13)+"I", bank)]
        rows.append(" ".join([label, obj["definition"], f"{graph:08x}", str(packed>>16),
                              f"{frame:08x}", str(count), *(f"{w:08x}" for w in words)]))
        deferred = bool(flags & (1 << 30))
        mask = []
        if deferred:
            definition = cache.tags[int(obj['definition'], 16)]['offset']
            model = cache.tags[cache.u(definition+0x38)]['offset']
            mask = [(~cache.u(model+0xa4+i*4))&0xffffffff for i in range(8)]
        control=bytes.fromhex(obj.get('control_hex',''))
        slot=struct.unpack_from('<I',control,0x80)[0] if len(control)>=0x84 else None
        stages.append(' '.join([label, f'{flags:08x}', str(len(mask)), *(f'{v:08x}' for v in mask),
            *([f'{slot:08x}'] if slot is not None else [])]))
        records.append(dict(label=label, snapshot=str(path.resolve()),
            snapshot_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
            object=obj["object"], definition=obj["definition"], graph=f"{graph:08x}",
            clip_index=packed>>16, frame_bits=f"{frame:08x}", root_hex=root.hex(),
            bank_sha256=hashlib.sha256(bank).hexdigest(), node_count=count,
            object_flags=f'{flags:08x}', deferred_flag_30=deferred,
            aim_slot_0_hex=control[0x80:0x84].hex() if slot is not None else None,
            captured_bank_stage='object-update bank with deferred nodes' if deferred else
                'object bank after deferred-node sampling; rendered interpolation not asserted',
            root_method="OriginalBDC40; captured64/70/7C/A0 fields; unparented,unmirrored,no sync override"))
args.output.parent.mkdir(parents=True, exist_ok=True)
args.output.write_text("# Paused original menu world-node banks. Hex fields preserve float bits.\n"+
                       "\n".join(rows)+"\n")
stage_path = args.output.with_suffix('.stages.tsv')
stage_path.write_text('# Captured object flags and original hlmt+A4 complement for deferred banks.\n'+
                      '\n'.join(stages)+'\n')
args.output.with_suffix(".provenance.json").write_text(json.dumps(dict(
    xbe_sha256=PIN, fixture_sha256=hashlib.sha256(args.output.read_bytes()).hexdigest(),
    records=records, stage_fixture_sha256=hashlib.sha256(stage_path.read_bytes()).hexdigest(),
    limits="Snapshot captures actual produced banks. Unit callback/IK/physics or other unrecorded pose producers are not asserted suppressed; mismatches remain unresolved."), indent=2)+"\n")
print(f"{len(rows)} exact captured objects serialized to {args.output}")
