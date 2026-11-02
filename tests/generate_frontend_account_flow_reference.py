#!/usr/bin/env python3
"""Extract authenticated native account/continuation callers and exact new IO."""
import argparse,hashlib,json
from pathlib import Path

def definition(text,token):
 pos=text.index(token);start=text.rfind('\n',0,pos)+1;end=text.index('{',pos)+1;depth=1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--root',type=Path,required=True);p.add_argument('--before',type=Path,required=True);p.add_argument('--after',type=Path,required=True);args=p.parse_args()
 proof=json.loads((args.reference/'provenance.json').read_text());parts=[]
 for name,token in [('src/ui/frontend_clans.cpp','MenuState::update_online_account('),('src/ui/frontend_friends.cpp','MenuState::update_online_friend_join(')]:
  source=args.reference/proof['capture_names'][name];assert hashlib.sha256(source.read_bytes()).hexdigest()==proof['source_pins'][name];parts.append(definition(source.read_text(),token))
 args.before.parent.mkdir(parents=True,exist_ok=True);args.before.write_text('// Unchanged captured native callers\n'+'\n'.join(parts)+'\n')
 parts=[]
 for name,token in [('src/ui/frontend_clans.cpp','MenuState::update_online_account('),('src/ui/frontend_friends.cpp','MenuState::update_online_friend_join(')]:parts.append(definition((args.root/name).read_text(),token))
 source=args.root/'src/ui/frontend_matchmaking_account_flow.cpp';text='\n'.join(l for l in source.read_text().splitlines()if not l.startswith('#include'));assert text.count('namespace halo2 {')==1 and text.rstrip().endswith('}');text=text.replace('namespace halo2 {','',1).rstrip()[:-1]
 args.after.parent.mkdir(parents=True,exist_ok=True);args.after.write_text('// Exact IO SHA256 '+hashlib.sha256(source.read_bytes()).hexdigest()+'\n'+'\n'.join(parts)+'\n'+text+'\n')

if __name__=='__main__':main()
