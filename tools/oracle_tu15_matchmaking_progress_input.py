#!/usr/bin/env python3
"""Original progress cancellation consumers and leave-count caller/getters.

Pane and frozen UI clock observations are explicit inputs to2584AE. The
2584C9 leave-count caller executes unchanged19475B/1946AA and their actual
session/membership getters over explicit synthetic session banks, stopping
before dialog allocation. No original UI/network producer or live-flow claim.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import UC_X86_REG_EAX, UC_X86_REG_ECX, UC_X86_REG_EDI, UC_X86_REG_EIP, UC_X86_REG_ESP
from oracle_game_variants import Source, STACK, STOP
from research_mp_sandbox_update_functions import NEW_DEFAULT, NEW_PIN

def policy_lines(raw):
    lines=raw.decode().splitlines()
    if lines[:1]==["Shutdown handler: initialize."] and lines[-1:]==["Shutdown handler: cleanup."]:
        return lines[1:-1]
    return lines


parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report',type=Path,required=True)
parser.add_argument('--fixture',type=Path)
parser.add_argument('--native',type=Path)
parser.add_argument('--host-local',action='store_true')
parser.add_argument('--expanded-party',action='store_true')
args=parser.parse_args()
root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
source=Source(dict(path=str(NEW_DEFAULT.resolve()),pin=NEW_PIN,globals=0x4ecc1c));m=source.g
m.mem_map(0x1100000,0x10000);session=0x1100000;owner=0x110e000
def write(at,value):m.mem_write(at,struct.pack('<I',value&0xffffffff))
def run(entry,stops):
 write(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK)
 def stop(g,at,size,data):
  if at in stops:g.emu_stop()
 hook=m.hook_add(UC_HOOK_CODE,stop)
 try:m.emu_start(entry,STOP,count=10000)
 finally:m.hook_del(hook)
 at=m.reg_read(UC_X86_REG_EIP);assert at in stops,hex(at);source.executed.discard(at);return at
cases=[]
m.mem_write(0x51d048,b'\x01')
for pane in range(4):
 for now in (0,0x12345678,0xfffffffe):
  for elapsed in (0,1,59999,60000,60001,0x7fffffff,0x80000000,0x80000001,0xfffffffe,0xffffffff):
   write(0x51d04c,now);write(owner+0x614,now-elapsed);write(STACK+4,owner);m.reg_write(UC_X86_REG_EAX,pane)
   allowed=run(0x2584ae,{0x2584c9,0x258515})==0x2584c9
   cases.append(dict(kind='cancel',pane=pane,now=now,elapsed=elapsed,original=int(allowed)))
m.mem_write(0x535f00,b'\x01');write(0x535f34,session);write(session+0x741c,3)
for current in (0,1,7,15):
 for local in range(1,5):
  for remote in range(13):
   n=local+remote
   for sparse in (False,True):
    slots=([*range(0,16,2),*range(1,16,2)] if sparse else list(range(16)))[:n]
    owners=[current]*local+[(current+1+i//4)%16 for i in range(remote)]
    write(session+0x72d8,current);write(session+0x40,current if args.host_local else (current+1)%16);write(session+0x1118,n);write(session+0x111c,sum(1<<s for s in slots))
    for slot,member in zip(slots,owners):write(session+0x1120+slot*0x13c+0xc,member)
    run(0x2584c9,{0x2584e3})
    total=m.reg_read(UC_X86_REG_EAX);observed_local=m.reg_read(UC_X86_REG_EDI);dialog=m.reg_read(UC_X86_REG_ECX)
    assert (total,observed_local)==(n,local),(total,observed_local,n,local)
    cases.append(dict(kind='leave',total=n,local=local,connections=len(set(owners)),current_member=current,
                      host_member=current if args.host_local else (current+1)%16,sparse=sparse,original=dialog))
decoder=Cs(CS_ARCH_X86,CS_MODE_32)
for at in source.executed:
 i=next(decoder.disasm(source.image.read(at,15),at));assert bytes(m.mem_read(at,i.size))==i.bytes

header=root/'tests/reference/frontend_progress_before_rust_20261102/frontend_matchmaking_progress.hpp.reference'
state=root/'tests/reference/frontend_progress_before_rust_20261102/frontend_state.cpp.reference'
text=state.read_text();start=text.index('    const auto party_players =',text.index('if (frame().screen == 210)'))
end=text.index('    return;',start);body=text[start:end]
cpp=scratch/'legacy.cpp';binary=scratch/'legacy'
cpp.write_text('#include '+json.dumps(str(header))+'\n#include<iostream>\nusing namespace halo2;\n'+r'''
struct Party{std::vector<unsigned> members;};struct Account{std::optional<Party> party;};
struct Leave{Account account_;std::vector<unsigned> locals;unsigned chosen{};
std::vector<unsigned> local_players()const{return locals;}void dialog(unsigned id,int){chosen=id;}
void choose(){
'''+body+r'''
}};
int main(){char kind;unsigned a,b;while(std::cin>>kind>>a>>b){if(kind=='C')std::cout<<matchmaking_progress_cancel_allowed(a,b)<<"\n";
else{Leave s;s.account_.party=Party{std::vector<unsigned>(a)};s.locals.resize(b);s.choose();std::cout<<s.chosen<<"\n";}}}
''')
subprocess.run(['c++','-std=c++20',str(cpp),'-o',str(binary)],check=True)
legacy_data=''.join(f"C {c['pane']} {c['elapsed']}\n" if c['kind']=='cancel' else f"L {c['total'] if args.expanded_party else c['connections']} {c['local']}\n" for c in cases)
legacy=list(map(int,subprocess.check_output([str(binary)],input=legacy_data.encode()).split()))
assert len(legacy)==len(cases)
owner_rs=root/'rust/online_protocol/src/matchmaking_frontend_progress.rs'
if args.native:command=[str(args.native.resolve(strict=True)),'--input']
else:
 probe=scratch/'input.rs';binary=scratch/'input-rust'
 probe.write_text('#[path='+json.dumps(str(owner_rs))+'] mod core;\n'+r'''
use std::io::{self,BufRead};
fn main(){for line in io::stdin().lock().lines(){let l=line.unwrap();let v:Vec<&str>=l.split_whitespace().collect();
let a:u32=v[1].parse().unwrap();let b:u32=v[2].parse().unwrap();
println!("{}",if v[0]=="C"{core::cancel_allowed(a,b) as u32}else{core::leave_dialog(a as usize,b as usize)});}}
''')
 subprocess.run(['rustup','run','1.96.0','rustc','--edition=2024','-Adead_code',str(probe),'-o',str(binary)],check=True)
 command=[str(binary)]
data=''.join(f"C {c['pane']} {c['elapsed']}\n" if c['kind']=='cancel' else f"L {c['total']} {c['local']}\n" for c in cases)
actual=[int(line) for line in policy_lines(subprocess.check_output(command,input=data.encode()))];assert len(actual)==len(cases)
differences=[];legacy_differences=[]
for c,a,b in zip(cases,actual,legacy):
 if a!=c['original']:differences.append(dict(case=c,actual=a))
 if b!=c['original']:legacy_differences.append(dict(case=c,legacy=b))
fixture=dict(schema='H2_TU15_PROGRESS_INPUT_ORIGINAL_V1',original_sha256=NEW_PIN,cases=cases)
raw=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
if args.fixture:args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(raw)
report=dict(passed=not differences,cases=len(cases),mismatches=len(differences),first_mismatches=differences[:4],
 legacy_differences=len(legacy_differences),expanded_party_counterfactual=args.expanded_party,host_local_counterfactual=args.host_local,legacy_cancel_differences=sum(c['case']['kind']=='cancel' for c in legacy_differences),
 legacy_leave_differences=sum(c['case']['kind']=='leave' for c in legacy_differences),
 minimal_leave_counterexample=next((c for c in legacy_differences if c['case']['kind']=='leave' and c['case']['total']==3 and c['case']['local']==2),None),
 original_instruction_addresses=len(source.executed),no_instruction_patches=True,no_replaced_callees=True,
 original_sha256=NEW_PIN,fixture_sha256=hashlib.sha256(raw).hexdigest(),rust_source_sha256=hashlib.sha256(owner_rs.read_bytes()).hexdigest(),
 backend='native CXX/Rust wrapper' if args.native else 'standalone current Rust owner',
 native_binary_sha256=hashlib.sha256(args.native.read_bytes()).hexdigest() if args.native else None,
 scope=__doc__,scratch=str(scratch),retained_large_artifacts=[])
args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
raise SystemExit(0 if report['passed'] else 1)
