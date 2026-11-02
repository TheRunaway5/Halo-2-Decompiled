#!/usr/bin/env python3
"""Compile captured native policy with observation-only lookup probes.

The four inserted comma expressions record the existing cache/value/default
reads, and may throw a controlled native exception before that read. They do
not choose records, defaults, row order or output. The production wrapper is
also compared with the uninstrumented native cache/default provider.
"""
import argparse
import hashlib
import json
from pathlib import Path


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args()
    pins=json.loads((args.reference/'provenance.json').read_text())['source_pins']
    sources=[]
    for name in ('src/ui/frontend_lobby.cpp','include/halo2/frontend_lobby_game.hpp'):
        p=args.reference/(Path(name).name+'.reference')
        assert hashlib.sha256(p.read_bytes()).hexdigest()==pins[name]
        s='\n'.join(line for line in p.read_text().splitlines()
                    if not line.startswith('#include') and line!='#pragma once')
        assert s.count('namespace halo2 {')==1
        sources.append(s.replace('namespace halo2 {','namespace halo2::lobby_view_before {',1))
    replacements={
        'content.categories.find(*category)': '(observed->read("category:"+std::to_string(*category)), content.categories.find(*category))',
        'content.parameters.find(found->second[row])': '(observed->read("parameter:"+std::to_string(found->second[row])), content.parameters.find(found->second[row]))',
        'values.find(id)': '(observed->read("value:"+std::to_string(id)), values.find(id))',
        'return source_game_variant_get(source_game_variant_default(engine), int(parameter));':
            'return (observed->read("default:"+std::to_string(engine)+":"+std::to_string(parameter)), observed->default_value(engine,parameter));',
    }
    for old,new in replacements.items():
        assert sources[1].count(old)==1,old
        sources[1]=sources[1].replace(old,new)
    # The captured convenience wrappers live in a nested test namespace;
    # qualify their template calls to exclude ADL into production halo2.
    for count in (5,6):
        old=f'return frontend_variant_quick_options<{count}>('
        assert sources[1].count(old)==1
        sources[1]=sources[1].replace(old,f'return halo2::lobby_view_before::frontend_variant_quick_options<{count}>(')
    args.output.parent.mkdir(parents=True,exist_ok=True)
    args.output.write_text('// Authenticated captured native policy; four observation probes.\n'+'\n'.join(sources)+'\n')


if __name__=='__main__':main()
