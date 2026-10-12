#!/usr/bin/env python3
"""Read-only authored weapon/effect/marker inventory for the 21 imported MP maps.

No clip names select an effect. Exact tag references, block headers and rows
are retained. Schema labels describe fields; executable ownership/execution
is a separate, explicitly bounded source in the accompanying research note.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import csv
from inspect_multiplayer import Cache, MAPS, ROOT

NONE = 0xffffffff
DLC = {"00000001": ("containment", "warlock"),
       "00000002": ("turf", "deltatap"),
       "00000003": ("triplicate", "gemini", "elongation", "dune", "backwash")}


def sha(path):
    h = hashlib.sha256()
    with path.open("rb") as f:
        for data in iter(lambda: f.read(1024 * 1024), b""):
            h.update(data)
    return h.hexdigest()


def hexword(value):
    return f"{value:08x}"


class Inventory:
    def __init__(self, cache):
        self.c = cache
        self.effects = {}
        self.models = {}

    def raw(self, at, count):
        assert 0 <= at and at + count <= len(self.c.data)
        return dict(file_offset=hexword(at), original_va=hexword(at + self.c.magic),
                    byte_count=count, bytes=bytes(self.c.data[at:at + count]).hex())

    def sid(self, at):
        value = self.c.u(at)
        return dict(word=hexword(value), text=self.c.text(
            self.c.u(368) + self.c.u(self.c.u(364) + (value & 0xffff) * 4)))

    def ref(self, at):
        datum = self.c.u(at + 4)
        result = dict(**self.raw(at, 8), group_word=hexword(self.c.u(at)),
                      datum=hexword(datum), target=self.c.reference(datum))
        if datum != NONE and result["target"]["group"] == "effe":
            self.effect(datum)
        return result

    def block(self, at, stride):
        return self.c.block(at, stride), self.raw(at, 8)

    def location(self, index, locations):
        return locations[index]["marker"] if 0 <= index < len(locations) else None

    def effect(self, datum):
        key = hexword(datum)
        if key in self.effects:
            return
        tag = self.c.tags[datum]
        root = tag["offset"]
        result = dict(tag=tag, resident=root is not None)
        self.effects[key] = result  # Break nested-effect cycles.
        if root is None:
            return
        result["root"] = self.raw(root, 0x30)
        result["flags"] = hexword(self.c.u(root))
        locations, result["locations_header"] = self.block(root + 0xc, 4)
        locations = [dict(index=i, raw=self.raw(at, 4), marker=self.sid(at))
                     for i, at in enumerate(locations)]
        result["locations"] = locations
        events, result["events_header"] = self.block(root + 0x14, 0x38)
        result["events"] = []
        for i, at in enumerate(events):
            event = dict(index=i, raw=self.raw(at, 0x38),
                         flags=hexword(self.c.u(at)),
                         skip_fraction_word=hexword(self.c.u(at + 4)),
                         delay_words=[hexword(self.c.u(at + x)) for x in (8, 12)],
                         duration_words=[hexword(self.c.u(at + x)) for x in (16, 20)])
            parts, event["parts_header"] = self.block(at + 0x18, 0x38)
            event["parts"] = [dict(index=j, raw=self.raw(p, 0x38),
                environment=self.c.h(p), disposition=self.c.h(p + 2),
                location_index=self.c.h(p + 4),
                location=self.location(self.c.h(p + 4), locations),
                flags=self.c.h(p + 6) & 0xffff, reference=self.ref(p + 0xc))
                for j, p in enumerate(parts)]
            systems, event["systems_header"] = self.block(at + 0x30, 0x38)
            event["systems"] = []
            for j, p in enumerate(systems):
                emitters, header = self.block(p + 0x30, 0xb8)
                event["systems"].append(dict(index=j, raw=self.raw(p, 0x38),
                    particle=self.ref(p), location_index=self.c.h(p + 8),
                    location=self.location(self.c.h(p + 8), locations),
                    coordinate_system=self.c.h(p + 0xc),
                    environment=self.c.h(p + 0xe), disposition=self.c.h(p + 0x10),
                    camera_mode=self.c.h(p + 0x12), flags=self.c.h(p + 0x16) & 0xffff,
                    emitters_header=header,
                    emitters=[dict(index=k, raw=self.raw(e, 0xb8))
                              for k, e in enumerate(emitters)]))
            # Beam/acceleration counts are retained so zero particle count is
            # never promoted into an assertion of no authored visual content.
            event["beam_count"] = self.c.u(at + 0x20)
            event["acceleration_count"] = self.c.u(at + 0x28)
            result["events"].append(event)

    def model(self, datum):
        if datum == NONE:
            return None
        key = hexword(datum)
        if key in self.models:
            return key
        tag = self.c.tags[datum]
        assert tag["group"] == "mode", tag
        root = tag["offset"]
        result = dict(tag=tag, resident=root is not None)
        self.models[key] = result
        if root is None:
            return key
        nodes, result["nodes_header"] = self.block(root + 0x48, 0x60)
        result["nodes"] = [dict(index=i, name=self.sid(at),
            parent=self.c.h(at + 4), raw=self.raw(at, 0x60))
            for i, at in enumerate(nodes)]
        groups, result["marker_groups_header"] = self.block(root + 0x58, 0xc)
        result["marker_groups"] = []
        for i, at in enumerate(groups):
            markers, header = self.block(at + 4, 0x24)
            result["marker_groups"].append(dict(index=i, name=self.sid(at),
                raw=self.raw(at, 0xc), markers_header=header,
                markers=[dict(index=j, raw=self.raw(p, 0x24),
                    region=struct.unpack_from('<b', self.c.data, p)[0],
                    permutation=struct.unpack_from('<b', self.c.data, p + 1)[0],
                    node=struct.unpack_from('<b', self.c.data, p + 2)[0],
                    translation_words=[hexword(self.c.u(p + x)) for x in (4, 8, 12)],
                    quaternion_words=[hexword(self.c.u(p + x)) for x in (16, 20, 24, 28)],
                    scale_word=hexword(self.c.u(p + 0x20)))
                    for j, p in enumerate(markers)]))
        return key

    def weapon(self, tag):
        root = tag["offset"]
        out = dict(tag=tag, resident=root is not None)
        if root is None:
            return out
        out["root"] = self.raw(root, 0x31c)
        out["flags_12c"] = hexword(self.c.u(root + 0x12c))
        out["prepared_callback_scalar_154_word"] = hexword(self.c.u(root + 0x154))
        out["misfire_time_25c_word"] = hexword(self.c.u(root + 0x25c))
        out["weapon_child_marker"] = self.sid(root + 0x284)
        own = self.c.u(root + 0x38)
        out["world_hlmt"] = self.c.reference(own)
        out["world_mode"] = None
        if own != NONE:
            assert self.c.tags[own]["group"] == "hlmt"
            at = self.c.tags[own]["offset"]
            if at is not None:
                out["world_hlmt_model_ref"] = self.raw(at, 8)
                out["world_mode"] = self.model(self.c.u(at + 4))
        fp, out["first_person_header"] = self.block(root + 0x2a8, 0x10)
        out["first_person"] = [dict(index=i, raw=self.raw(at, 0x10),
            model=self.model(self.c.u(at + 4)), animation=self.ref(at + 8))
            for i, at in enumerate(fp)]
        attachments, out["attachments_header"] = self.block(root + 0x94, 0x18)
        out["attachments"] = [dict(index=i, raw=self.raw(at, 0x18),
            reference=self.ref(at), marker=self.sid(at + 8))
            for i, at in enumerate(attachments)]
        out["ready_effect"] = self.ref(root + 0x140)
        out["ready_damage_effect"] = self.ref(root + 0x148)
        mags, out["magazines_header"] = self.block(root + 0x2c0, 0x5c)
        out["magazines"] = [dict(index=i, raw=self.raw(at, 0x5c),
            initial_rounds=self.c.h(at + 6), capacity=self.c.h(at + 0xa),
            reserve=self.c.h(at + 0xc), reloading_effect=self.ref(at + 0x34),
            chambering_effect=self.ref(at + 0x44)) for i, at in enumerate(mags)]
        triggers, out["triggers_header"] = self.block(root + 0x2c8, 0x40)
        out["triggers"] = [dict(index=i, raw=self.raw(at, 0x40),
            charging_effect=self.ref(at + 0x30), charging_damage_effect=self.ref(at + 0x38))
            for i, at in enumerate(triggers)]
        barrels, out["barrels_header"] = self.block(root + 0x2d0, 0xec)
        out["barrels"] = []
        for i, at in enumerate(barrels):
            rows, header = self.block(at + 0xe4, 0x34)
            out["barrels"].append(dict(index=i, raw=self.raw(at, 0xec),
                flags=hexword(self.c.u(at)), magazine_index=self.c.h(at + 0x28),
                optional_marker=self.sid(at + 0x30), projectile=self.ref(at + 0x8c),
                illumination_recovery_word=hexword(self.c.u(at + 0xa0)),
                firing_effects_header=header,
                firing_effects=[dict(index=j, raw=self.raw(p, 0x34),
                    count_bounds=[self.c.h(p), self.c.h(p + 2)],
                    **{name: self.ref(p + 4 + k * 8)
                       for k, name in enumerate(('normal', 'misfire', 'empty'))},
                    **{name + '_damage': self.ref(p + 0x1c + k * 8)
                       for k, name in enumerate(('normal', 'misfire', 'empty'))})
                    for j, p in enumerate(rows)]))
        out["normal_firing_reference_absent"] = all(
            row["normal"]["datum"] == 'ffffffff'
            for barrel in out["barrels"] for row in barrel["firing_effects"])
        return out

    def starting(self):
        c = self.c
        out = {"scenario_slayer_rows": []}
        for i, p in enumerate(c.block(c.scenario + 0x128, 0x9c)):
            modes = [c.h(p + 4 + n * 2) for n in range(4)]
            if any(n == 2 or 12 <= n <= 14 for n in modes):
                datum = c.u(p + 0x40)
                out["scenario_slayer_rows"].append(dict(index=i, raw=self.raw(p, 0x9c),
                    modes=modes, collection=c.collection(datum)))
        root = next(t['offset'] for t in c.tags.values() if t['group'] == 'matg')
        datum = c.u(root + 0x16c)
        runtimes = c.block(c.tags[datum]['offset'] + 8, 0x568)
        assert len(runtimes) == 1
        out["runtime_weapon_rows"] = [dict(index=i, raw=self.raw(p, 8),
            weapon=self.ref(p)) for i, p in enumerate(c.block(runtimes[0] + 0x68, 8))]
        return out


def run():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--instructions', type=Path,
                    help='Optional complete original TU1.1 owning listings (requires Capstone).')
    args = ap.parse_args()
    paths = [ROOT / 'assets/disc/maps' / (name + '.map') for name in MAPS]
    paths += [ROOT / 'assets/multiplayer-map-pack/demos/dlc' / bank / (name + '.map')
              for bank, names in DLC.items() for name in names]
    report = dict(format='authored-mp-weapon-effects-v1',
        boundary='Tag-reference/row inventory; no generic live effect, camera, marker or particle execution claim.',
        sources={str(p.relative_to(ROOT)): sha(p) for p in [Path(__file__),
            ROOT / 'tools/inspect_multiplayer.py', *[ROOT / 'tools/reference/assembly/src/Assembly/Plugins/Halo2' / (name + '.xml')
                for name in ('weap', 'effe', 'mode')]]}, maps=[])
    for path in paths:
        c = Cache(path)
        inv = Inventory(c)
        weapons = [inv.weapon(t) for t in c.tags.values() if t['group'] == 'weap']
        report['maps'].append(dict(path=str(path.relative_to(ROOT)), sha256=sha(path),
            scenario_type_i16_at_10=c.h(c.scenario + 0x10),
            weapons=weapons, effects=inv.effects, models=inv.models, starting=inv.starting()))
        print(f'{path.stem}: {len(weapons)} weapons, {len(inv.effects)} effects, {len(inv.models)} models', flush=True)
        c.data.close()
        c.stream.close()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')
    if args.instructions:
        instruction_inventory(args.instructions)


def instruction_inventory(directory):
    """The decompiler index supplies entry boundaries, never field semantics."""
    sys.path.insert(0, str(ROOT / 'tools/reference/halo2-decompiled/tools'))
    from xbe import Xbe
    from functions import _Code, _trace
    image_path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(image_path) == 'aaea4c9c58ac02cf5f9cc97a16d688318c59b652e92301078e2547a2a5ea5377'
    index = ROOT / 'analysis/map-pack-update/functions.tsv'
    with index.open() as f:
        starts = {int(row['address'], 16) for row in csv.DictReader(f, delimiter='\t')}
    code = _Code(Xbe(image_path), '.text')
    report = dict(image_sha256=sha(image_path), boundary_index_sha256=sha(index),
                  execution='Static complete instruction listing only; no new kernel execution.',
                  functions=[])
    directory.mkdir(parents=True, exist_ok=True)
    for entry in (0x1040f0, 0x106070, 0x106c30, 0x106e70, 0x107160, 0x107b50, 0x107e90, 0x1099f0):
        fn = _trace(code, entry, starts)
        lines = []
        for address in sorted(fn.code):
            ins = code.at(address)
            assert ins.size == fn.code[address]
            lines.append(f'{address:08x} {ins.bytes.hex()} {ins.mnemonic} {ins.op_str}'.rstrip())
        path = directory / f'{entry:08x}.asm'
        path.write_text('\n'.join(lines) + '\n')
        report['functions'].append(dict(entry=hexword(entry), end=hexword(fn.end),
            instruction_count=len(lines), sha256=sha(path),
            calls=[dict(site=hexword(site), target=hexword(target), kind=kind)
                   for site, target, kind in sorted(fn.sites)]))
    (directory / 'report.json').write_text(json.dumps(report, indent=2, sort_keys=True) + '\n')


if __name__ == '__main__':
    run()
