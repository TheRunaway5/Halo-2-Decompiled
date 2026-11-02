#!/usr/bin/env python3
"""Extract the whole authenticated modern clan caller and exact native Rust IO.

Only the clan-page branch of auxiliary() is wrapped as a handled predicate.
The surrounding PCR/lobby/login dispatcher is verified in place, not ported.
"""
import argparse,hashlib,json,re
from pathlib import Path
def body(path):
 text='\n'.join(line for line in path.read_text().splitlines()if not line.startswith('#include'))
 assert text.count('namespace halo2 {')==1
 text=text.replace('namespace halo2 {','',1)
 text,count=re.subn(r'\n}\s*(?:// namespace halo2)?\s*\Z','\n',text);assert count==1
 return text
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--root',type=Path,required=True);p.add_argument('--before',type=Path,required=True);p.add_argument('--after',type=Path,required=True);a=p.parse_args()
 proof=json.loads((a.reference/'provenance.json').read_text())
 def captured(name):
  path=a.reference/proof['capture_names'][name];assert hashlib.sha256(path.read_bytes()).hexdigest()==proof['source_pins'][name];return path
 old=captured('src/ui/frontend_clans.cpp');state=captured('src/ui/frontend_state.cpp').read_text()
 old_branch='if (frame().screen == 211 && frame().context == 41) { ++clan_message_page_; ++revision; return; }'
 new_branch='if (clan_message_advance()) return;'
 assert state.count(old_branch)==1
 current=(a.root/'src/ui/frontend_state.cpp').read_text();assert current.count(new_branch)==1 and old_branch not in current
 # Preserve the dispatch prefix and next statement exactly around this branch.
 start=state.index('void MenuState::auxiliary()');end=state.index('  update_online_login();',start)
 expected=state[start:end].replace(old_branch,new_branch)
 actual_start=current.index('void MenuState::auxiliary()');actual_end=current.index('  update_online_login();',actual_start)
 assert current[actual_start:actual_end]==expected
 branch=old_branch.replace('return;', 'return true;')
 a.before.parent.mkdir(parents=True,exist_ok=True);a.before.write_text('// Exact whole caller SHA256 '+proof['source_pins']['src/ui/frontend_clans.cpp']+'\n'+body(old)+'\n// Exact auxiliary clan branch; void return mapped to handled=true.\nbool MenuState::clan_message_advance(){'+branch+' return false;}\n')
 files=[a.root/'src/ui/frontend_clans.cpp',a.root/'src/ui/frontend_matchmaking_clans.cpp'];a.after.parent.mkdir(parents=True,exist_ok=True);a.after.write_text('\n'.join('// Exact source SHA256 '+hashlib.sha256(f.read_bytes()).hexdigest()+'\n'+body(f)for f in files))
if __name__=='__main__':main()
