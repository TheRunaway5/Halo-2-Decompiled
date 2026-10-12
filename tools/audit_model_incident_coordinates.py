#!/usr/bin/env python3
"""Retain the literal incident-to-world-basis dataflow of original shaders.

This inspects authored class0/class1 bytecode; it does not run or mutate the
original game, and it does not infer live material/class admission. Dependency
tracking uses the original simultaneous operand-read/write instruction rules.
"""
from pathlib import Path
import hashlib
import json
import struct
from audit_rendering import Cache

ROOT = Path(__file__).resolve().parents[1]
MAC = ['NOP', 'MOV', 'MUL', 'ADD', 'MAD', 'DP3', 'DPH', 'DP4',
       'DST', 'MIN', 'MAX', 'SLT', 'SGE', 'ARL']
ILU = ['NOP', 'MOV', 'RCP', 'RCC', 'RSQ', 'EXP', 'LOG', 'LIT']
USED = {0: (), 1: (0,), 2: (0, 1), 3: (0, 2), 4: (0, 1, 2),
        5: (0, 1), 6: (0, 1), 7: (0, 1), 8: (0, 1), 9: (0, 1),
        10: (0, 1), 11: (0, 1), 12: (0, 1), 13: (0,)}


def operands(t, semantic_ids, registers, outputs):
    y, z, w = t[1:]
    result = []
    for n in range(3):
        if n == 0:
            mux, reg, neg = z >> 26 & 3, z >> 28, y >> 8 & 1
            sw = [y >> 6 & 3, y >> 4 & 3, y >> 2 & 3, y & 3]
        elif n == 1:
            mux, reg, neg = z >> 11 & 3, z >> 13 & 15, z >> 25 & 1
            sw = [z >> 23 & 3, z >> 21 & 3, z >> 19 & 3, z >> 17 & 3]
        else:
            mux, reg, neg = w >> 28 & 3, (z & 3) * 4 + (w >> 30), z >> 10 & 1
            sw = [z >> 8 & 3, z >> 6 & 3, z >> 4 & 3, z >> 2 & 3]
        if mux == 1:
            value = outputs[0] if reg == 12 else registers[reg]
            name = 'R' + str(reg)
        elif mux == 2:
            reg = y >> 9 & 15
            semantic = semantic_ids[reg] if reg < len(semantic_ids) else None
            value = [{f'v:{semantic}'} for _ in range(4)] if semantic is not None else [set() for _ in range(4)]
            name = 'v' + str(reg)
        elif mux == 3:
            reg = y >> 13 & 255
            assert not w & 2, 'Relative node-bank instruction requires separate proof'
            value = [{f'c:{reg}'} for _ in range(4)]
            name = 'c' + str(reg)
        else:
            value = [set() for _ in range(4)]
            name = '?'
        result.append(dict(text=('-' if neg else '') + name + '.' + ''.join('xyzw'[s] for s in sw),
                           components=[value[s] for s in sw]))
    return result


def has_incident(value):
    return 'v:8' in value


def has_node(value):
    return bool({'c:50', 'c:51', 'c:52'} & value)


def inspect(code, semantics):
    assert struct.unpack_from('<H', code)[0] == 0x2078
    assert len(code) == 4 + struct.unpack_from('<H', code, 2)[0] * 16
    registers = [[set() for _ in range(4)] for _ in range(13)]
    outputs = [[set() for _ in range(4)] for _ in range(16)]
    asm, incident_flow, first_node_mixes = [], [], []
    for at in range(4, len(code), 16):
        t = struct.unpack_from('<4I', code, at)
        mac, ilu = t[1] >> 21 & 15, t[1] >> 25 & 7
        assert mac < len(MAC)
        op = operands(t, semantics, registers, outputs)
        a, b, c = [o['components'] for o in op]
        m = [set() for _ in range(4)]
        if mac == 1:
            m = a
        elif mac in (2, 9, 10, 11, 12):
            m = [a[i] | b[i] for i in range(4)]
        elif mac == 3:
            m = [a[i] | c[i] for i in range(4)]
        elif mac == 4:
            m = [a[i] | b[i] | c[i] for i in range(4)]
        elif mac in (5, 6, 7):
            count = 4 if mac == 7 else 3
            deps = set().union(*a[:count], *b[:count])
            if mac == 6:
                deps |= b[3]
            m = [deps] * 4
        elif mac == 8:
            m = [set(), a[1] | b[1], a[2], b[3]]
        elif mac == 13:
            raise AssertionError('ARL is outside this bounded incident proof')
        l = c if ilu == 1 else ([c[0]] * 4 if 2 <= ilu <= 6 else [set().union(*c)] * 4)
        reg, mm, im, om = t[3] >> 20 & 15, t[3] >> 24 & 15, t[3] >> 16 & 15, t[3] >> 12 & 15
        destinations = []
        def destination(kind, r, mask):
            return kind + str(r) + '.' + ''.join('xyzw'[i] for i in range(4) if mask & (8 >> i))
        if mm:
            destinations.append('MAC->' + destination('R', reg, mm))
        if im:
            destinations.append('ILU->' + destination('R', 1 if mac else reg, im))
        if om:
            assert t[3] & 0x800
            destinations.append(('ILU' if t[3] & 4 else 'MAC') + '->' + destination('o', t[3] >> 3 & 15, om))
        text = f'{(at-4)//16:02d} ' + (MAC[mac] + ' ' + ','.join(op[n]['text'] for n in USED[mac]) if mac else '')
        text += ' | ' + (ILU[ilu] + ' ' + op[2]['text'] if ilu else '') + ' ; ' + ','.join(destinations)
        asm.append(text)
        used = list(USED[mac]) + ([2] if ilu else [])
        if any(has_incident(s) for n in used for s in op[n]['components']):
            incident_flow.append(dict(instruction=(at-4)//16, disassembly=text, words=[f'{w:08X}' for w in t]))
        if any(has_incident(s) and has_node(s) for s in m) and not any(
                has_incident(s) and has_node(s) for n in USED[mac] for s in op[n]['components']):
            # The first node/incident combination must project incident onto a
            # previously transformed geometric basis, rather than transform it.
            av, bv = set().union(*a[:3]), set().union(*b[:3])
            if has_incident(bv):
                av, bv = bv, av
            assert mac == 5 and has_incident(av) and not has_node(av), text
            assert has_node(bv) and not has_incident(bv), text
            assert {s for s in bv if s.startswith('v:')} <= {'v:4', 'v:5', 'v:6'}, text
            first_node_mixes.append(dict(instruction=(at-4)//16, disassembly=text,
                                        incident_dependencies=sorted(av), transformed_basis_dependencies=sorted(bv)))
        def write(target, value, mask):
            for i in range(4):
                if mask & (8 >> i):
                    target[i] = value[i]
        if mac and mm and not (ilu and reg == 1):
            write(outputs[0] if reg == 12 else registers[reg], m, mm)
        if ilu and im:
            ir = 1 if mac else reg
            write(outputs[0] if ir == 12 else registers[ir], l, im)
        if om:
            write(outputs[t[3] >> 3 & 15], l if t[3] & 4 else m, om)
    return '\n'.join(asm) + '\n', incident_flow, first_node_mixes


def main():
    source = ROOT / 'analysis/multiplayer-shading/model-incident-consumers.json'
    directory = ROOT / 'analysis/multiplayer-shading/model-incident-coordinate-proof'
    directory.mkdir(exist_ok=True)
    caches, records, unique = {}, [], set()
    for row in json.loads(source.read_text()):
        name = row['map']
        if name not in caches:
            maps = 'assets/map-pack/maps' if (ROOT / f'assets/map-pack/maps/{name}.map').is_file() else 'assets/disc/maps'
            caches[name] = Cache(ROOT / maps / f'{name}.map')
        cache = caches[name]
        _, root = cache.tag(row['datum'], 'vrtx')
        for ordinal, at in enumerate(cache.block(root + 4, 28)[:2]):
            semantics = [cache.u16(s) for s in cache.block(at + 4, 2)]
            assert 8 in semantics
            code = bytes(cache.data[cache.u32(at+16)-cache.magic:cache.u32(at+16)-cache.magic+cache.u32(at+12)])
            sha = hashlib.sha256(code).hexdigest()
            assembly, flow, mixes = inspect(code, semantics)
            if ordinal == 0:
                assert not mixes
            else:
                assert len(mixes) == 3
            stem = row['name'].split('\\')[-1] + '-class' + str(ordinal)
            (directory / (stem + '.vp')).write_bytes(code)
            (directory / (stem + '.txt')).write_text(assembly)
            unique.add(sha)
            records.append(dict(map=name, datum=f"{row['datum']:08X}", name=row['name'],
                                classification=ordinal, semantics=semantics, sha256=sha,
                                payload=stem+'.vp', disassembly=stem+'.txt',
                                incident_flow=flow, first_node_combinations=mixes))
    for c in caches.values():
        c.close()
    proof = dict(authority='Authored original vrtx class0 and class1 bytecode, original semantic list and simultaneous-read instruction dataflow.',
                 source_inventory_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
                 records=len(records), unique_payloads=len(unique), records_data=records,
                 conclusion='Every retained local-class node/incident first combination is DP3 of unchanged incident (possibly normalized) against already transformed normal/tangent/binormal. No incident vector is rotated by the node matrix. Class0 consumes already-world geometric bases and unchanged incident.',
                 boundary='This proves the coordinate contract of these retained compiled families; it does not establish live material/class selection, original stream producer admission or physical NV2A precision.')
    (directory / 'proof.json').write_text(json.dumps(proof, indent=2)+'\n')
    print(json.dumps({k: v for k,v in proof.items() if k != 'records_data'}, indent=2))


if __name__ == '__main__':
    main()
