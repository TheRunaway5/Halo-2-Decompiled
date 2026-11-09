"""Inventory all literal prediction callers in the installed E3 map variants.

Authenticate complete map bytes against the completed installed sweep, preserve
full script-node salts and tag identities, and read each predicted resource's
authored metadata. This is content inventory, not runtime/native acceptance.
Run through run_test.py and retain only bounded metadata.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct

from mcc_content import MccCache
import steam_workshop as workshop


FEATURES = {119: ('shader_predict', 40, 'shad'), 120: ('bitmap_predict', 39, 'bitm'),
            607: ('sound_looping_predict', 35, 'lsnd')}


def digest(raw):
    return hashlib.sha256(raw).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sweep', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    assert not args.report.resolve().is_relative_to(owner)
    sweep = json.loads(args.sweep.read_text())
    assert sweep['completed']
    mod = next(row for row in sweep['mods'] if row['id'] == '3360515088')
    packages = [path / mod['id'] for path in workshop.roots() if (path / mod['id']).is_dir()]
    assert len(packages) == 1
    maps = workshop.source_owned(packages[0], 'maps')
    rows = []
    for entry in mod['maps']:
        path = maps / entry['map']
        before = workshop.pin(path)
        assert before == entry['source']
        cache = MccCache(path)
        try:
            scenario = cache.tags[cache.u32(cache.index + 12)]
            base = scenario['offset']

            def bank(field, stride):
                count = cache.u32(base + field)
                if not count:
                    return b''
                offset = cache.pointer(cache.u32(base + field + 4), cache.magic, count * stride)
                return bytes(cache.data[offset:offset + count * stride])

            nodes, strings = bank(0x238, 20), bank(0x1b0, 1)

            def node(handle):
                index = handle & 65535
                assert index < len(nodes) // 20
                result = struct.unpack_from('<4H3I', nodes, index * 20)
                assert result[0] == handle >> 16 and result[3] != 0xbaba
                return result

            calls, resources = [], {}
            for index, values in enumerate(struct.iter_unpack('<4H3I', nodes)):
                salt, opcode, _, flags, _, _, function = values
                if not salt or flags == 0xbaba or flags & 7 or opcode not in FEATURES:
                    continue
                name, kind, group = FEATURES[opcode]
                fn = node(function)
                assert fn[2] == 2 and fn[3] & 1
                end = strings.index(b'\0', fn[5])
                assert strings[fn[5]:end].decode('ascii') == name
                argument = node(fn[4])
                assert argument[2] == kind and argument[4] == 0xffffffff
                assert argument[3] & 7 == 1, 'Prediction argument needs actual expression evaluation'
                handle = argument[6]
                calls.append(dict(root=(salt << 16) | index, function=function, argument=fn[4],
                                  name=name, opcode=opcode, resource=handle))
                if handle == 0xffffffff or handle in resources:
                    continue
                tag = cache.tags[handle]
                assert tag['group'] == group and tag['offset'] is not None
                resource = dict(handle=handle, name=tag['name'], group=group)
                offset = tag['offset']
                if group == 'bitm':
                    count = cache.u32(offset + 0x44)
                    first = cache.pointer(cache.u32(offset + 0x48), cache.magic, count * 168)
                    resource['frames'] = count
                    resource['frame_metadata_sha256'] = digest(bytes(cache.data[first:first + count * 168]))
                    resource['frame_layout'] = [dict(index=frame,
                        width=cache.u16(first + frame * 168 + 4),
                        height=cache.u16(first + frame * 168 + 6),
                        depth=cache.u16(first + frame * 168 + 8),
                        image_type=cache.u16(first + frame * 168 + 10),
                        format=cache.u16(first + frame * 168 + 12),
                        flags=cache.u16(first + frame * 168 + 14)) for frame in range(count)]
                elif group == 'lsnd':
                    count = cache.u32(offset + 0x24)
                    first = cache.pointer(cache.u32(offset + 0x28), cache.magic, count * 0x58)
                    tracks = []
                    for track in range(count):
                        sound = cache.u32(first + track * 0x58 + 0x20)
                        selected = cache.tags[sound] if sound != 0xffffffff else None
                        assert selected is None or selected['group'] == 'snd!'
                        tracks.append(dict(index=track, first_loop_sound=sound,
                            first_loop_name=selected['name'] if selected else None))
                    resource['tracks'] = tracks
                resources[handle] = resource
            rows.append(dict(map=path.name, source=before, script_nodes_sha256=digest(nodes),
                             calls=calls, resources=list(resources.values())))
        finally:
            cache.close()
        assert workshop.pin(path) == before
    report = dict(scope=__doc__, passed=True, sweep_sha256=digest(args.sweep.read_bytes()),
        helper_sha256=digest(Path(__file__).read_bytes()), maps=rows,
        runtime_qualified=False, production_admission_changed=False, complete_mod_support=False,
        owned_scratch=str(owner), retained_large_artifacts=[])
    raw = (json.dumps(report, indent=2) + '\n').encode()
    assert len(raw) < 1024 * 1024
    args.report.write_bytes(raw)
    print(json.dumps(dict(passed=True, maps=len(rows),
        prediction_calls=[dict(map=row['map'], calls=len(row['calls'])) for row in rows])))


if __name__ == '__main__':
    main()
