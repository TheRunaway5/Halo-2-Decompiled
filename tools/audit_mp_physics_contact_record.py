#!/usr/bin/env python3
"""Audit immutable original constructor fixtures and complete image bodies."""
import argparse, hashlib, json, struct
from pathlib import Path
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_IMM
from export_mp_physics_contact_record_1_1_to_1_5 import (
    ROOT, OUT, SOURCE, SOURCE_PIN, OLD_ENTRY, NEW_DEFAULT, NEW_PIN, PIN,
    Xbe, sha, verify, source_inputs)
from oracle_mp_item_current_producers_1_1 import encoded, frozen_write

def audit():
    count=verify();a=OUT/'measured';b=OUT/'repeat'
    left={str(p.relative_to(a)):sha(p)for p in a.rglob('*')if p.is_file()}
    right={str(p.relative_to(b)):sha(p)for p in b.rglob('*')if p.is_file()}
    assert left==right and len(left)==4
    r=json.loads((a/'report.json').read_text())
    assert r['exporter_sha256']==sha(ROOT/'tools/export_mp_physics_contact_record_1_1_to_1_5.py')
    assert r['source_manifest_sha256']==SOURCE_PIN and not r['native_expected_values_used']
    assert r['old_image_sha256']==PIN and r['later_owned_image_sha256']==NEW_PIN
    assert sha(NEW_DEFAULT)==NEW_PIN
    old=Xbe(ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03');new=Xbe(NEW_DEFAULT)
    mapping=r['correspondence'];assert mapping['old_entry']=='001d2df0'and mapping['later_entry']=='001d76b0'
    assert mapping['whole_normalized_body_and_internal_branch_topology_equal']and mapping['every_instruction_offset_equal']
    assert len(mapping['paired_sites'])==98
    images={'map_pack_1_1':old,'title_update_1_5':new}
    listings={}
    for item in mapping['listings']:
        p=a/item['file'];assert sha(p)==item['sha256']
        rows={}
        for line in p.read_text().splitlines():
            address,raw,*_=line.split();address=int(address,16);raw=bytes.fromhex(raw)
            assert images[item['image']].read(address,len(raw))==raw
            rows[address]=raw
        assert len(rows)==item['rows']==98;listings[item['image']]=rows
    paired=mapping['paired_memory_operands'];assert len(paired)==1
    ref=paired[0];assert ref['old_address']=='0052625c'and ref['later_address']=='0052be70'
    assert ref['old_hex']==old.read(0x52625c,4).hex()and ref['later_hex']==new.read(0x52be70,4).hex()
    call,_,expected=source_inputs();assert r['original_captured_call']==call
    known=0
    for label,c in zip(('map_pack_1_1','title_update_1_5'),r['cases']):
        assert c['return_esp_delta']==40 and c['returned_eax']==call['returned_eax']and not c['supplied_services']
        assert c['nine_stack_words']==call['stack'][:9]and c['initial_registers']==call['registers']
        assert int(c['nine_stack_words'][0],16)>>29==1 # Actual branch avoids unmeasured lookup.
        assert {int(p,16)for p in c['executed_addresses']}<=listings[label].keys()
        assert len(c['input_buffers'])==len(c['output_buffers'])==5
        memory={int(p['address'],16)+n:v for p in c['input_buffers']for n,v in enumerate(bytes.fromhex(p['raw_hex']))}
        assert len(c['ordered_stores'])==14
        for store in c['ordered_stores']:
            assert int(store['instruction'],16)in listings[label]
            at=int(store['address'],16);before=bytes.fromhex(store['before_hex']);written=bytes.fromhex(store['written_hex'])
            assert bytes(memory[at+n]for n in range(len(before)))==before
            assert len(before)==len(written)==store['size']
            for n,v in enumerate(written):memory[at+n]=v
        for p in c['output_buffers']:
            raw=bytes.fromhex(p['raw_hex']);at=int(p['address'],16)
            assert bytes(memory[at+n]for n in range(len(raw)))==raw;known+=len(raw)
        assert c['output_buffers'][0]['raw_hex']==call['contact48_after_hex']
    assert r['fixture_sha256']==sha(a/'fixtures.tsv')
    # Locate genuine direct CALLs in the new original text, independent of any
    # annotation function name. They prove the matched entry is a call target.
    md=Cs(CS_ARCH_X86,CS_MODE_32);md.detail=True;md.skipdata=True
    section=new.section('.text');callers=[]
    for ins in md.disasm(new.section_bytes(section),section.va):
        if ins.mnemonic=='call'and len(ins.operands)==1 and ins.operands[0].type==X86_OP_IMM and ins.operands[0].imm==0x1d76b0:
            callers.append(dict(site=f'{ins.address:08x}',instruction_hex=ins.bytes.hex()))
    assert callers
    return dict(schema=1,auditor_sha256=sha(__file__),source_freeze_verified_files=count,
        independent_reports_and_all_exports_byte_identical=True,compared_files=left,
        complete_original_instruction_rows=196,actual_original_constructor_calls=2,
        full_row_body_property_output_bytes=known,ordered_original_stores=28,
        original_later_direct_callers=callers,no_native_expected_values=True,
        no_live_solver_or_damage_admission_added=True)

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--output',type=Path,default=OUT/'audit.json');args=ap.parse_args()
    r=audit();frozen_write(args.output,encoded(r));print(json.dumps(r))

if __name__=='__main__':main()
