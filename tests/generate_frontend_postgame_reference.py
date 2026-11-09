#!/usr/bin/env python3
"""Authenticate modern postgame callers, the leave action and two PCR factory hooks.

The rest of the actual native PCR factory must remain byte-identical after
substituting only the captured prior-report and navigation statements. Actual
native factory/rendering/world IO is outside this caller comparison. The two
existing account helper definitions are captured unchanged and reach their
production Rust leaves on the C++ side.
"""
import argparse,hashlib,json,re
from pathlib import Path
def definition(text,token):
 at=text.index(token);start=text.rfind('\n',0,at)+1;end=text.index('{',at)+1;depth=1
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
def body(p):
 s='\n'.join(line for line in p.read_text().splitlines()if not line.startswith('#include'));assert s.count('namespace halo2 {')==1;s=s.replace('namespace halo2 {','',1);s,n=re.subn(r'\n}\s*(?:// namespace halo2)?\s*\Z','\n',s);assert n==1;return s
def leave_action(text):
 accept=definition(text,'MenuState::accept(')
 start='  } else if (entry.destination == -18) {\n'
 end='  } else if (entry.destination == -11) {'
 assert accept.count(start)==accept.count(end)==1
 assert accept.endswith('  ++revision;\n}')
 return accept.split(start,1)[1].split(end,1)[0]
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--root',type=Path,required=True);p.add_argument('--before',type=Path,required=True);p.add_argument('--after',type=Path,required=True);p.add_argument('--helpers',type=Path,required=True);a=p.parse_args();proof=json.loads((a.reference/'provenance.json').read_text())
 def captured(n):
  f=a.reference/proof['capture_names'][n];assert hashlib.sha256(f.read_bytes()).hexdigest()==proof['source_pins'][n];return f.read_text()
 old=captured('src/ui/frontend_state.cpp');new=(a.root/'src/ui/frontend_state.cpp').read_text();captured('include/halo2/frontend.hpp')
 methods=['online_postgame_host(','update_online_postgame(','has_online_prior_stats(','review_online_postgame(','close_online_prior_stats(']
 before=[definition(old,'MenuState::'+n)for n in methods];after=[body(a.root/'src/ui/frontend_matchmaking_postgame.cpp')]+[definition(new,'MenuState::'+n)for n in methods]
 hooks=[('if(id==16 && f.vista_pcr->online)online_prior_report_=f.vista_pcr->lobby_review;','postgame_flow_drive(5,&f,id);','capture'),('if(id!=185 || !f.vista_pcr->online)stack_.clear();','postgame_flow_drive(6,&f,id);','navigation')]
 factory=definition(old,'MenuState::open_vista_pcr(')
 for statement,replacement,name in hooks:
  assert old.count(statement)==1 and new.count(replacement)==1;factory=factory.replace(statement,replacement)
  before.append('void MenuState::postgame_'+name+'(unsigned id,const MenuFrame&f){'+statement+'}')
  after.append('void MenuState::postgame_'+name+'(unsigned id,const MenuFrame&f){'+replacement+'}')
 assert factory==definition(new,'MenuState::open_vista_pcr(')
 legacy_leave=leave_action(old)
 rust_leave=leave_action(new)
 assert rust_leave==legacy_leave.splitlines(keepends=True)[0]+'    if (!postgame_flow_drive(7)) return;\n'
 for parts,branch in [(before,legacy_leave),(after,rust_leave)]:
  parts.append('void MenuState::confirm_postgame_leave(){\n'+branch+'  ++revision;\n}')
 helper_old=captured('src/network/online_account.cpp');helper_new=(a.root/'src/network/online_account.cpp').read_text();helpers=[]
 for n in ['bool online_party_follower(','bool online_party_leader_left_postgame(']:
  d=definition(helper_old,n);assert d==definition(helper_new,n);helpers.append(d)
 for p,parts in [(a.before,before),(a.after,after)]:p.parent.mkdir(parents=True,exist_ok=True);p.write_text('\n'.join(parts)+'\n')
 a.helpers.parent.mkdir(parents=True,exist_ok=True);a.helpers.write_text('namespace halo2 {\n'+'\n'.join(helpers)+'\n}\n')
if __name__=='__main__':main()
