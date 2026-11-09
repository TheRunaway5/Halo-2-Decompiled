#!/usr/bin/env python3
"""Authenticate complete modern sign-in callers and declared dispatcher/storage seams.

Open's authentication prefix, Account acceptance and Back cancellation are
extracted exactly. Key parser/serialization and node-handle handoff are declared
wrappers around the captured statements; unrelated profile/menu logic is not
included. Providers, credentials, filesystem and actual gameplay are native IO.
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
  f=a.reference/proof['capture_names'][n];assert hashlib.sha256(f.read_bytes()).hexdigest()==proof['source_pins'][n];return f.read_text()
 old=captured('src/ui/frontend_state.cpp');new=(a.root/'src/ui/frontend_state.cpp').read_text();captured('include/halo2/frontend.hpp')
 methods=['profile_login_key(','bind_primary_login(','update_online_login(','return_online(','return_online_lobby(']
 before=[definition(old,'bool online_screen(')]+[definition(old,'MenuState::'+n)for n in methods]
 after=[body(a.root/'src/ui/frontend_matchmaking_signin.cpp')]+[definition(new,'MenuState::'+n)for n in methods]
 for text,parts in [(old,before),(new,after)]:
  op=definition(text,'MenuState::open(');prefix=op[op.index('{')+1:op.index('  if (stack_.size() >= 64)')]
  parts.append('std::pair<unsigned,unsigned> MenuState::signin_prepare_open(unsigned screen,unsigned context){'+prefix+'return {screen,context};}')
 branch=definition(old,'  if (f.screen == 20) {')
 old_accept=definition(old,'MenuState::accept(');new_accept=definition(new,'MenuState::accept(')
 assert old_accept[:old_accept.index(branch)]==new_accept[:new_accept.index('  if(signin_flow_drive(4).handled)return;')]
 before.append('bool MenuState::signin_accept(){const auto&f=frame();'+branch.replace('return;','return true;')+'return false;}')
 assert new.count('if(signin_flow_drive(4).handled)return;')==1
 after.append('bool MenuState::signin_accept(){return signin_flow_drive(4).handled;}')
 back=definition(old,'  if (frame().screen == 20 && awaiting_discord_login_) {')
 old_back=definition(old,'MenuState::back(');new_back=definition(new,'MenuState::back(')
 assert old_back.replace(back,'  signin_flow_drive(5);')==new_back
 before.append('void MenuState::signin_back(){'+back+'}')
 after.append('void MenuState::signin_back(){signin_flow_drive(5);}')
 condition='id.size()==32 && id.find_first_not_of("0123456789abcdef")==std::string::npos'
 assert old.count(condition)==1 and 'if(file>>std::quoted(id))party_flow_.signin_load_key(name,id);'in new
 before.append('void MenuState::signin_load_key(const std::string&name,const std::string&id){if('+condition+')profile_login_keys_[name]=id;}')
 after.append('void MenuState::signin_load_key(const std::string&name,const std::string&id){party_flow_.signin_load_key(name,id);}')
 assert 'profile_login_keys_.erase(profile_name());'in old and 'party_flow_.signin_take_key(profile_name());'in new
 assert 'const auto saved_login_key=profile_login_keys_.extract(previous_name);'in old and 'const auto saved_login_key=party_flow_.signin_take_key(previous_name);'in new
 assert 'if(!saved_login_key.empty())profile_login_keys_[profile_name()]=saved_login_key.mapped();'in old and 'signin_restore_key(saved_login_key);'in new
 before.append('std::optional<std::string> MenuState::signin_take_key(const std::string&name){auto node=profile_login_keys_.extract(name);return node.empty()?std::nullopt:std::optional<std::string>(node.mapped());}')
 before.append('void MenuState::signin_restore_key(const std::optional<std::string>&key){if(key)profile_login_keys_[profile_name()]=*key;}')
 after.append('std::optional<std::string> MenuState::signin_take_key(const std::string&name){return party_flow_.signin_take_key(name);}')
 old_rows='for(const auto &[name,key]:profile_login_keys_)\n    file<<"login_key "<<std::quoted(name)<<\' \'<<std::quoted(key)<<\'\\n\';'
 new_rows=old_rows.replace('profile_login_keys_','party_flow_.signin_keys()')
 assert old_rows in old and new_rows in new
 for parts,rows in [(before,old_rows),(after,new_rows)]:parts.append('std::string MenuState::signin_saved_keys()const{std::ostringstream file;'+rows+'return file.str();}')
 a.before.parent.mkdir(parents=True,exist_ok=True);a.before.write_text('// Exact captured callers and the declared storage/dispatcher seams.\n'+'\n'.join(before)+'\n');a.after.parent.mkdir(parents=True,exist_ok=True);a.after.write_text('\n'.join(after)+'\n')
if __name__=='__main__':main()
