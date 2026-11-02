#!/usr/bin/env python3
"""Full native3232B0 lifetime on genuine typed BEFORE with an absent base lock.

Every captured bank and ordered operand/store/FP is compared. No source saved
register image or after-DATA is supplied. Registered registry entries remain
unsupported and are rejected before this full-return proof is attempted.
"""
import argparse,bisect,hashlib,inspect,json,os,subprocess,sys,tarfile
from pathlib import Path
import validate_native_agent_cleanup_children_tu15 as child
from oracle_smg_firing_1_1 import ROOT,Xbe,_Code
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from generate_physics_live_support_tu15 import complete_body

def sha(path):return hashlib.file_digest(Path(path).open('rb'),'sha256').hexdigest()

def main():
    p=argparse.ArgumentParser(add_help=False);p.add_argument('--archive',type=Path,required=True)
    p.add_argument('--archive-sha256',required=True);a,rest=p.parse_known_args()
    sys.argv=[sys.argv[0],*rest];assert os.environ.get('HALO2_TEST_SCRATCH')
    assert sha(a.archive)==a.archive_sha256 and sha(NEW_DEFAULT)==NEW_PIN
    capture=json.loads((a.archive.parent/'capture-native-before.json').read_text())
    assert capture['schema']=='native-tu15-contact-manager-typed-before-v1'
    typed=capture['input'];assert typed['owner']==0x3232b0 and typed['preserved_x87_depth']==0
    assert not capture['physics_result_supplied'] and not capture.get('guest_abi_fabricated',False)
    wanted={r['before_file']:r for r in capture['regions']};banks={}
    stream=subprocess.Popen(['zstd','-dc',str(a.archive)],stdout=subprocess.PIPE)
    with tarfile.open(fileobj=stream.stdout,mode='r|')as tar:
        for member in tar:
            name=Path(member.name).name
            if name in wanted:
                r=wanted[name];raw=tar.extractfile(member).read();assert len(raw)==r['size'];banks[r['address']]=raw
    assert stream.wait()==0 and len(banks)==len(wanted);starts=sorted(banks)
    def read(address,size=4):
        n=bisect.bisect_right(starts,address)-1;assert n>=0
        first=starts[n];raw=banks[first];assert first<=address and address+size<=first+len(raw),(hex(address),size)
        return raw[address-first:address-first+size]
    def word(address,size=4):return int.from_bytes(read(address,size),'little')
    manager=typed['manager'];collector=manager+0x10
    assert word(manager)==0x41f124 and word(collector)==0x41f478
    assert word(manager+0xa4)==0 and word(collector+4,2)&0x8000
    registry=word(0x48c93c)+8;mask=word(registry+8);keys=word(registry)
    assert mask<=0xffff and not mask&(mask+1)
    slot=((collector>>4)*0x9e3779b1)&mask;visited=[]
    while True:
        key=word(keys+slot*4);visited.append(dict(slot=slot,key=key))
        assert key!=collector,'Actual registry key is registered: native full-return storage branch unqualified'
        if not key:break
        assert len(visited)<=mask
        slot=(slot+1)&mask
    provenance=dict(archive=str(a.archive.resolve()),archive_sha256=a.archive_sha256,
        actual_typed_input=typed,actual_before_banks=len(banks),actual_before_bytes=sum(map(len,banks.values())),
        collector=collector,collector_flags=word(collector+4,2),registry=registry,mask=mask,keys=keys,
        actual_registry_probe=visited,actual_registry_key_present=False,
        captured_original_gp_frame=False,expected_after_consumed=False,supplied_callee_outcomes=False)
    child.OWNERS=(0x3232b0,0x3231b0,0x31f660,0x325af0,0x315d10,0x316dc0,0xb4150,
        0x283450,0x2e3250,0x2e2e30,0x149da0,0x149e00,0x233250,0x233420,
        0x2e12a0,0x2e11e0,0x2e33f0,0x2e3350)
    code=_Code(Xbe(NEW_DEFAULT),'.text')
    for owner in child.OWNERS:
        for pc in complete_body(code,owner).code:
            i=code.at(pc)
            n=bisect.bisect_right(starts,pc)-1
            if n>=0 and starts[n]<=pc and pc+i.size<=starts[n]+len(banks[starts[n]]):assert read(pc,i.size)==i.bytes
    child.base.reference_inputs=lambda image:(banks,provenance)
    fp=typed['floating_point']
    child.cases=lambda:[dict(name=f'actual_manager_base_lock_flags_{flag}',entry=0x3232b0,
        receiver=manager,arguments=[flag],user_words=[],before_fp_input=[fp['x87_control_word'],fp['x87_status'],fp['mxcsr']])
        for flag in (typed['disposing'],0,3)]
    source=inspect.getsource(child.main)
    needle="'tools/validate_native_agent_cleanup_children_tu15.py'"
    source=source.replace(needle,needle+",'tools/validate_native_contact_manager_base_lock_typed_before_tu15.py','include/halo2/native_tu15_base_lock.hpp','include/halo2/native_tu15_contact_manager_cleanup.hpp','include/halo2/native_tu15_activation_collector_cleanup.hpp','src/physics/native_tu15_contact_manager_cleanup.cpp','src/physics/native_tu15_activation_contact_manager.cpp','src/physics/native_tu15_island_activation.cpp'")
    needle="'native_tu15_contact_unwind_internal.hpp'):shutil.copyfile"
    assert source.count(needle)==1
    source=source.replace(needle,"'native_tu15_contact_unwind_internal.hpp','native_tu15_contact_manager_cleanup.cpp','native_tu15_activation_contact_manager.cpp','native_tu15_island_activation.cpp'):shutil.copyfile")
    needle='driver=\'#include "halo2/native_tu15_agent_cleanup_children.hpp"\\n\'+driver'
    assert source.count(needle)==1
    source=source.replace(needle,'driver=\'#include "halo2/native_tu15_base_lock.hpp"\\n#include "halo2/native_tu15_contact_manager_cleanup.hpp"\\n#include "halo2/native_tu15_agent_cleanup_children.hpp"\\n\'+driver')
    needle='NativeTu15AgentCleanupChildrenServices services;services.allocation=growth;'
    assert source.count(needle)==1
    source=source.replace(needle,needle+'''NativeTu15ContactManagerCleanupServices manager_services;manager_services.collector.allocation=growth;NativeTu15ActivationContactManagerStats collector_stats;
      manager_services.collector.destroy_body=[](auto,auto,auto,auto&){throw std::runtime_error("Unqualified actual body destructor");};
      manager_services.collector.release_base_lock=[&](auto p,auto flags,auto&){check(flags==0,"Unqualified base-lock flags");native_tu15_release_unregistered_base_lock(memory,p,observe);};''')
    needle='if(entry==0x2f82d0u)native_tu15_cleanup_cached_contact_agent'
    assert source.count(needle)==1
    source=source.replace(needle,'if(entry==0x3232b0u)result=native_tu15_delete_contact_manager(memory,receiver,c.at("arguments")[0],{},fp,manager_services,observe,&collector_stats);\\n        else '+needle)
    source=source.replace('native_tu15_array_allocator.cpp native_tu15_array_growth.cpp)', 'native_tu15_array_allocator.cpp native_tu15_array_growth.cpp native_tu15_contact_manager_cleanup.cpp native_tu15_activation_contact_manager.cpp native_tu15_island_activation.cpp)')
    needle="(project/'driver.cpp').write_text(driver)"
    assert source.count(needle)==1
    source=source.replace(needle,"driver=driver.replace('check(cases>20,','check(cases==3,').replace('      check(calls==','      check(result==c.at(\"return_word\")&&collector_stats.collector_destructions==1u&&collector_stats.base_lock_releases==1u,name+\" native base-lock lifetime differs\");\\n      check(calls==');"+needle)
    fp_needle='environment=dict(base.__dict__,observer_source=observer_source);'
    assert source.count(fp_needle)==1
    source=source.replace(fp_needle,"source=source.replace(\"case['before_fp']=fp\",\"fp=case.get('before_fp_input',fp);case['before_fp']=fp\");"+fp_needle)
    source=source.replace("target=project/'native_tu15_agent_cleanup_children.cpp'", "target=project/'include/halo2/native_tu15_base_lock.hpp'")
    begin=source.index("mutants=[('point_stride'");end=source.index('\n  for name,before,after in mutants:',begin)
    source=source[:begin]+'''mutants=[('wrong_base_byte','std::byte{0x7f}','std::byte{0x00}'),('registered_refusal','if(native_tu15_base_lock_registered(memory,object))','if(true || native_tu15_base_lock_registered(memory,object))')]'''+source[end:]
    source=source.replace("scope='Declared valid objects", "scope='Every genuine typed manager BEFORE bank with current absent registry, native base-lock2E12A0 lookup/update and native manager/collector/body-listener/reference/array lifetime; no oldCPU callbacks or supplied outcomes; ordinary marshalled offline ABI only; declared valid objects")
    environment=dict(child.__dict__);exec(source,environment)
    return environment['main']()

if __name__=='__main__':raise SystemExit(main())
