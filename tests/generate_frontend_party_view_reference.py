#!/usr/bin/env python3
"""Extract authenticated captured menu/text and current native IO definitions.

The comparator supplies controlled cached native facts and resource providers;
the copied policy and adapter definitions are not rewritten or abbreviated.
"""
import argparse
import hashlib
import json
from pathlib import Path


def definition(text,token):
    pos=text.index(token);start=text.rfind('\n',0,pos)+1;end=text.index('{',pos)+1;depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--reference',type=Path,required=True);p.add_argument('--root',type=Path,required=True)
    p.add_argument('--before',type=Path,required=True);p.add_argument('--after',type=Path,required=True);p.add_argument('--adapter',type=Path,required=True);args=p.parse_args()
    capture=args.reference/'frontend_party.cpp.reference';pins=json.loads((args.reference/'provenance.json').read_text())['source_pins']
    assert hashlib.sha256(capture.read_bytes()).hexdigest()==pins['src/ui/frontend_party.cpp']
    tokens=['MenuState::populate_online_party(','MenuState::online_party_text(']
    for path,source in [(args.before,capture),(args.after,args.root/'src/ui/frontend_party.cpp')]:
        text=source.read_text();path.parent.mkdir(parents=True,exist_ok=True)
        path.write_text('// Exact source SHA256 '+hashlib.sha256(source.read_bytes()).hexdigest()+'\n'+'\n'.join(definition(text,t)for t in tokens)+'\n')
    source=args.root/'src/ui/frontend_matchmaking_party_view.cpp';text=source.read_text()
    text='\n'.join(line for line in text.splitlines()if not line.startswith('#include'))
    assert text.count('namespace halo2 {')==1 and text.rstrip().endswith('}')
    text=text.replace('namespace halo2 {','',1).rstrip()[:-1]
    args.adapter.parent.mkdir(parents=True,exist_ok=True)
    args.adapter.write_text('// Exact native IO source SHA256 '+hashlib.sha256(source.read_bytes()).hexdigest()+'\n'+text+'\n')


if __name__=='__main__':main()
