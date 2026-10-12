#!/usr/bin/env python3
"""Compile complete authenticated vitality update, recovery and powerup owners."""
import argparse,json,struct
from pathlib import Path
from types import SimpleNamespace
from generate_physics_world_bootstrap_runtime_20261011 import generate,sha
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

BASE=ROOT/'analysis/damage-shields-goal-20261011/vitality-runtime'
REPORT_PIN='e0f135ff479f39a091589fa7393d3a72654b8d91ffe8aa334a4a874c6a17c3d1'
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--manifest',type=Path,required=True)
    a=ap.parse_args();report=BASE/'pairing-full/report.json';assert sha(report)==REPORT_PIN
    paths=[ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',NEW_DEFAULT]
    assert [sha(p) for p in paths]==[PIN,NEW_PIN]
    images=[Xbe(p) for p in paths];codes=[_Code(x,'.text') for x in images]
    addresses={};entries={};instructions={};bodies={};routines=[]
    def bind(p,q):
        p&=0xffffffff;q&=0xffffffff
        assert p not in addresses or addresses[p]==q,(hex(p),hex(q))
        addresses[p]=q
    for row in json.loads(report.read_text())['owners']:
        assert row['full_normalized_instruction_internal_branch_topology_equal']
        listings=[]
        for item in row['listings']:
            path=report.parent/item['file'];assert sha(path)==item['sha256']
            listings.append([(int(s.split()[0],16),bytes.fromhex(s.split()[1])) for s in path.read_text().splitlines()])
        assert len(listings[0])==len(listings[1])==row['old_rows']
        p,q=[int(row[k],16) for k in ('old_entry','u5_entry')];entries[f'{p:08x}']=q;bind(p,q)
        bodies[f'{p:08x}']=[p for p,raw in listings[0]];tables=[]
        for table in row['jump_tables']:
            assert table['element_bytes']==4
            p,q=[int(table[k],16) for k in ('old_va','u5_va')];bind(p,q)
            for n,(left,right) in enumerate(zip(table['old_targets'],table['u5_targets'])):
                bind(int(left,16),int(right,16));bind(p+4*n,q+4*n)
            for image,at,key in zip(images,(p,q),('old_raw_hex','u5_raw_hex')):
                assert image.read(at,table['count']*4).hex()==table[key]
            tables.append(dict(disc=p,later=q,count=table['count']))
        routines.append(dict(entry=int(row['old_entry'],16),tables=tables))
        for (p,raw),(q,later) in zip(*listings):
            assert images[0].read(p,len(raw))==raw and images[1].read(q,len(later))==later
            x,y=[code.at(at) for code,at in zip(codes,(p,q))]
            assert bytes(x.bytes)==raw and bytes(y.bytes)==later
            assert x.mnemonic==y.mnemonic and len(x.operands)==len(y.operands)
            instructions[f'{p:08x}']=q;bind(p,q)
            for u,v in zip(x.operands,y.operands):
                assert u.type==v.type and u.size==v.size
                if u.type==1:assert x.reg_name(u.reg)==y.reg_name(v.reg)
                if u.type==2 and u.imm!=v.imm:bind(u.imm,v.imm)
                if u.type==3:
                    assert (x.reg_name(u.mem.base),x.reg_name(u.mem.index),u.mem.scale,x.reg_name(u.mem.segment))==(
                        y.reg_name(v.mem.base),y.reg_name(v.mem.index),v.mem.scale,y.reg_name(v.mem.segment))
                    if u.mem.disp!=v.mem.disp:bind(u.mem.disp,v.mem.disp)
    paired=dict(complete_bodies=len(entries),entries=entries,disc_1_1_sha256=PIN,later_owned_xbe_sha256=NEW_PIN,
        address_map={f'{p:08x}':q for p,q in sorted(addresses.items())},instruction_map=instructions,
        body_sites=bodies,routines=routines,authority_sha256=sha(report),configuration_tool_sha256=sha(Path(__file__)))
    normalized=a.manifest.with_name('normalized-correspondence.json');normalized.parent.mkdir(parents=True,exist_ok=True)
    normalized.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    generate(SimpleNamespace(correspondence=normalized,output=a.output,manifest=a.manifest),paired,
        complete_listings=True,runtime_header='halo2/source_vitality_runtime.hpp',
        runtime_function='source_run_vitality_runtime',
        instruction_guard=lambda i:'s.require_x87();' if i.mnemonic.startswith('f') else '')
if __name__=='__main__':main()
