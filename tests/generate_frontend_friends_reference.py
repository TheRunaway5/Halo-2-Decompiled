#!/usr/bin/env python3
"""Extract authenticated full native friends callers and exact new native IO."""
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
 proof=json.loads((a.reference/'provenance.json').read_text());name='src/ui/frontend_friends.cpp';old=a.reference/proof['capture_names'][name];assert hashlib.sha256(old.read_bytes()).hexdigest()==proof['source_pins'][name]
 a.before.parent.mkdir(parents=True,exist_ok=True);a.before.write_text('// Exact captured caller SHA256 '+proof['source_pins'][name]+'\n'+body(old))
 files=[a.root/name,a.root/'src/ui/frontend_matchmaking_friends.cpp'];a.after.parent.mkdir(parents=True,exist_ok=True);a.after.write_text('\n'.join('// Exact source SHA256 '+hashlib.sha256(p.read_bytes()).hexdigest()+'\n'+body(p)for p in files))
if __name__=='__main__':main()
