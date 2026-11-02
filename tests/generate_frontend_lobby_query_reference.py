#!/usr/bin/env python3
"""Extract the authenticated whole native lobby caller and actual Rust IO adapter."""
import argparse,hashlib,json
from pathlib import Path

def definition(text,token):
 pos=text.index(token);start=text.rfind('\n',0,pos)+1;end=text.index('{',pos)+1;depth=1
 while depth:
  depth+=(text[end]=='{')-(text[end]=='}');end+=1
 return text[start:end]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--root',type=Path,required=True);p.add_argument('--before',type=Path,required=True);p.add_argument('--after',type=Path,required=True);args=p.parse_args()
 source=args.reference/'frontend_state.cpp.reference';pins=json.loads((args.reference/'provenance.json').read_text())['source_pins'];assert hashlib.sha256(source.read_bytes()).hexdigest()==pins['src/ui/frontend_state.cpp']
 text=source.read_text();helpers=[]
 for name,token in [('frontend_lobby_team.hpp','frontend_lobby_team_count('),('frontend_lobby_ready.hpp','frontend_lobby_ready_status(')]:
  helper=args.reference/(name+'.reference');assert hashlib.sha256(helper.read_bytes()).hexdigest()==pins['include/halo2/'+name]
  h=helper.read_text()
  if name=='frontend_lobby_ready.hpp':helpers.append(definition(h,'struct FrontendLobbyReadyInput')+';')
  helpers.append(definition(h,token))
 args.before.parent.mkdir(parents=True,exist_ok=True);args.before.write_text('// Exact captured source SHA256 '+hashlib.sha256(source.read_bytes()).hexdigest()+'\n'+'\n'.join(helpers)+'\n'+definition(text,'MenuState::lobby_view()')+'\n')
 source=args.root/'src/ui/frontend_matchmaking_lobby_query.cpp';text='\n'.join(l for l in source.read_text().splitlines()if not l.startswith('#include'));assert text.count('namespace halo2 {')==1 and text.rstrip().endswith('}');text=text.replace('namespace halo2 {','',1).rstrip()[:-1]
 getter=definition((args.root/'src/ui/frontend_state.cpp').read_text(),'MenuState::lobby_query_campaign_file(')
 args.after.parent.mkdir(parents=True,exist_ok=True);args.after.write_text('// Exact native IO source SHA256 '+hashlib.sha256(source.read_bytes()).hexdigest()+'\n'+getter+'\n'+text+'\n')

if __name__=='__main__':main()
