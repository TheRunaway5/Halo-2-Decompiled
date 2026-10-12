#!/usr/bin/env python3
"""Compile complete paired falling/airborne producers; preserve required owners."""
import argparse,json
from pathlib import Path
from types import SimpleNamespace
from generate_physics_world_bootstrap_runtime_20261011 import generate,sha
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN

BASE=ROOT/'analysis/damage-shields-goal-20261011/falling-producer'
TRIG=r'''
void falling_trigonometric(WorldState &s,bool cosine) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    const auto input=s.fp.st(0);long double result;std::uint16_t status;
    if(cosine)asm volatile("fldt %2\n\tfcos\n\tfnstsw %1\n\tfstpt %0"
        :"=m"(result),"=m"(status):"m"(input):"st");
    else asm volatile("fldt %2\n\tfsin\n\tfnstsw %1\n\tfstpt %0"
        :"=m"(result),"=m"(status):"m"(input):"st");
    s.fp.st(0)=result;s.fp.status=(s.fp.status&~0x600u)|(status&0x600u);
#else
    throw std::runtime_error("Required original falling trigonometric backend unavailable");
#endif
}
'''
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True);ap.add_argument('--manifest',type=Path,required=True)
    a=ap.parse_args();authority=BASE/'manifest.json'
    assert sha(authority)=='ff7e9a352ac5c4ee3e6fcd58f99e2288dcc5cb450209e8c124e1d9159dee7c83'
    source=json.loads(authority.read_text())
    paths=[ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',NEW_DEFAULT]
    assert sha(paths[0])==PIN and sha(paths[1])==NEW_PIN
    images=[Xbe(p) for p in paths];codes=[_Code(x,'.text') for x in images]
    addresses={};entries={};instructions={};bodies={};routines=[]
    def bind(p,q):
        p&=0xffffffff;q&=0xffffffff
        assert p not in addresses or addresses[p]==q,(hex(p),hex(q))
        addresses[p]=q
    support=BASE/'support-motion/pairing/report.json'
    assert sha(support)=='99f7141c5bcd970ee422498db11a03cdd39b06f316dc775222ef9225f94154ed'
    point=BASE/'support-motion/point-owner/report.json'
    assert sha(point)=='3a8ce3017822e6a3a377f9ae98a78d4ae6df5afcfd712c2f34705abf3bcc5293'
    requests=BASE/'damage-request-owners/pairing/report.json'
    assert sha(requests)=='a1631b38958ff5fb816aa74bb1dcc4e1fef56feb6ef7c23d8c0be6bc87e8a251'
    rows=[*source['body_correspondence']]
    for report,wanted in ((support,'0010df90'),(point,'000bacc0'),(requests,'000d7c60'),(requests,'000d7cd0')):
        row=next(r for r in json.loads(report.read_text())['owners'] if r['old_entry']==wanted)
        assert row['full_normalized_instruction_internal_branch_topology_equal']
        rows.append(dict(tu11=row['old_entry'],u5=row['u5_entry'],rows=row['old_rows'],
            complete_normalized_body_internal_branch_shape_equal=True,
            **{field:dict(path=str((report.parent/item['file']).relative_to(ROOT)),sha256=item['sha256'])
               for field,item in zip(('reused_tu11_listing','reused_u5_listing'),row['listings'])}))
    for row in rows:
        assert row['complete_normalized_body_internal_branch_shape_equal']
        listings=[]
        for field in ('reused_tu11_listing','reused_u5_listing'):
            item=row[field];path=ROOT/item['path'];assert sha(path)==item['sha256']
            listings.append([(int(s.split()[0],16),bytes.fromhex(s.split()[1])) for s in path.read_text().splitlines()])
        assert len(listings[0])==len(listings[1])==row['rows']
        p,q=[int(row[k],16) for k in ('tu11','u5')];entries[f'{p:08x}']=q;bind(p,q)
        bodies[f'{p:08x}']=[p for p,raw in listings[0]];routines.append(dict(entry=p,tables=[]))
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
        body_sites=bodies,routines=routines,authority_sha256=sha(authority),support_authority_sha256=sha(support),
        point_authority_sha256=sha(point),
        request_authority_sha256=sha(requests),
        configuration_tool_sha256=sha(Path(__file__)))
    normalized=a.manifest.with_name('normalized-correspondence.json');normalized.parent.mkdir(parents=True,exist_ok=True)
    normalized.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    def extension(i):
        if i.mnemonic in ('fsin','fcos'):return 'falling_trigonometric(s,'+str(i.mnemonic=='fcos').lower()+');'
    def guard(i):
        return 's.require_x87();' if i.mnemonic.startswith('f') else ''
    generate(SimpleNamespace(correspondence=normalized,output=a.output,manifest=a.manifest),paired,
        complete_listings=True,runtime_header='halo2/biped_falling_damage.hpp',
        runtime_function='source_run_biped_falling_damage_runtime',extra_instruction=extension,
        extra_preamble=TRIG,instruction_guard=guard)
    manifest=json.loads(a.manifest.read_text());manifest['configuration_tool_sha256']=sha(Path(__file__))
    manifest['scope']='Complete paired producers; actual required lower frames; no supplied velocity/support/damage/lifecycle result or live gameplay proof'
    a.manifest.write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
if __name__=='__main__':main()
