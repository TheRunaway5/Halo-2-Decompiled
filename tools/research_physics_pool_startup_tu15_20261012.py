#!/usr/bin/env python3
"""Recover fresh physics pool producers from actual TU1.5 stores and calls.

Complete bodies, internal branches, relocated operands, resident allocator
object/table, names and containing caller are retained. Prefixes locate only.
No heap capture, constructed pool or allocator result is supplied.
"""
import argparse
import collections
import csv
import hashlib
import json
import struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code, _trace, PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN, norm, full_tokens, containing

def sha(p): return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    a = ap.parse_args()
    old_path = ROOT / 'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03'
    assert sha(old_path) == PIN and sha(NEW_DEFAULT) == NEW_PIN
    old, new = Xbe(old_path), Xbe(NEW_DEFAULT)
    oc, nc = _Code(old, '.text'), _Code(new, '.text')
    os = {int(r['address'], 16) for r in csv.DictReader(
        (ROOT / 'analysis/map-pack-update/functions.tsv').open(), delimiter='\t')}
    old_obj = struct.unpack('<I',old.read(0x46fbf8,4))[0]
    old_vt = struct.unpack('<I',old.read(old_obj,4))[0]
    old_methods = struct.unpack('<2I',old.read(old_vt,8))
    assert (old_obj,old_vt,old_methods) == (0x480d68,0x45a480,(0x11e150,0x11e180))
    os.update(old_methods)
    md = Cs(CS_ARCH_X86, CS_MODE_32); md.detail = True; md.skipdata = True
    sec = new.section('.text')
    linear = list(md.disasm(new.section_bytes(sec), sec.va))
    ns = {i.operands[0].imm for i in linear if i.id and i.mnemonic == 'call'
          and i.operands[0].type == 2 and nc.inside(i.operands[0].imm)}
    # These actual producers were located by stores to the actual selected
    # image globals. The complete parent call proves both constructor entries.
    seeds = {0x22cdc0, 0x1d6ee0, 0x22ce40, 0x16e450, 0x16e4b0,
             0x32a4e0, 0x11f3f0, 0x33b3ac, 0x11f420, 0x33b3da}
    ns |= seeds | {0x1cb130}
    old_prefixes = collections.defaultdict(list)
    window = collections.deque(maxlen=6)
    old_sec = old.section('.text')
    for i in md.disasm(old.section_bytes(old_sec),old_sec.va):
        window.append((i.address,norm(i,old) if i.id else ''))
        if len(window)==6:
            old_prefixes[tuple(token for _,token in window)].append(window[0][0])
    mapping = {}
    def pair(q):
        nf = _trace(nc, q, ns)
        ni = [nc.at(p) for p in sorted(nf.code)]
        matches = []
        for p in old_prefixes[tuple(norm(i, new) for i in ni[:6])]:
            of = _trace(oc, p, os)
            oi = [oc.at(t) for t in sorted(of.code)]
            if full_tokens(oi, old) == full_tokens(ni, new): matches.append((p, of, oi))
        assert len(matches) == 1, (hex(q), [hex(r[0]) for r in matches])
        p, of, oi = matches[0]
        assert not of.tables and not nf.tables
        mapping[p] = q
        return p, of, nf, oi, ni
    rows = [pair(q) for q in sorted(seeds)]
    # Retain the complete original caller even though the allocator/arena
    # owners preceding/following the constructors are not executed here.
    caller = pair(0x1cb130)
    # Pair the existing raw pool validity/reset/allocation APIs independently.
    tokens = [norm(i, new) if i.id else '' for i in linear]
    for p in (0x16d210, 0x16d220, 0x16d2c0, 0x16d4c0, 0x16d560):
        of = _trace(oc, p, os)
        oi = [oc.at(t) for t in sorted(of.code)]
        prefix = [norm(i, old) for i in oi[:6]]
        candidates = []
        for n in range(len(tokens)-len(prefix)):
            if tokens[n:n+len(prefix)] != prefix: continue
            q = linear[n].address
            nf = _trace(nc, q, ns | {q})
            ni = [nc.at(t) for t in sorted(nf.code)]
            if full_tokens(oi, old) == full_tokens(ni, new): candidates.append((q, nf, ni))
        assert len(candidates) == 1, (hex(p), len(candidates))
        q, nf, ni = candidates[0]; ns.add(q); mapping[p] = q
        rows.append((p, of, nf, oi, ni))
    a.output.mkdir(parents=True, exist_ok=False)
    out = a.output / 'instructions'; out.mkdir()
    report = dict(disc_1_1_sha256=PIN, later_owned_xbe_sha256=NEW_PIN,
        tool_sha256=sha(__file__), entries={}, instruction_map={}, address_map={},
        routines=[], outside_direct_calls={}, indirect_calls=[], scope=__doc__)
    def bind(key, p, q):
        k = f'{p & 0xffffffff:08x}'; q &= 0xffffffff
        assert k not in report[key] or report[key][k] == q, (key,k,q)
        report[key][k] = q
    for p, of, nf, oi, ni in rows + [caller]:
        q = mapping[p]
        row = dict(disc_1_1_entry=f'{p:08x}', later_entry=f'{q:08x}',
                   instructions=len(oi), tables=[], complete_paired_body=True,
                   retained_caller_only=q == 0x1cb130)
        if q != 0x1cb130: bind('entries', p, q)
        for x, y in zip(oi, ni):
            bind('instruction_map', x.address, y.address)
            for u, v in zip(x.operands, y.operands):
                assert u.type == v.type and u.size == v.size
                if u.type == 2: left, right = u.imm, v.imm
                elif u.type == 3: left, right = u.mem.disp, v.mem.disp
                else: continue
                if containing(old,left) and containing(new,right): bind('address_map',left,right)
            if x.mnemonic == 'call':
                if x.operands[0].type == 2:
                    target, later = x.operands[0].imm, y.operands[0].imm
                    if target in mapping: assert mapping[target] == later
                    else: report['outside_direct_calls'].setdefault(f'{target:08x}',[]).append(
                        dict(site=x.address,later_site=y.address,later_target=later))
                else: report['indirect_calls'].append(dict(site=x.address,later_site=y.address,
                    owner=p,later_owner=q,operand=y.op_str))
        for label, ins in (('disc',oi),('later',ni)):
            path = out / f'{p:08x}-{label}.asm'
            path.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
            row[label+'_listing'] = dict(file=path.name,sha256=sha(path))
        report['routines'].append(row)
    # Literal pointer chain and complete constructor names are independent
    # selected-image evidence, not an assumed relocation delta.
    obj = struct.unpack('<I',new.read(0x474cb8,4))[0]
    vt = struct.unpack('<I',new.read(obj,4))[0]
    methods = struct.unpack('<2I',new.read(vt,8))
    assert (obj,vt,methods) == (0x486034,0x45f048,(0x11f3f0,0x11f420))
    report['resident_allocator'] = dict(global_address=0x474cb8,object=obj,vtable=vt,
        pointer_hex=new.read(0x474cb8,4).hex(),object_hex=new.read(obj,4).hex(),
        table_hex=new.read(vt,8).hex(),methods=methods)
    report['names'] = {f'{p:08x}':new.read(p,32).hex()
                       for p in (0x4638a4,0x463894,0x461adc)}
    report['kernel_imports'] = {f'{p:08x}':new.read(p,4).hex() for p in (0x41b6d4,0x41b6d0)}
    report['complete_bodies'] = len(report['entries'])
    path = a.output / 'complete-report.json'
    path.write_text(json.dumps(report,sort_keys=True,indent=2)+'\n')
    print(json.dumps(dict(sha256=sha(path),complete_bodies=report['complete_bodies'],
        instructions=sum(r['instructions'] for r in report['routines']),entries=report['entries'])))
if __name__ == '__main__': main()
