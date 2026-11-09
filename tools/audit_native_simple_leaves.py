#!/usr/bin/env python3
"""Bounded candidate audit of exact linear constant/field-return owner bodies.

This does not authenticate executable identity, production reachability or a
native implementation. Bare RET transport is counted separately and excluded
from migration candidates. Unknown source shapes remain unclassified.
"""
import argparse
import collections
import hashlib
import json
import os
from pathlib import Path
import re
from generated_routine_ownership import function_bodies,literal_source_paths,qualified_dispatch_entries
ROOT=Path(__file__).resolve().parents[1]
NUMBER=r'(?:0x[0-9a-f]+|[0-9]+)u'
CALLBACK=re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\([^;{}]*\);\}|if\(s\.services\.instruction\)s\.services\.instruction\([^;{}]*\);|s\.instruction(?:_literal)?\([^;{}]*\);')
RET=re.compile(r'(?P<operations>.*)s\.pop\(\);(?:s\.gp\[4\]\+=(?P<cleanup>'+NUMBER+r');)?return;')
def classify(body):
    value=body[body.index('{')+1:-1]
    value=re.sub(r'throw std::runtime_error\("Original [^"\n]*fell outside authenticated CFG"\);\s*$','',value)
    value=CALLBACK.sub('',value);value=re.sub(r'\s+','',value)
    match=RET.fullmatch(value)
    if not match:return None
    operations=match['operations'];result={'cleanup':int((match['cleanup']or'0u')[:-1],0)}
    if not operations:return dict(result,kind='bare_return_abi_only_excluded')
    constant=re.fullmatch(r's\.set\(0,('+NUMBER+r'),([124]),0\);',operations)
    if constant:return dict(result,kind='constant',value=int(constant[1][:-1],0),width=int(constant[2]))
    field=re.fullmatch(r's\.set\(0,(?P<signed>sign_extend\()?r\(s\.memory,\((?P<address>(?:'+NUMBER+r'\+)?s\.gp\[[0-7]\])\),(?P<read>[124])\)(?:,(?P<extend>[12])\))?,(?P<write>[124]),0\);',operations)
    if field:
        if bool(field['signed'])!=bool(field['extend']):return None
        if field['extend']and field['extend']!=field['read']:return None
        return dict(result,kind='signed_field'if field['signed']else'field',address=field['address'],read_width=int(field['read']),write_width=int(field['write']))
    if operations=='s.set(0,s.flags.logic((s.reg(0,4,0))^(s.reg(0,4,0)),4),4,0);':return dict(result,kind='zero_with_logic_flags',value=0,width=4)
    if operations=='s.set(0,s.flags.logic((s.reg(0,4,0))|(0xffffffffu),4),4,0);':return dict(result,kind='minus_one_with_logic_flags',value=0xffffffff,width=4)
    return None
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    rows=[];bare=[];files={};kinds=collections.Counter()
    for path in literal_source_paths(ROOT):
        text=path.read_text();pin=hashlib.sha256(text.encode()).hexdigest();owners=list(function_bodies(text));candidates=[]
        for signature,body in owners:
            classification=classify(body)
            if classification is not None:candidates.append((signature,body,classification))
        if not candidates:continue
        name=str(path.relative_to(ROOT));files[name]=pin
        maps={image:qualified_dispatch_entries(text,image)for image in('tu15','map-pack11')}
        for signature,body,data in candidates:
            identity={'source':name,'source_sha256':pin,'symbol':signature[1],'state_type':signature.state_type,'body_sha256':hashlib.sha256(body.encode()).hexdigest()}
            identity.update(data)
            if data['kind']=='bare_return_abi_only_excluded':bare.append(identity);continue
            identity['internal_dispatch_candidates']={image:values[signature[1]]for image,values in maps.items()if signature[1]in values}
            identity['image_and_executable_identity_qualified']=False;identity['native_implementation_qualified']=False
            rows.append(identity);kinds[data['kind']]+=1
    if any(hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=pin for name,pin in files.items()):raise ValueError('Simple-leaf inputs changed during audit')
    report={'schema':'native-simple-leaf-candidates-v1','candidate_count':len(rows),'kinds':dict(kinds),'candidates':rows,'bare_return_abi_only_excluded':len(bare),
        'source_sha256':files,'tool_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),'scope':'Exact full linear body shapes among recognized literal-instruction owners; other shapes, dynamic site-only adapters and unentered production variants remain open.',
        'production_reachability_qualified':False,'native_closure_proved':False,'retained_large_artifacts':[]}
    raw=json.dumps(report,indent=2,sort_keys=True)+'\n'
    if len(raw.encode())>1048576:raise ValueError('Simple-leaf audit exceeds bounded report budget')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(raw)
    print(json.dumps({'candidates':len(rows),'kinds':dict(kinds),'bare_returns_excluded':len(bare),'report':str(args.report)}));return 0
if __name__=='__main__':raise SystemExit(main())
