#!/usr/bin/env python3
"""Compare native TU1.5 lift-body velocity setters and activation to physical i386."""
import argparse
import copy
import inspect
import json
import os
import struct
import subprocess
from pathlib import Path

import validate_native_array_growth_tu15 as base
from oracle_smg_firing_1_1 import ROOT, Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from build_native_tu15_constraint_before_reference import sha, save

STAGE = ROOT / 'analysis/physics/native-phantom-body-velocity-stage-20261103/staged'
COMP, CELLS, BODY, MOTION, ISLAND, WORLD, VEC, TABLE = (base.UI+i*0x1000 for i in range(1, 9))
OWNERS = (0x1d9080, 0x1d9100, 0x1d92c0, 0x315c90, 0x315cb0,
          0x3165d0, 0x316d40, 0x322660, 0x3239f0, 0x323a00)


def cases():
    rows = []
    for entry in OWNERS[:3]:
        for index, (input_words, motion_words) in enumerate((
            ((0x3e800000, 0x40000000, 0xc0800000), (0x3f800000, 0xc0000000, 0x40a00000)),
            ((0x7fc12345, 0x80000000, 0x7f800000), (0x7fc98765, 0, 0xff800000)),
            ((0x7f812345, 1, 0x7f7fffff), (0x3f800000, 1, 0x7f7fffff)),
        )):
            for mode in ('active', 'static', 'no_island', 'wake_no_world', 'wake_island', 'wake_deferred', 'component_gate'):
                slot = 2 if index == 1 else 0
                cell = CELLS+0x40+slot*0x60
                wake = mode in ('wake_island', 'wake_deferred')
                deferred = mode == 'wake_deferred'
                words = {COMP+4: 2 if mode=='component_gate' else 0, COMP+0x70: CELLS, cell: BODY,
                         BODY+4: 0x00030000, BODY+8: WORLD if wake else 0, BODY+0x3c: MOTION,
                         BODY+0x40: 1 if mode=='static' else 0,
                         BODY+0x44: 0 if mode=='no_island' else ISLAND, BODY+0x74: 0,
                         MOTION: TABLE, TABLE+0x50: 0x3239f0, TABLE+0x54: 0x323a00,
                         ISLAND+0x24: 0, ISLAND+0x28: 0x100 if mode=='active' else 0, ISLAND+0x40: 0,
                         WORLD+8: WORLD+0x600, WORLD+0xc: 0, WORLD+0x10: 2,
                         WORLD+0x14: WORLD+0x700, WORLD+0x18: 1, WORLD+0x700: ISLAND,
                         WORLD+0x7c: 0, WORLD+0x10c: int(deferred), WORLD+0x108: WORLD+0x800, WORLD+0x800: BODY}
                for i, (v, current) in enumerate(zip(input_words, motion_words)):
                    words[VEC+i*4] = v
                    words[MOTION+0x40+i*4] = current
                rows.append(dict(name=f'{entry:x}_{mode}_{index}', entry=entry, receiver=VEC if entry==0x1d9100 else slot,
                                 eax=slot if entry==0x1d9100 else VEC, edx=VEC, esi=COMP,
                                 arguments=[COMP] if entry!=0x1d92c0 else [], slot=slot, component=COMP, vector=VEC,
                                 user_words=[(p-base.UI,v) for p,v in words.items()]))
    seed=next(row for row in rows if row['name']=='1d9080_wake_island_0')
    for entry in OWNERS[:3]:
        row=copy.deepcopy(next(r for r in rows if r['name']==f'{entry:x}_wake_island_0'))
        row['name']+= '_current_motion_after_wake'
        row['user_words'].append((MOTION+0x200-base.UI,TABLE))
        row['mutations']=[dict(instruction=0x316d97,address=BODY+0x3c,written=struct.pack('<I',MOTION+0x200).hex())]
        rows.append(row)
    assert seed
    return rows


def original(image, banks, rows, scratch):
    source = inspect.getsource(base.original)
    point = " mov $\u007bcase['receiver']\u007d,%ecx\n int3"
    assert source.count(point)==1
    source=source.replace(point, " mov $\u007bcase['receiver']\u007d,%ecx\n mov $\u007bcase['eax']\u007d,%eax\n mov $\u007bcase['edx']\u007d,%edx\n mov $\u007bcase['esi']\u007d,%esi\n int3")
    env=dict(base.__dict__,OWNERS=OWNERS)
    # Physical metadata builder reads the selected owner tuple from its globals.
    exec(inspect.getsource(base.metadata),env)
    exec(source,env)
    return env['original'](image,banks,rows,scratch)


def driver():
    prefix=(ROOT/'tests/native_tu15_array_growth_tests.cpp').read_text().split('int main(int argc,char**argv)')[0]
    return '#include "halo2/native_tu15_phantom_body_velocity.hpp"\n'+prefix+r'''
int main(int argc,char**argv)try{
 check(argc==2||argc==3,"Supply phantom velocity fixture");const bool mutant=argc==3;const auto path=std::filesystem::path(argv[1]);std::ifstream in(path);const auto fixture=J::parse(in);
 unsigned returns{},differences{};for(const auto&c:fixture.at("cases"))for(bool reads:{false,true}){
  if(mutant&&(reads||c.contains("fault")||c.contains("mutations")))continue;
  if(reads&&c.contains("mutations"))continue;
  if(!reads&&c.contains("fault")&&!c.at("fault").at("write").get<bool>())continue;
  Memory memory;for(const auto&b:c.at("banks"))memory.banks.emplace(b.at("address"),file(path.parent_path()/b.at("before_file").get<std::string>()));memory.fault=c.value("fault",J{});memory.log_reads=reads;
  J stores=J::array();std::set<unsigned> applied;SourcePhysicsContactRecordObserver trace;
  if(!reads)trace=[&](const auto&e){stores.push_back(J{{"instruction",e.instruction},{"address",e.address},{"before",hex(e.before)},{"written",hex(e.written)}});unsigned i{};for(const auto&m:c.value("mutations",J::array())){if(m.at("instruction")==e.instruction&&applied.insert(i).second){const auto b=unhex(m.at("written"));memory.write(m.at("address"),b);}++i;}_mm_setcsr(_mm_getcsr()^0x6000u);};
  NativeTu15RetainedStoreObserver observer(trace);
  NativeTu15PhysicsFrameFp fp{c.at("before_fp")[0],c.at("before_fp")[1],c.at("before_fp")[2]};
  const auto src=c.at("vector").get<std::uint32_t>(),component=c.at("component").get<std::uint32_t>(),slot=c.at("slot").get<std::uint32_t>(),entry=c.at("entry").get<std::uint32_t>();
  NativeTu15VelocityWords words=[&](unsigned lane){check(lane<3,"Read unprovided fourth velocity word");std::array<std::byte,4>b{};memory.read(src+lane*4,b);std::uint32_t w{};for(unsigned i=0;i<4;++i)w|=std::to_integer<std::uint32_t>(b[i])<<(i*8);return w;};
  const auto sw=status(),cw=control();const auto mx=_mm_getcsr();bool failed{};
  try{if(entry==0x1d92c0u)native_tu15_add_phantom_body_linear_velocity(memory,component,slot,words,fp,0,{},observer);
      else native_tu15_set_phantom_body_velocity(memory,component,slot,entry==0x1d9100u?NativeTu15PhantomVelocityKind::angular:NativeTu15PhantomVelocityKind::linear,words,fp,{},observer);}
  catch(const Fault&){failed=true;}
  const auto name=c.at("name").get<std::string>();check(failed==c.value("declared_fault",false),name+" completion differs");check(sw==status()&&cw==control()&&mx==_mm_getcsr(),name+" host FP differs");
  bool mismatch{};for(const auto&b:c.at("banks"))mismatch|=memory.banks.at(b.at("address"))!=file(path.parent_path()/b.at("after_file").get<std::string>());
  if(mutant){check(!failed,"Mutation refused");++returns;differences+=mismatch;continue;}
  check(J::array({fp.control_word,fp.status,fp.mxcsr})==c.at("after_fp"),name+" semantic FP differs");
  if(reads){J expected=J::array();for(const auto&e:c.at("reads"))expected.push_back(J::array({e.at("address"),e.at("bytes")}));check(memory.reads==expected,name+" retained read order differs: "+memory.reads.dump()+" expected "+expected.dump());}
  else{check(stores==c.at("writes"),name+" retained SourcePC stores differ: "+stores.dump()+" expected "+c.at("writes").dump());check(applied.size()==c.value("mutations",J::array()).size(),name+" post-store mutation phase absent");}
  check(!mismatch,name+" retained bank differs");
 }
 if(mutant){check(returns>50&&differences>0,"Mutant did not change completed retained DATA");std::cout<<J{{"complete_returns",returns},{"retained_differences",differences}}.dump()<<'\n';}
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',required=True,type=Path);a=parser.parse_args()
    a.output=a.output.resolve();a.output.mkdir(parents=True,exist_ok=False);scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve()
    report=dict(passed=False,owned_scratch=str(scratch),original_authority=NEW_PIN,scope=__doc__,retained_large_artifacts=[],live_admission=False)
    try:
        assert sha(NEW_DEFAULT)==NEW_PIN
        image=Xbe(NEW_DEFAULT);code=_Code(image,'.text');rows=cases();banks={0x474d04:struct.pack('<I',base.UI+0x9000),base.UI+0x9000:bytes(12)}
        # Authored global-pointer read is retained; zero vector is part of the
        # declared USER bank below, avoiding overlapping retained ranges.
        del banks[base.UI+0x9000]
        for row in rows:row['user_words'] += [(0x9000+i*4,0) for i in range(3)]
        fixture,authority=original(image,banks,rows,scratch);faults=[];seen=set()
        for row in rows:
            if row.get('mutations'):continue
            for key in ('reads','writes'):
                occurrences={}
                for event in row[key]:
                    size=event.get('bytes',len(event.get('written',''))//2);extent=(event['address'],size);occurrences[extent]=occurrences.get(extent,0)+1
                    site=event['instruction'],key
                    if site in seen:continue
                    seen.add(site);fresh={k:copy.deepcopy(row[k])for k in ('entry','receiver','eax','edx','esi','arguments','slot','component','vector','user_words')}
                    fresh.update(name=f'{key}_fault_{site[0]:x}',fault=dict(address=extent[0],bytes=size,occurrence=occurrences[extent],write=key=='writes'));faults.append(fresh)
        fs=scratch/'faults';fs.mkdir();ff,_=original(image,banks,faults,fs)
        for row in faults:
            assert row['declared_fault']
            for bank in row['banks']:
                for key in ('before_file','after_file'):bank[key]=str(ff/bank[key])
        save(fixture/'original.json',dict(image_sha256=NEW_PIN,cases=rows+faults));report['fixture_sha256']=sha(fixture/'original.json');report['authority']=authority
        source=STAGE/'src/physics/native_tu15_phantom_body_velocity.cpp';activation=ROOT/'src/physics/native_tu15_island_activation.cpp';cpp=scratch/'driver.cpp';cpp.write_text(driver())
        pins={str(p.relative_to(ROOT)):sha(p)for p in (source,activation,STAGE/'include/halo2/native_tu15_phantom_body_velocity.hpp',ROOT/'src/physics/native_tu15_joint_constraint_numeric_internal.hpp',ROOT/'include/halo2/native_tu15_retained_store_observer.hpp',ROOT/'tools/observe_native_array_growth_tu15.cpp',ROOT/'tools/validate_native_array_growth_tu15.py',ROOT/'tests/native_tu15_array_growth_tests.cpp',Path(__file__))}
        changes=(('swap-output','angular ? 0x50u : 0x40u','angular ? 0x40u : 0x50u'),('wrong-zero','velocity(2), 0}','velocity(2), 1}'))
        report['results']=[]
        for compiler in ('g++','clang++'):
            for mutant,old,new in (('native','',''),*changes):
                binary=scratch/(compiler+'-'+mutant);unit=source
                if mutant!='native':
                    original_text=source.read_text();assert original_text.count(old)==1;unit=scratch/(mutant+'.cpp');unit.write_text(original_text.replace(old,new))
                cmd=[compiler,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-I'+str(STAGE/'include'),'-I'+str(ROOT/'include'),'-I'+str(ROOT/'src/physics'),str(cpp),str(unit),str(activation),'-o',str(binary)]
                p=subprocess.run(cmd,capture_output=True,text=True);(a.output/(binary.name+'-compile.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,p.stderr[-4000:]
                p=subprocess.run([str(binary),str(fixture/'original.json'),*(['mutant'] if mutant!='native'else[])],capture_output=True,text=True);(a.output/(binary.name+'.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,p.stderr[-4000:]
                report['results'].append(dict(compiler=compiler,configuration=mutant,mutation=json.loads(p.stdout)if mutant!='native'else None))
        executed={pc for row in rows+faults for pc in row['executed_sites']};base.OWNERS=OWNERS
        assert all(sha(ROOT/n)==h for n,h in pins.items())
        report.update(passed=True,source_sha256=pins,compiled_driver_sha256=sha(cpp),full_returns=len(rows),fault_prefixes=len(faults),
                      reads=sum(len(row['reads'])for row in rows+faults),stores=sum(len(row['writes'])for row in rows+faults),
                      coverage={hex(o):dict(executed=len(executed&set(base.complete_body(code,o).code)),total=len(base.complete_body(code,o).code))for o in OWNERS})
    except BaseException as error:report['error']=str(error)or repr(error);raise
    finally:
        report['temporary_allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in scratch.rglob('*')if p.is_file())
        save(a.output/'report.json',report)


if __name__=='__main__':main()
