#!/usr/bin/env python3
"""Find literal instruction/state bookkeeping outside generated filenames.

This is a source candidate audit, not a reachability or native qualification.
Unrecognized C++ ownership is retained explicitly instead of silently omitted.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
from generated_routine_ownership import masked,BRACES

CALL=re.compile(r'\b(?:instruction|[A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*\.instruction(?:_literal)?)\s*\(\s*(?P<pc>0x[0-9a-fA-F]+)[uU]?\b')
FUNCTION=re.compile(r'^[ \t]*(?:\[\[[^\]]+\]\][ \t]*)?(?:(?:inline|static|constexpr)\s+)*(?:[\w:<>]+(?:\s*[&*])?\s+)+(?P<name>[\w:]+)\s*\([^;{}]*\)\s*\{',re.M)
GP=re.compile(r'\b\w+\.gp\s*\[|\b\w+\.(?:reg|set)\s*\(')
FLAGS=re.compile(r'\b(?:iterator_(?:flags|logic_flags)|state_product)\s*\(|\b\w+\.flags\.(?:logic|minus|plus|shift|increment)\s*\(')


def candidates(text):
    code=masked(text);calls=list(CALL.finditer(code));owned=set();rows=[];last_end=0
    for signature in FUNCTION.finditer(code):
        if signature.start()<last_end:continue
        if signature['name'] in ('if','for','while','switch','catch'):continue
        depth=1;end=None
        for brace in BRACES.finditer(code,signature.end()):
            depth+=1 if brace[0]=='{' else -1
            if not depth:end=brace.end();break
        if end is None:continue
        last_end=end;body=code[signature.start():end]
        events=[(index,call) for index,call in enumerate(calls) if signature.start()<=call.start()<end]
        if not events:continue
        owned.update(index for index,_ in events)
        sites=sorted({call['pc'].lower() for _,call in events})
        rows.append(dict(function=signature['name'],line=text.count('\n',0,signature.start())+1,
            body_sha256=hashlib.sha256(text[signature.start():end].encode()).hexdigest(),
            literal_instruction_calls=len(events),literal_sites=sites[:64],literal_site_count=len(sites),
            literal_sites_complete=len(sites)<=64,literal_sites_sha256=hashlib.sha256(json.dumps(sites,separators=(',',':')).encode()).hexdigest(),
            gp_operations=len(GP.findall(body)),flag_operations=len(FLAGS.findall(body)),
            goto_count=len(re.findall(r'\bgoto\b',body)),actual_image_identity_qualified=False,
            instruction_shape_review_required=len(events)>=3 and bool(GP.search(body) or FLAGS.search(body))))
    unowned=[dict(line=text.count('\n',0,call.start())+1,pc=call['pc'].lower()) for i,call in enumerate(calls) if i not in owned]
    return rows,unowned,len(calls)


def audit(root):
    result=subprocess.run(['rg','--files','src','-g','*.cpp','-g','*.hpp','-g','*.h','-g','*.c','-g','!*generated*'],
        cwd=root,text=True,capture_output=True,check=True)
    files=[];scanned=0
    for relative in sorted(result.stdout.splitlines()):
        path=root/relative;data=path.read_bytes();text=data.decode();scanned+=1
        rows,unowned,count=candidates(text)
        if not count:continue
        files.append(dict(source=relative,source_sha256=hashlib.sha256(data).hexdigest(),bytes=len(data),
            literal_instruction_calls=count,functions=rows,unattributed_literal_calls=unowned[:64],unattributed_literal_call_count=len(unowned),
            unattributed_literal_calls_sha256=hashlib.sha256(json.dumps(unowned,separators=(',',':')).encode()).hexdigest(),
            static_production_reachability_qualified=False))
    return dict(schema='nongenerated-instruction-shape-audit-v1',source_candidates_recorded=True,
        scope='Source files without generated in their filename; some contain generated code.',
        native_closure_proved=False,scanned_files=scanned,candidate_files=len(files),
        functions_with_literal_callbacks=sum(len(f['functions']) for f in files),
        instruction_shape_review_candidates=sum(r['instruction_shape_review_required'] for f in files for r in f['functions']),
        unattributed_literal_calls=sum(f['unattributed_literal_call_count'] for f in files),files=files,
        limitations=['Literal callbacks plus GP/flag bookkeeping are source review candidates, not a complete C++ semantic classifier.',
            'First/minimum literal PCs are not assumed to be actual function identities.',
            'Production routing, dynamic callback sites, generated units and independent ABI/parity proofs remain separate.'],
        retained_large_artifacts=[])


def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    report=audit(Path(__file__).resolve().parents[1]);report['tool_sha256']=hashlib.sha256(Path(__file__).read_bytes()).hexdigest()
    encoded=json.dumps(report,indent=2)+'\n'
    if len(encoded.encode())>900_000:raise ValueError('Instruction-shape report exceeds bounded metadata budget')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(encoded)
    print(json.dumps({key:report[key] for key in ('candidate_files','functions_with_literal_callbacks','instruction_shape_review_candidates','unattributed_literal_calls')}))


if __name__=='__main__':main()
