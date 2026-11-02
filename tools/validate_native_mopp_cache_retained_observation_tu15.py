#!/usr/bin/env python3
"""Explicit MOPP cache observation capability and unchanged raw refusal."""
import argparse, hashlib, json, os, shutil, subprocess
from pathlib import Path
from oracle_smg_firing_1_1 import Xbe, _Code
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN
from capstone.x86_const import X86_OP_MEM

ROOT = Path(__file__).resolve().parents[1]
PAIRS = [(0x30a1ef,0),(0x30a1f4,0x30),(0x30a210,0x20),(0x30a22f,0x10),
         (0x30a24f,0x24),(0x30a26f,0x14),(0x30a28f,0x28),(0x30a2cd,0x18),
         (0x30a030,0),(0x30a035,0x30),(0x30a0d3,0x20),(0x30a0f3,0x10),
         (0x30a114,0x24),(0x30a135,0x14),(0x30a156,0x28),(0x30a190,0x18)]
SOURCE = r'''
#include "halo2/native_tu15_mopp_aabb_private_cache.hpp"
#include "native_tu15_joint_constraint_numeric_internal.hpp"
#include <iostream>
#include <limits>
using namespace halo2;
void check(bool ok,const char*message){if(!ok)throw std::runtime_error(message);}
struct Bank:SourcePhysicsContactMemory,NativeTu15RetainedStoreObservationDomain,NativeTu15RetainedStoreObserverProvider{
 std::array<std::byte,256>data;mutable unsigned reads{},policies{};unsigned writes{},events{};bool blocked{};
 SourcePhysicsContactRecordObserver callback;
 Bank(){data.fill(std::byte{0x5a});callback=[this](const auto&e){++events;check(e.address>=0x1000&&e.address<0x1100,"private event escaped");};}
 const SourcePhysicsContactRecordObserver&retained_store_observer()const noexcept override{return callback;}
 bool observes_retained_store(std::uint32_t,std::uint32_t p,std::size_t)const override{
  ++policies;if(blocked)throw std::runtime_error("outer retained alias refused");return p<0x20000u||p>=0x20040u;}
 void read(std::uint32_t p,std::span<std::byte>v)const override{check(p>=0x1000u&&std::uint64_t(p)+v.size()<=0x1100u,"private preimage reached bank");++reads;std::copy_n(data.begin()+p-0x1000u,v.size(),v.begin());}
 void write(std::uint32_t p,std::span<const std::byte>v)override{check(p>=0x1000u&&std::uint64_t(p)+v.size()<=0x1100u,"private write reached bank");++writes;std::copy(v.begin(),v.end(),data.begin()+p-0x1000u);}
};
int main(){try{unsigned controls{};auto refuses=[&](auto f){bool rejected{};try{f();}catch(const std::runtime_error&){rejected=true;}check(rejected,"negative admitted");++controls;};
 Bank bank;SourcePhysicsContactRecordObserver empty;
 NativeTu15MoppAabbPrivateCache raw(bank,0x20000u);joint_constraint_numeric::Memory raw_memory(raw,empty);
 refuses([&]{raw_memory.word(0x30a1efu,0x20000u,9);});
 check(!raw.retained_store_observer()&&!raw.data_is_defined(0x20000u,4),"raw/default mode changed");++controls;
 // A genuine address inside the existing retained bank must refuse before
 // local definition, backing DATA access, or observer publication.
 const auto bank_before=bank.data;
 NativeTu15MoppAabbPrivateCache alias(bank,0x1040u,NativeTu15MoppAabbRetainedDataObservation{});
 joint_constraint_numeric::Memory alias_memory(alias,empty);
 refuses([&]{alias_memory.word(0x30a1efu,0x1040u,99);});
 check(!alias.data_is_defined(0x1040u,4)&&bank.data==bank_before&&!bank.reads&&!bank.writes&&!bank.events,"retained alias touched DATA");++controls;
 NativeTu15MoppAabbPrivateCache local(bank,0x20000u,NativeTu15MoppAabbRetainedDataObservation{});
 joint_constraint_numeric::Memory m(local,empty);
 const std::array<std::array<std::uint32_t,2>,16>pairs{{@PAIRS@}};
 for(auto pair:pairs){m.word(pair[0],0x20000u+pair[1],pair[0]);check(local.data_is_defined(0x20000u+pair[1],4),"Source cache word undefined");++controls;}
 check(!bank.events&&!bank.reads&&!bank.writes,"cache preimage/event/delegation differs");++controls;
 check(&local.retained_store_observer()==&bank.callback,"retained observer target was not forwarded");++controls;
 check(!local.data_is_defined(0x20004u,4),"cache hole fabricated");++controls;
 std::array<std::byte,4>value{};refuses([&]{local.read(0x20004u,value);});
 m.word(0x309f24u,0x1004u,7);check(bank.events==1&&bank.reads==1&&bank.writes==1,"implicit retained event lost");++controls;
 refuses([&]{m.word(0x2f3d6bu,0x20000u,99);});
 refuses([&]{m.word(0x30a1efu,0x20004u,99);});
 refuses([&]{(void)local.observes_retained_store(0x30a1efu,0x20000u,8);});
 refuses([&]{(void)local.observes_retained_store(0x30a1efu,0x2003fu,4);});
 refuses([&]{(void)local.observes_retained_store(0x30a1efu,0x20020u,std::numeric_limits<std::size_t>::max()-15u);});
 refuses([&]{local.require_retained_extent(0x20000u,4);});
 refuses([&]{local.copy_defined_data(0x303ba0u,0x1000u,0x20000u,4);});
 bank.blocked=true;refuses([&]{m.word(0x30a1efu,0x20000u,99);});bank.blocked=false;
 check(m.word(0x20000u)==0x30a030u,"refused outer policy committed cache write");++controls;
 joint_constraint_numeric::Memory ordinary(bank,empty);ordinary.word(0x30a1efu,0x1008u,13);
 check(bank.events==2&&bank.reads==2&&bank.writes==2&&!local.data_is_defined(0x20004u,4),"public numeric default changed");++controls;
 std::cout<<controls<<"\n";return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<"\n";return 1;}}
'''


def main():
    parser=argparse.ArgumentParser();parser.add_argument('--stage',required=True,type=Path);parser.add_argument('--report',required=True,type=Path);a=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);include=scratch/'include';shutil.copytree(ROOT/'include',include)
    shutil.copytree(a.stage/'staged/include',include,dirs_exist_ok=True)
    numeric=a.stage/'staged/src/physics/native_tu15_joint_constraint_numeric_internal.hpp';shutil.copyfile(numeric,scratch/numeric.name)
    header=include/'halo2/native_tu15_mopp_aabb_private_cache.hpp';original=header.read_text()
    code=_Code(Xbe(NEW_DEFAULT),'.text');source_stores=[]
    for pc,offset in PAIRS:
        i=code.at(pc);op=i.operands[0];assert i.mnemonic=='mov'and op.type==X86_OP_MEM and op.size==4 and i.reg_name(op.mem.base)=='ecx'and op.mem.disp==offset and not op.mem.index
        source_stores.append(dict(instruction=pc,offset=offset,bytes=i.bytes.hex(),assembly=i.mnemonic+' '+i.op_str))
    source=scratch/'controls.cpp';source.write_text(SOURCE.replace('@PAIRS@',','.join('{'+hex(p)+','+hex(n)+'}'for p,n in PAIRS)))
    mutants={
     'disable_raw_refusal':('if(!retained_only_)throw std::runtime_error("Private MOPP cache raw observation requires its owner");','if(false)throw std::runtime_error("Private MOPP cache raw observation requires its owner");'),
     'expose_private_preimage':('            return false;\n        }\n        const auto*d=','            return true;\n        }\n        const auto*d='),
     'drop_outer_policy':('if(d->observes_retained_store(pc,p,n))','if(false)'),
     'ignore_outer_retained':('if(d->observes_retained_store(pc,p,n))','if((d->observes_retained_store(pc,p,n),false))'),
     'drop_implicit_observer':('if(retained_only_)if(const auto*p=','if(false)if(const auto*p='),
     'provide_cache_holes':(':memory_(memory),identity_(identity){',':memory_(memory),identity_(identity){defined_.fill(true);'),
    }
    report=dict(passed=False,owner=str(scratch),source_stores=source_stores,original_authority=NEW_PIN,results=[],source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),header_sha256=hashlib.sha256(original.encode()).hexdigest(),numeric_header_sha256=hashlib.sha256(numeric.read_bytes()).hexdigest(),scope='Explicit retained-only cache capability; source-defined64B mask retained, raw default and retained extent/copy refusals unchanged; no raw private preimages/fault domain.')
    try:
        for compiler in ('g++','clang++'):
            for name in ('baseline',*mutants):
                text=original
                if name!='baseline':
                    before,after=mutants[name];assert text.count(before)==1,name;text=text.replace(before,after)
                header.write_text(text);exe=scratch/(compiler.replace('+','p')+'-'+name)
                command=[compiler,'-std=c++20','-O0','-I'+str(include),'-I'+str(scratch),str(source),'-o',str(exe)]
                build=subprocess.run(command,capture_output=True,text=True);assert build.returncode==0,build.stderr
                run=subprocess.run([str(exe)],capture_output=True,text=True);passed=run.returncode==0 if name=='baseline'else run.returncode!=0
                row=dict(compiler=compiler,variant=name,passed=passed,returncode=run.returncode,diagnostics=(run.stdout+run.stderr)[-4000:]);report['results'].append(row);assert passed,row
        report['passed']=True
    finally:
        header.write_text(original);a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(dict(passed=True,report=str(a.report))));return 0

if __name__=='__main__':raise SystemExit(main())
