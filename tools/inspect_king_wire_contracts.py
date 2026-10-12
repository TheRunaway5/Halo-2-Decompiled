#!/usr/bin/env python3
"""Static complete original King wire/builder contracts; no codec executions.

Derive helper entries from actual mapped direct calls in the immutable
replication inspection. Never supply an original service return or native data.
"""
from pathlib import Path
import argparse, csv, hashlib, json, re
from oracle_game_over import SPECS
from oracle_mp_full_scoreboard_1_5 import _Code, literal, complete_tokens, references
from research_mp_lobby_later_update import table_pair
from inspect_render_primitives_title_updates import prefix, section
from xbe import Xbe, FUNCTIONS_CSV
from inventory import read_rows

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT/'analysis/multiplayer/game-variants/king-replication-source-20261012/complete/report.json'
BASE_PIN = '44c4599ff00ed27dc3124555d98ca3e6c9f4a10565c9dfaf03fa9eb2fa1d6e2d'
OUT = ROOT/'analysis/multiplayer/game-variants/king-wire-source-20261012'
INITIAL = ['king_wire_size', 'king_wire_write', 'king_wire_read', 'shared_write', 'shared_read']
EDGES = [
    ('shared_size', 'king_wire_size', 0xa4950),
    ('field_begin', 'king_wire_write', 0xb56e0),
    ('field_end', 'king_wire_write', 0xb5760),
    ('write_bits', 'king_wire_write', 0x195720),
    ('read_bit', 'king_wire_read', 0x1957d0),
    ('read_bits', 'king_wire_read', 0x1959c0),
    ('checked_value_diagnostic', 'king_wire_write', 0xb66f0),
    ('checkpoint_pop', 'field_end', 0x194710),
]

def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def target(i): return int(i.op_str, 16) if i.mnemonic == 'call' and i.op_str.startswith('0x') else None

def main():
    ap=argparse.ArgumentParser(description=__doc__); ap.add_argument('--output',type=Path,default=OUT)
    a=ap.parse_args(); a.output.mkdir(parents=True,exist_ok=False)
    assert sha(BASE)==BASE_PIN
    base=json.loads(BASE.read_text()); images={}; codes={}; starts={}; entries={}; owners={}; edges=[]
    for name,spec in SPECS.items():
        p=Path(spec['path']); assert sha(p)==spec['pin']; im=Xbe(p);images[name]=im
        starts[name]={int(row['address'],16) for row in csv.DictReader((ROOT/spec['tsv']).open(),delimiter='\t')}
        codes[name]={s.name:_Code(im,s.name) for s in im.sections if s.name in ('.text','D3D')}
        entries[name]={key:base['entries'][name][key] for key in INITIAL}
        for key,entry in entries[name].items(): owners[name,key]=literal(codes[name][section(im,entry).name],entry,starts[name])
    for key,parent,old in EDGES:
        oi=owners['retail',parent][1]
        indexes=[j for j,i in enumerate(oi) if target(i)==old];assert indexes,(key,parent,old)
        targets={}
        for name,im in images.items():
            ins=owners[name,parent][1]
            assert complete_tokens(oi,images['retail'])==complete_tokens(ins,im),(name,parent)
            actual={target(ins[j]) for j in indexes};assert len(actual)==1 and None not in actual,(key,name,actual)
            entry=actual.pop();entries[name][key]=entry;targets[name]=entry
            owners[name,key]=literal(codes[name][section(im,entry).name],entry,starts[name])
        edges.append({'owner':key,'parent':parent,'parent_instruction_indexes':indexes,'actual_targets':targets})
    # Overflow query is an independently located context primitive, not a
    # claimed transitive King decoder call. King read does not invoke it.
    indexed=read_rows(FUNCTIONS_CSV); old=images['retail']; entry=0x1946f0
    entries['retail']['stream_overflowed']=entry
    owners['retail','stream_overflowed']=literal(codes['retail']['.text'],entry,starts['retail'])
    pat=prefix(old,entry,int(indexed[entry]['size'])); locators=[]
    oi=owners['retail','stream_overflowed'][1]
    for name in ('tu11','u5'):
        im=images[name];accepted=[];candidates=[]
        for s in im.sections:
            if s.name not in codes[name]:continue
            for match in re.finditer(pat,im.section_bytes(s),re.S):
                candidate=s.va+match.start()
                if candidate not in starts[name]:continue
                candidates.append(candidate);f,ins=literal(codes[name][s.name],candidate,starts[name])
                if complete_tokens(oi,old)==complete_tokens(ins,im):accepted.append((candidate,f,ins))
        assert len(accepted)==1,(name,candidates,[v[0] for v in accepted])
        candidate,f,ins=accepted[0];entries[name]['stream_overflowed']=candidate;owners[name,'stream_overflowed']=(f,ins)
        locators.append({'owner':'stream_overflowed','image':name,'candidates':candidates,'accepted':candidate})
    listings=[];comparisons=[]
    for name,im in images.items():
        for key,entry in entries[name].items():
            f,ins=owners[name,key];p=a.output/f'{name}-{key}-{entry:08x}.asm'
            p.write_text(''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in ins))
            listings.append({'image':name,'owner':key,'entry':entry,'instructions':len(ins),'file':p.name,'sha256':sha(p),
                'direct_calls':[{'site':i.address,'target':target(i)} for i in ins if target(i) is not None]})
            if name=='retail':continue
            of,oi=owners['retail',key]
            assert complete_tokens(oi,old)==complete_tokens(ins,im),(name,key)
            topology=table_pair(of,f,oi,ins,old,im);assert topology is not None and all(v['topology_equal'] for v in topology),(name,key)
            refs=references(oi,ins,old,im)
            assert all(v['old']['bytes_hex']==v['later']['bytes_hex'] for v in refs if v['old']['section']=='.rdata'),(name,key)
            mapped=dict(zip(entries['retail'].values(),entries[name].values()));constraints=[]
            for x,y in zip(oi,ins):
                t=target(x)
                if t in mapped:
                    assert target(y)==mapped[t]
                    constraints.append({'old_site':x.address,'later_site':y.address,'old_target':t,'later_target':target(y)})
            comparisons.append({'owner':key,'image':name,'complete_tokens_equal':True,'switch_topology':topology,
                'relocated_operands':refs,'direct_call_constraints':constraints})
    deps=[Path(__file__),ROOT/'tools/oracle_game_over.py',ROOT/'tools/oracle_mp_full_scoreboard_1_5.py',
          ROOT/'tools/research_mp_lobby_later_update.py',ROOT/'tools/inspect_render_primitives_title_updates.py',BASE]
    report={'schema':'king-wire-static-source-v1','scope':__doc__,'images':base['images'],'entries':entries,
        'derived_call_edges':edges,'overflow_context_locators':locators,'listings':listings,'comparisons':comparisons,
        'instruction_patches':[],'supplied_returns':[],'codec_executions':0,
        'dependencies':{str(p.relative_to(ROOT)):sha(p) for p in deps}}
    p=a.output/'report.json';p.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps({'owners':len(entries['retail']),'listings':len(listings),'pairs':len(comparisons),'entries':entries,'sha256':sha(p)}))

if __name__=='__main__': main()
