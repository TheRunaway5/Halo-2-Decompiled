#!/usr/bin/env python3
"""Compile complete authenticated damage delivery and its completed lower owners."""
import argparse,json,struct
from pathlib import Path
from types import SimpleNamespace
from generate_physics_world_bootstrap_runtime_20261011 import generate,sha
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code,PIN
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from native_damage_voice_request_tu15 import install_native_damage_voice_request

BASE=ROOT/'analysis/damage-shields-goal-20261011/delivery-runtime'
AUTHORITY=ROOT/'analysis/damage-shields-goal-20261011/dispatcher-boundary/pairing/report.json'
LOOKUP=ROOT/'analysis/damage-shields-goal-20261011/vitality-runtime/pairing-full/report.json'
AMOUNT=ROOT/'analysis/damage-shields-goal-20261011/amount-boundary/pairing/report.json'
CALLBACKS=ROOT/'analysis/damage-shields-goal-20261011/amount-boundary/callbacks/report.json'
REPORT_PIN='705e957d4705d048b6ca23611cdd0fd12c5185e3eccd6e3e8b6fe06f8f7a651d'
LOOKUP_PIN='e0f135ff479f39a091589fa7393d3a72654b8d91ffe8aa334a4a874c6a17c3d1'
AMOUNT_PIN='99c4266327fa78c815b58941f30a6d77b6b7fd097a57381f8476c7745f3e85f4'
CALLBACKS_PIN='60f8942bf5e8c0ce790c64e5a95fad83f916431c784b7746b53565c06044fdc7'
DELIVERY=ROOT/'analysis/damage-shields-goal-20261011/dispatcher-boundary/direct-callees/report.json'
DELIVERY_PIN='b67f02a45f4d3e0a3c153ce24c9c3c869c41e0a1d4068da497424de3b5d6ba8b'
LOWER=ROOT/'analysis/damage-shields-goal-20261011/vitality-delivery-boundary/pairing/report.json'
LOWER_PIN='9c4efec3a049701dd19571750a1a9bfac1b7fb0650eb4e065ec3c958e84148e1'
NEXT=ROOT/'analysis/damage-shields-goal-20261011/vitality-delivery-boundary/next-owners/report.json'
NEXT_PIN='627ff419faca76f4baaab777f2ae27cae74d6c821e6e609e6921d86251fb2fcc'
COMPARATOR=ROOT/'analysis/damage-shields-goal-20261011/vitality-delivery-boundary/comparator/report.json'
COMPARATOR_PIN='57fd72a586624ae453dbab3b450bebffc2984477eb8fedc6f22dcd5c8d686b69'
SECTIONS=ROOT/'analysis/damage-shields-goal-20261011/vitality-delivery-boundary/section-owners/report.json'
SECTIONS_PIN='21d757bc92a33a4504b0dc5d76dd08764c3144e617a703ceb6e949b877a5719e'
LOCATION=ROOT/'analysis/damage-shields-goal-20261011/vitality-delivery-boundary/location-owners/report.json'
LOCATION_PIN='5a76f825244652aff7abd0c67bfca96a7ff07f392772df90ab6e780274a665a2'
FEEDBACK=ROOT/'analysis/damage-shields-goal-20261011/delivery-next-boundary/pairing/report.json'
FEEDBACK_PIN='b8f917d6d0e50d666e8e6a983413e9ac71986f4bba5981e752f1c7a96280f845'
FEEDBACK_REPORTS={
    'feedback-lower':'12afc6b0b8b56079b346506bd93d189150bdaeff1af0a5cc311e6b218e0933b9',
    'response':'9bba3234cf27b50b79bf49cf4e23b65ef9582e66cba8f397018b76637ae4ca67',
    'feedback-services':'45253c30a6cf9e764f50006ccea75be3a1e3277c19ddf7363e75c660ee415cd4',
    'feedback-next':'9c2b6c867e537ad1a385f58550f98c561e0667acc61119fd4c9247ec9082d263',
    'conversion-v2':'6d8be574aa0c28bdbfc0bea4b76979a0bc6159693b040ba6716dafe95c2aa2b6',
}
FEEDBACK_NEXT_SERVICES=ROOT/'analysis/damage-shields-goal-20261011/feedback-next-services/paired/report.json'
FEEDBACK_NEXT_SERVICES_PIN='1e87fea980174b232db5816523585cf37d80a626ab707dd4dd5fdea6fcd8e922'
ORDERED_SSE=r'''
// Preserve original operand order, including distinct quiet NaN payloads.
// C++ commutative arithmetic can otherwise let the optimizer swap operands.
__m128 damage_addss(__m128 left,__m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("addss %1,%0":"+x"(left):"x"(right));return left;
#else
    throw std::runtime_error("Required original damage SSE operand-order backend unavailable");
#endif
}
__m128 damage_mulss(__m128 left,__m128 right) {
#if (defined(__i386__) || defined(__x86_64__)) && !defined(_MSC_VER)
    asm volatile("mulss %1,%0":"+x"(left):"x"(right));return left;
#else
    throw std::runtime_error("Required original damage SSE operand-order backend unavailable");
#endif
}
'''
def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,required=True)
    ap.add_argument('--manifest',type=Path,required=True)
    a=ap.parse_args();assert sha(AUTHORITY)==REPORT_PIN and sha(LOOKUP)==LOOKUP_PIN
    assert sha(AMOUNT)==AMOUNT_PIN
    assert sha(CALLBACKS)==CALLBACKS_PIN and sha(DELIVERY)==DELIVERY_PIN and sha(LOWER)==LOWER_PIN
    assert sha(NEXT)==NEXT_PIN and sha(COMPARATOR)==COMPARATOR_PIN and sha(SECTIONS)==SECTIONS_PIN and sha(LOCATION)==LOCATION_PIN and sha(FEEDBACK)==FEEDBACK_PIN
    paths=[ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',NEW_DEFAULT]
    assert [sha(p) for p in paths]==[PIN,NEW_PIN]
    images=[Xbe(p) for p in paths];codes=[_Code(x,'.text') for x in images]
    addresses={};entries={};instructions={};bodies={};routines=[]
    def bind(p,q):
        p&=0xffffffff;q&=0xffffffff
        assert p not in addresses or addresses[p]==q,(hex(p),hex(q))
        addresses[p]=q
    owners=[(AUTHORITY,row) for row in json.loads(AUTHORITY.read_text())['owners']]
    owners.extend((LOOKUP,row) for row in json.loads(LOOKUP.read_text())['owners'])
    owners.extend((AMOUNT,row) for row in json.loads(AMOUNT.read_text())['owners'])
    owners.extend((CALLBACKS,row) for row in json.loads(CALLBACKS.read_text())['owners'])
    owners.extend((report,row) for report in (DELIVERY,LOWER) for row in json.loads(report.read_text())['owners'])
    for row in json.loads(NEXT.read_text())['owners']:
        if row.get('full_normalized_instruction_internal_branch_topology_equal'):owners.append((NEXT,row))
        else:assert row['old_entry']=='00124440' and 'u5_entry' not in row
    comparator=json.loads(COMPARATOR.read_text());assert comparator['complete_normalized_body_equal']
    owners.append((COMPARATOR,dict(comparator,old_rows=5,jump_tables=[],full_normalized_instruction_internal_branch_topology_equal=True)))
    owners.extend((SECTIONS,row) for row in json.loads(SECTIONS.read_text())['owners'])
    owners.extend((LOCATION,row) for row in json.loads(LOCATION.read_text())['owners'])
    for row in json.loads(FEEDBACK.read_text())['owners']:
        if row.get('full_normalized_instruction_internal_branch_topology_equal'):
            owners.append((FEEDBACK,row))
        else:
            assert row['old_entry']=='00329168' and row['full_body_matches']==['0032ccd8','0037d0b4']
    for name,pin in FEEDBACK_REPORTS.items():
        report=FEEDBACK.parent.parent/name/'report.json';assert sha(report)==pin
        owners.extend((report,row) for row in json.loads(report.read_text())['owners'])
    assert sha(FEEDBACK_NEXT_SERVICES)==FEEDBACK_NEXT_SERVICES_PIN
    owners.extend((FEEDBACK_NEXT_SERVICES,row) for row in json.loads(FEEDBACK_NEXT_SERVICES.read_text())['owners'])
    withheld={0x329168,0x329185,0x32e59c,0x32e529,0x32e540,0x32e5b2,0x32e5bf,0x32e4d7}
    seen={}
    for report,row in owners:
        assert row['full_normalized_instruction_internal_branch_topology_equal']
        listings=[]
        for item in row['listings']:
            path=report.parent/item['file'];assert sha(path)==item['sha256']
            listings.append([(int(s.split()[0],16),bytes.fromhex(s.split()[1])) for s in path.read_text().splitlines()])
        assert len(listings[0])==len(listings[1])==row['old_rows']
        if row['old_entry'] in seen:
            assert seen[row['old_entry']]==listings
            continue
        if int(row['old_entry'],16) in withheld:continue
        seen[row['old_entry']]=listings
        p,q=[int(row[k],16) for k in ('old_entry','u5_entry')];entries[f'{p:08x}']=q;bind(p,q)
        bodies[f'{p:08x}']=[p for p,raw in listings[0]];tables=[]
        for table in row['jump_tables']:
            width=table['element_bytes'];assert width in (1,4)
            p,q=[int(table[k],16) for k in ('old_va','u5_va')];bind(p,q)
            for n,(left,right) in enumerate(zip(table['old_targets'],table['u5_targets'])):
                if width==4:bind(int(left,16),int(right,16))
                else:assert left==right
                bind(p+width*n,q+width*n)
            for image,at,key in zip(images,(p,q),('old_raw_hex','u5_raw_hex')):
                assert image.read(at,table['count']*width).hex()==table[key]
            if width==4:tables.append(dict(disc=p,later=q,count=table['count']))
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
        body_sites=bodies,routines=routines,authority_sha256=sha(AUTHORITY),lookup_authority_sha256=sha(LOOKUP),amount_authority_sha256=sha(AMOUNT),callback_authority_sha256=sha(CALLBACKS),delivery_authority_sha256=sha(DELIVERY),lower_authority_sha256=sha(LOWER),next_authority_sha256=sha(NEXT),comparator_authority_sha256=sha(COMPARATOR),section_authority_sha256=sha(SECTIONS),location_authority_sha256=sha(LOCATION),feedback_authority_sha256=sha(FEEDBACK),feedback_lower_authorities=FEEDBACK_REPORTS,feedback_next_services_authority_sha256=sha(FEEDBACK_NEXT_SERVICES),withheld_entries=[f'{p:08x}' for p in sorted(withheld)],withheld_reason='Composed original Unicorn replay and literal native x87 disagree on precision-status-driven CRT branch. Retained feedback-runtime/original-v2, composition-v1 and generated-94; native ownership remains explicit at acos 329168.',configuration_tool_sha256=sha(Path(__file__)))
    normalized=a.manifest.with_name('normalized-correspondence.json');normalized.parent.mkdir(parents=True,exist_ok=True)
    normalized.write_text(json.dumps(paired,indent=2,sort_keys=True)+'\n')
    def instruction_factory(*,get,put,addr,xf):
        def extended_instruction(i):
            op=list(i.operands);mn=i.mnemonic
            if i.address in (0xd7939,0xd80c6,0xda969):
                assert mn=='call' and op[0].imm==0x1781f0
                # TU1.5 publication observes the real shield effect request,
                # then executes the unchanged whole lower owner. Keep the
                # paired TU1.1 path and all original instruction semantics.
                actual={0xd7939:0xd8889,0xd80c6:0xd9016,0xda969:0xdb8b9}[i.address]
                assert codes[1].at(actual).operands[0].imm==0x179610
                return (f's.push(0x{i.address:x}u,s.image==SourcePhysicsContactImage::TitleUpdate15?0x{actual+5:x}u:0x{i.address+i.size:x}u);'
                    f'if(s.image==SourcePhysicsContactImage::TitleUpdate15 && s.services.required_owner)s.required(0x{actual:x}u,selected(0x1781f0u,s.image),false);'
                    'else run_1781f0(s);')
            if mn=='setge':return put(i,op[0],'s.flags.sf==s.flags.of')
            if i.mnemonic=='mul':
                operand,=i.operands
                assert operand.type==1 and operand.size==4
                from generate_physics_world_bootstrap_runtime_20261011 import REGS
                index,width,shift=REGS[i.reg_name(operand.reg)];assert width==4 and shift==0
                return ('{const auto x=std::uint64_t(s.gp[0])*s.reg('+str(index)+',4,0);'
                    's.gp[0]=std::uint32_t(x);s.gp[2]=std::uint32_t(x>>32u);s.flags.cf=s.flags.of=s.gp[2]!=0;}')
            if i.mnemonic=='cvttss2si':
                left,right=i.operands
                assert left.type==1 and left.size==4 and right.type==1 and i.reg_name(right.reg).startswith('xmm')
                from generate_physics_world_bootstrap_runtime_20261011 import REGS
                index,width,shift=REGS[i.reg_name(left.reg)];assert width==4 and shift==0
                source=int(i.reg_name(right.reg)[3:])
                return f's.set({index},std::uint32_t(_mm_cvttss_si32(s.xmm[{source}])),4,0);'
        return extended_instruction
    generate(SimpleNamespace(correspondence=normalized,output=a.output,manifest=a.manifest),paired,
        complete_listings=True,runtime_header='halo2/source_damage_delivery_runtime.hpp',
        runtime_function='source_run_damage_delivery_runtime',
        extra_instruction_factory=instruction_factory,
        extra_preamble=ORDERED_SSE,
        instruction_guard=lambda i:'s.require_x87();' if i.mnemonic.startswith('f') else '')
    a.output.write_text(a.output.read_text().replace('_mm_add_ss(', 'damage_addss(').replace('_mm_mul_ss(', 'damage_mulss('))
    text=a.output.read_text()
    ending='} // namespace halo2\n'
    assert text.endswith(ending)
    owns='bool source_damage_delivery_owns_entry(std::uint32_t p) {switch(p){'+''.join(f'case 0x{int(p,16):x}u:' for p in sorted(entries))+'return true;default:return false;}}\n'
    a.output.write_text(install_native_damage_voice_request(text[:-len(ending)]+owns+ending))
    manifest=json.loads(a.manifest.read_text());manifest['generated_cpp_sha256']=sha(a.output)
    manifest['configuration_tool_sha256']=sha(Path(__file__))
    manifest['scalar_add_multiply_operand_order']='literal ADDSS/MULSS, including quiet NaN payload selection'
    a.manifest.write_text(json.dumps(manifest,indent=2,sort_keys=True)+'\n')
    print(json.dumps(dict(final_generated_cpp_sha256=sha(a.output),final_manifest_sha256=sha(a.manifest),complete_bodies=len(entries))))
if __name__=='__main__':main()
