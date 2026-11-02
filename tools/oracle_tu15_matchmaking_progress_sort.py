#!/usr/bin/env python3
"""Unchanged TU1.5 progress sort caller through its real original callees.

Explicit synthetic session/player banks; stops before widget allocation.
No networking, source slot producer or delivered runtime claim.
"""
import argparse
import hashlib,json,os,random,struct,subprocess,sys
from pathlib import Path
def policy_lines(raw):
    lines=raw.decode().splitlines()
    if lines[:1]==["Shutdown handler: initialize."] and lines[-1:]==["Shutdown handler: cleanup."]:
        return lines[1:-1]
    return lines


parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report',type=Path,required=True)
parser.add_argument('--fixture',type=Path)
parser.add_argument('--native',type=Path)
args=parser.parse_args()
root=Path(__file__).resolve().parents[1];scratch=Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
sys.path.insert(0,str(root/'tools'))
from oracle_game_variants import Source,STACK,STOP
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from capstone import Cs,CS_ARCH_X86,CS_MODE_32
from unicorn import UC_HOOK_CODE
from unicorn.x86_const import *
source=Source(dict(path=str(NEW_DEFAULT.resolve()),pin=NEW_PIN,globals=0x4ecc1c));m=source.g
m.mem_map(0x1100000,0x10000);session=0x1100000;frame=0x110e000
def write(at,value):m.mem_write(at,struct.pack('<I',value&0xffffffff))
m.mem_write(0x535f00,b'\x01');write(0x535f3c,session);write(session+0x741c,3)
visits=0;calls=0
def observe(g,at,size,data):
 global visits,calls
 if at==0x257950:g.emu_stop();return
 visits+=1
 if at in (0x257692,0x2576d7,0x25770a):calls+=1
hook=m.hook_add(UC_HOOK_CODE,observe)
cases=[]
for n in range(17):
 for ranked,teams in [(False,False),(False,True),(True,False),(True,True)]:
  patterns=[[(0,0)]*n,[(i%2,10) for i in range(n)],[(0,i) for i in range(n)],[(0,n-i) for i in range(n)],[(-1 if i%2 else 1,127 if i%3 else -1) for i in range(n)]]
  for players in patterns:cases.append(dict(ranked=ranked,teams=teams,players=players))
for ranked,teams in [(False,False),(False,True),(True,False),(True,True)]:
 cases.append(dict(ranked=ranked,teams=teams,players=[(1,10),(0,5),(1,30),(0,20)]))
rng=random.Random(0x25790e)
for _ in range(512):
 n=rng.randrange(17);cases.append(dict(ranked=bool(rng.randrange(2)),teams=bool(rng.randrange(2)),players=[(rng.choice([-128,-1,0,1,2,7,15,127]),rng.choice([-128,-1,0,1,2,24,25,49,50,127])) for i in range(n)]))
for c in cases:
 players=c['players'];n=len(players);write(session+0x111c,(1<<n)-1)
 for i,(team,value) in enumerate(players):
  props=session+0x1120+i*0x13c+0xa8
  m.mem_write(props+0x7c,bytes([team&255]));m.mem_write(props+0x7e,bytes([value&255]))
 m.mem_write(frame-0x100,bytes(0x100))
 write(frame-0x10,n);write(frame-8,n)
 m.mem_write(frame-1,bytes([c['ranked']]));m.mem_write(frame-2,bytes([c['teams']]))
 for i in range(16):write(frame-0x54+4*i,i if i<n else -1)
 write(STACK,STOP);m.reg_write(UC_X86_REG_ESP,STACK);m.reg_write(UC_X86_REG_EBP,frame);m.reg_write(UC_X86_REG_EBX,3);m.reg_write(UC_X86_REG_EDI,n)
 m.emu_start(0x25790e,STOP,count=300000)
 assert m.reg_read(UC_X86_REG_EIP)==0x257950,hex(m.reg_read(UC_X86_REG_EIP))
 source.executed.discard(0x257950)
 c['original']=list(struct.unpack('<'+'I'*n,m.mem_read(frame-0x54,n*4))) if n else []
 assert sorted(c['original'])==list(range(n))
m.hook_del(hook)
decoder=Cs(CS_ARCH_X86,CS_MODE_32)
for at in source.executed:
 ins=next(decoder.disasm(source.image.read(at,15),at));assert bytes(m.mem_read(at,ins.size))==ins.bytes
header=root/'tests/reference/frontend_progress_before_rust_20261102/frontend_matchmaking_progress.hpp.reference'
cpp=scratch/'before.cpp';binary=scratch/'before'
cpp.write_text('#include "'+str(header)+'"\n#include<iostream>\nint main(){unsigned n,a,b;while(std::cin>>n>>a>>b){std::vector<halo2::MatchmakingProgressPlayer> p;for(unsigned i=0;i<n;++i){int x,y;std::cin>>x>>y;p.push_back({x,y});}auto v=halo2::matchmaking_progress_order(p,a,b);for(auto i:v)std::cout<<i<<" ";std::cout<<"\\n";}}\n')
subprocess.run(['c++','-std=c++20','-I',str(root/'include'),str(cpp),'-o',str(binary)],check=True)
data=''.join(' '.join(map(str,[len(c['players']),int(c['ranked']),int(c['teams']),*(v for row in c['players'] for v in row)]))+'\n' for c in cases)
lines=subprocess.check_output([str(binary)],input=data.encode()).decode().splitlines();assert len(lines)==len(cases)
legacy_mismatches=[]
for c,line in zip(cases,lines):
 actual=list(map(int,line.split()));c['cpp_before']=actual
 if actual!=c['original']:legacy_mismatches.append(c)
fixture={'schema':'H2_TU15_MATCHMAKING_PROGRESS_SORT_ORIGINAL_V1','original_tu15_sha256':NEW_PIN,'entry':'0x25790E','stop':'0x257950','qsort':'0x32B560','comparators':['0x257692','0x2576D7','0x25770A'],'game_session_getter':'0x5A340','membership':'0x667C0','player_getter':'0x195301','cases':cases}
fixture_bytes=(json.dumps(fixture,separators=(',',':'))+'\n').encode()
if args.fixture:
 args.fixture.parent.mkdir(parents=True,exist_ok=True);args.fixture.write_bytes(fixture_bytes)
owner=root/'rust/online_protocol/src/matchmaking_frontend_progress.rs'
if args.native:
 command=[str(args.native.resolve(strict=True)),'--sort']
else:
 probe=scratch/'sort.rs';binary=scratch/'sort-rust'
 probe.write_text('#[path='+json.dumps(str(owner))+'] mod core;\n'+r'''use std::io::{self,BufRead};
fn main(){for line in io::stdin().lock().lines(){let l=line.unwrap();let v:Vec<i32>=l.split_whitespace().map(|s|s.parse().unwrap()).collect();
let n=v[0] as usize;let p:Vec<_>=(0..n).map(|i|core::Player{team:v[3+2*i],value:v[4+2*i]}).collect();
for i in core::order(&p,v[1]!=0,v[2]!=0){print!("{} ",i);}println!();}}
''')
 subprocess.run(['rustup','run','1.96.0','rustc','--edition=2024','-Adead_code',str(probe),'-o',str(binary)],check=True)
 command=[str(binary)]
actual=policy_lines(subprocess.check_output(command,input=data.encode()))
assert len(actual)==len(cases)
mismatches=[]
for c,line in zip(cases,actual):
 order=list(map(int,line.split()))
 if order!=c['original']:mismatches.append(dict(case=c,actual=order))
report={'passed':not mismatches,'original_execution_passed':True,'cases':len(cases),
 'mismatches':len(mismatches),'first_counterexamples':mismatches[:4],
 'legacy_stable_sort_differences':len(legacy_mismatches),'legacy_first_counterexamples':legacy_mismatches[:4],
 'original_instruction_addresses':len(source.executed),'original_instruction_visits':visits,'original_comparator_calls':calls,
 'source_sha256':NEW_PIN,'no_instruction_patches':True,'no_replaced_callees':True,
 'backend':'native CXX/Rust wrapper' if args.native else 'standalone current Rust owner',
 'rust_source_sha256':hashlib.sha256(owner.read_bytes()).hexdigest(),
 'native_binary_sha256':hashlib.sha256(args.native.read_bytes()).hexdigest() if args.native else None,
 'scope':__doc__,'fixture_sha256':hashlib.sha256(fixture_bytes).hexdigest(),'scratch':str(scratch),'retained_large_artifacts':[]}
args.report.parent.mkdir(parents=True,exist_ok=True)
args.report.write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))
raise SystemExit(1 if mismatches else 0)
