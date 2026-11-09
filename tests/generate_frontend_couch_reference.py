#!/usr/bin/env python3
"""Authenticated whole couch caller, complete controller/identity callers and exact Back branches.

Back's void returns map to handled=true in the branch wrapper. Profile-list
conditions are bound to id/name parameters. The remaining dispatcher and actual
rendering/provider/profile IO are not replaced or qualified by this fixture.
"""
import argparse,hashlib,json,re
from pathlib import Path
def definition(text,token):
 at=text.index(token);start=text.rfind('\n',0,at)+1;end=text.index('{',at)+1;depth=1
 while depth:depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]
def body(path):
 s='\n'.join(line for line in path.read_text().splitlines()if not line.startswith('#include'));assert s.count('namespace halo2 {')==1;s=s.replace('namespace halo2 {','',1);s,n=re.subn(r'\n}\s*(?:// namespace halo2)?\s*\Z','\n',s);assert n==1;return s
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--root',type=Path,required=True);p.add_argument('--before',type=Path,required=True);p.add_argument('--after',type=Path,required=True);a=p.parse_args();proof=json.loads((a.reference/'provenance.json').read_text())
 def captured(n):
  p=a.reference/proof['capture_names'][n];assert hashlib.sha256(p.read_bytes()).hexdigest()==proof['source_pins'][n];return p
 old_state=captured('src/ui/frontend_state.cpp').read_text();state=(a.root/'src/ui/frontend_state.cpp').read_text();old=captured('src/ui/frontend_couch.cpp');parts=[body(old)]
 methods=['controller_slot(','finish_local_signin(','leave_local_controller(','online_identity(']
 parts += [definition(old_state,'MenuState::'+n)for n in methods]
 couch=definition(old_state,'if(online_couch_screen(frame())) {');joining=definition(old_state,'if(joining_controller_ && frame().screen==10) {')
 parts.append('bool MenuState::couch_back(bool joining){if(joining){'+joining.replace('return;','return true;')+'}else{'+couch.replace('return;','return true;')+'}return false;}')
 old_back=definition(old_state,'MenuState::back(');prefix=old_back[:old_back.index('  if(frame().vista_leave_task)')]
 expected=prefix.replace(couch,'  if(couch_back())return;').replace(joining,'  if(couch_back(true))return;')
 actual=definition(state,'MenuState::back(');assert actual[:actual.index('  if(frame().vista_leave_task)')]==expected
 guest='id==10 && joining_online_';condition='id==10 && joining_controller_ &&\n          (profiles_[i]==signed_in_ || std::find(local_profiles_.begin(),local_profiles_.end(),profiles_[i])!=local_profiles_.end())'
 assert old_state.count(guest)==1 and old_state.count(condition)==1
 assert state.count('if(party_flow_.couch_guest_entry(id))')==1 and state.count('if(!couch_profile_entry(id,profiles_[i]))continue;')==1
 parts.append('bool MenuState::couch_profile_entry(unsigned id,const std::string&name)const{return !('+condition.replace('profiles_[i]','name')+');}')
 parts.append('bool MenuState::guest_entry(unsigned id)const{return '+guest+';}')
 old_header=captured('include/halo2/frontend.hpp').read_text();assert 'return joining_controller_?joining_controller_:online_local_options_;'in old_header;assert 'return party_flow_.active_controller();'in(a.root/'include/halo2/frontend.hpp').read_text()
 a.before.parent.mkdir(parents=True,exist_ok=True);a.before.write_text('// Exact captured modern callers and explicitly declared branch wrappers.\n'+'\n'.join(parts)+'\n')
 files=[a.root/'src/ui/frontend_couch.cpp',a.root/'src/ui/frontend_matchmaking_couch.cpp'];new=[body(p)for p in files];new += [definition(state,'MenuState::'+n)for n in methods];new.append('bool MenuState::guest_entry(unsigned id)const{return party_flow_.couch_guest_entry(id);}')
 a.after.parent.mkdir(parents=True,exist_ok=True);a.after.write_text('\n'.join(new)+'\n')
if __name__=='__main__':main()
