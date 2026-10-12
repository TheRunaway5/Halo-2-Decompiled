#!/usr/bin/env python3
"""Audit bounded static correspondence questions, not whole-game parity."""
import argparse,json
from pathlib import Path
from capstone.x86 import X86_OP_IMM
from research_mp_combat_update_inventory import ROOT,OLD,NEW_DEFAULT,NEW_PIN,PIN,sha
from oracle_smg_firing_1_1 import Xbe,_Code
BASE=ROOT/'analysis/multiplayer/title-update-1.5/combat-function-inventory'
def main():
    ap=argparse.ArgumentParser();ap.add_argument('--suite',type=Path,default=BASE/'frozen/suite.json');a=ap.parse_args();s=json.loads(a.suite.read_text())
    for p,h in s['files'].items():assert sha(ROOT/p)==h,p
    prior=0
    for p,h in s['prior_manifests'].items():
        assert sha(ROOT/p)==h
        for q,v in json.loads((ROOT/p).read_text())['files'].items():assert sha(ROOT/q)==v,q;prior+=1
    r=json.loads((BASE/'report.json').read_text());b=json.loads((BASE/'reexecution/report.json').read_text());assert r==b
    assert r['tool_sha256']==sha(ROOT/'tools/research_mp_combat_update_inventory.py')and r['disc_xbe_sha256']==PIN and r['owned_later_xbe_sha256']==NEW_PIN
    assert not r['native_gameplay_update_applied']and r['selected_functions']==765
    assert sum('later_entry'in q for q in r['routines'])==720 and len(r['unmapped_neighbor_intervals'])==45
    assert all(q['normalized_instructions_and_internal_branch_equal']for q in r['routines']if'later_entry'in q)
    old,new=Xbe(OLD),Xbe(NEW_DEFAULT);assert sha(OLD)==PIN and sha(NEW_DEFAULT)==NEW_PIN;codes=[_Code(x,'.text')for x in(old,new)]
    listing_rows=0;ref_count=0;immediate_count=0;calls=0
    for q in r['routines']:
        for v in q.get('different_paired_image_memory_bytes',[]):
            assert old.read(int(v['disc_address'],16),v['size']).hex()==v['disc_hex']
            assert new.read(int(v['later_address'],16),v['size']).hex()==v['later_hex']and v['disc_hex']!=v['later_hex'];ref_count+=1
        for v in q.get('different_nonbranch_immediates',[]):
            for label,code in zip(('disc','later'),codes):
                ins=code.at(int(v[label+'_site'],16));assert ins.mnemonic+' '+ins.op_str==v[label+'_instruction']
                assert any(o.type==X86_OP_IMM and(o.imm&0xffffffff)==int(v[label+'_immediate'],16)for o in ins.operands)
            immediate_count+=1
        for v in q.get('unresolved_or_different_paired_callees',[]):
            for label,code in zip(('disc','later'),codes):
                ins=code.at(int(v[label+'_site'],16));assert ins.mnemonic=='call'and ins.operands[0].type==X86_OP_IMM and ins.operands[0].imm==int(v[label+'_target'],16)
            calls+=1
    for run in(BASE,BASE/'reexecution'):
        for p in(run/'instructions').glob('*.asm'):
            code,image=(codes[0],old)if'-disc-'in p.name else(codes[1],new)
            for line in p.read_text().splitlines():
                at,h,assembly=line.split(' ',2);ins=code.at(int(at,16));raw=bytes.fromhex(h)
                assert ins.bytes==raw==image.read(int(at,16),len(raw))and ins.mnemonic+' '+ins.op_str==assembly;listing_rows+=1
            counterpart=(BASE/'reexecution/instructions'/p.name)if run==BASE else(BASE/'instructions'/p.name)
            assert p.read_bytes()==counterpart.read_bytes()
    result={'suite_sha256':sha(a.suite),'files':len(s['files']),'prior_frozen_members_unchanged':prior,'bounded_selected_functions':765,'unique_prefix_candidates':720,'unmatched_or_ambiguous':45,
            'different_absolute_memory_operands_retained':ref_count,'different_nonbranch_immediates_retained':immediate_count,'unresolved_or_different_callees_retained':calls,
            'original_listing_rows_verified':listing_rows,'whole_inventory_and_listings_independently_identical':True,'whole_gameplay_equivalence_claimed':False,'native_gameplay_update_applied':False}
    (a.suite.parent/'audit.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps(result))
if __name__=='__main__':main()
