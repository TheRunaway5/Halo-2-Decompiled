#!/usr/bin/env python3
"""Instrument disposable post-binding aliases without changing service admission."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import sys

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from generated_routine_ownership import function_bodies,literal_callbacks,masked,CANONICAL_ONLY_CALLBACK_SOURCES
from perf_4k60.census_common import callback_image_scope,EXPRESSION,HEX
from native_service_observations import native_service_observation

NON_CODE=re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',re.S)
CALL_RECEIVER=r'(?:[A-Za-z_]\w*(?:\(\))?\.)*instruction(?:_literal)?'
GUARD=re.compile(r'\bif\s*\(\s*(?P<predicate>s\.gp\[1\]\s*&&\s*)?(?P<receiver>'+CALL_RECEIVER+r')\s*\)\s*(?P<brace>\{\s*)?(?P=receiver)\s*\(')
LITERAL=re.compile(r'\b'+CALL_RECEIVER+r'\s*\(')
SERVICE=re.compile(r'\b(?:s\.)?services\.instruction\s*\(')
LITERAL_PC=re.compile(rf'(?:{EXPRESSION}|\(s\.image==SourcePhysicsContactImage::MapPack11\?{HEX}:{HEX}\))')
INCLUDE='#include "halo2/native_execution_accounting.hpp"\n'
SCOPE='halo2_execution_scope_8ef3'


def sha(value):return hashlib.sha256(value.encode() if isinstance(value,str) else value).hexdigest()


def close_parenthesis(mask,opening):
    depth=0
    for index in range(opening,len(mask)):
        if mask[index]=='(':depth+=1
        elif mask[index]==')':
            depth-=1
            if not depth:return index+1
    raise ValueError('Unclosed instruction callback argument')


def after_space(mask,index):
    while index<len(mask) and mask[index].isspace():index+=1
    return index


def instrument(text,canonical_text,canonical_name,*,embedded=False):
    if INCLUDE in text or SCOPE in text:raise ValueError('Execution accounting input is already instrumented')
    input_sha,canonical_sha=sha(text),sha(canonical_text)
    unit_key=sha(canonical_name+'\0'+canonical_sha+'\0'+input_sha)[:32]
    unit='::halo2::execution_accounting::unit_'+unit_key
    scope=callback_image_scope(canonical_text)
    edits=[];rows=[]
    canonical={match[1]:body for match,body in function_bodies(canonical_text)}
    for match,body in function_bodies(text):
        symbol=match[1]
        if symbol not in canonical:raise ValueError('Alias function is absent from canonical source: '+symbol)
        mask=NON_CODE.sub(lambda m:' '*len(m.group()),body)
        covered=[];sites=[];forwarders=[];predicates={}
        for guard in GUARD.finditer(mask):
            closed=close_parenthesis(mask,guard.end()-1)
            if not LITERAL_PC.fullmatch(mask[guard.end():closed-1].strip()):
                continue
            end=after_space(mask,closed)
            if end>=len(mask) or mask[end]!=';':raise ValueError('Instruction guard has no statement boundary')
            end+=1
            if guard['brace']:
                end=after_space(mask,end)
                if end>=len(mask) or mask[end]!='}':raise ValueError('Instruction guard owns additional statements')
                end+=1
            following=after_space(mask,end)
            if re.match(r'else\b',mask[following:]):raise ValueError('Instruction callback guard has an unsupported else clause')
            sites.append((guard.start(),end));covered.append((guard.start(),end))
            if guard['predicate']:predicates[guard.start()]='s.gp[1]'
        for callback in SERVICE.finditer(mask):
            if not any(a<=callback.start()<b for a,b in covered):
                closed=close_parenthesis(mask,callback.end()-1)
                if LITERAL_PC.fullmatch(mask[callback.end():closed-1].strip()):
                    raise ValueError(f'Unowned literal service instruction call in{canonical_name}:run_{symbol}')
                forwarders.append(body[callback.start():closed])
        for callback in LITERAL.finditer(mask):
            if any(a<=callback.start()<b for a,b in covered):continue
            closed=close_parenthesis(mask,callback.end()-1)
            if not LITERAL_PC.fullmatch(mask[callback.end():closed-1].strip()):
                forwarders.append(body[callback.start():closed]);continue
            end=after_space(mask,closed)
            if end>=len(mask) or mask[end]!=';':raise ValueError('Literal instruction call is not a statement')
            sites.append((callback.start(),end+1))
        sites.sort()
        if any(b>c for (a,b),(c,d) in zip(sites,sites[1:])):raise ValueError('Overlapping literal instrumentation sites')
        for start,end in sites:
            original=body[start:end]
            count=(('if('+predicates[start]+')') if start in predicates else '')+'::halo2::execution_accounting::record_literal(halo2_execution_counter_8ef3);'
            edits.append((match.start()+start,match.start()+end,'{'+count+original+'}'))
        image=('::halo2::execution_accounting::runtime_image(s)' if match.state_type else
               '1u' if scope=='tu15' else '0u' if scope=='map-pack11' else '2u')
        forwarders=sorted(set(forwarders))
        declaration=(f'\nstatic ::halo2::execution_accounting::Counter halo2_execution_counter_8ef3'
                     f'{{&{unit},"{symbol}",{len(sites)}u,{len(forwarders)}u}};\n'
                     f'::halo2::execution_accounting::Scope {SCOPE}'
                     '{halo2_execution_counter_8ef3,'+image+'};\n')
        edits.append((match.end(),match.end(),declaration))
        semantics=('native_service_observation_sites'if native_service_observation(canonical_name,canonical_sha,symbol)else
            'retained_generated_body_sites' if canonical_name.endswith('_generated.cpp') else
            'instruction_shape_candidate_requires_review' if len(sites)>=3 and re.search(r'\b\w+\.(?:gp\s*\[|reg\s*\(|set\s*\(|flags\.)|iterator_flags\(',mask) else
            'literal_observer_or_instruction_shape_unclassified')
        rows.append(dict(symbol=symbol,function_name=match.function_name,state_type=match[2],canonical_body_sha256=sha(canonical[symbol]),
                         input_body_sha256=sha(body),literal_sites=len(sites),
                         source_body_semantics=semantics,
                         dynamic_instruction_forwarders=forwarders))
    if not rows:raise ValueError('No generated function definitions to instrument')
    pieces=[];cursor=0
    for start,end,replacement in sorted(edits):
        if start<cursor:raise ValueError('Overlapping generated source edits')
        pieces.extend((text[cursor:start],replacement));cursor=end
    pieces.append(text[cursor:]);text=''.join(pieces)
    identity=(f'namespace halo2::execution_accounting {{ inline constexpr UnitIdentity unit_{unit_key}'
              '{'+','.join(json.dumps(value) for value in (canonical_name,canonical_sha,input_sha,scope))+'}; }\n')
    if not embedded:text=INCLUDE+identity+text
    manifest=dict(schema='native-execution-instrumentation-v1',canonical_source=canonical_name,
        canonical_sha256=canonical_sha,input_sha256=input_sha,output_sha256=sha(text),
        callback_image_scope=scope,functions=rows,function_count=len(rows),
        callback_address_identity=('canonical-only; production remapper unauthenticated'if canonical_name in CANONICAL_ONLY_CALLBACK_SOURCES else 'literal source observation namespace'),
        runtime_state_image_proves_actual_pc_mapping=False,
        source_filename_scope='generated' if canonical_name.endswith('_generated.cpp') else 'nongenerated',
        literal_sites=sum(row['literal_sites'] for row in rows),
        function_entry_scope_before_native_prefix=True,service_callbacks_replaced=False,
        native_admission_state_modified=False,canonical_source_modified=False,
        counter_semantics='literal callback/statement sites, including repeated-string observations; nongenerated observer boundaries require semantic review',
        native_closure_proved=False)
    return text,manifest


def unit_declaration(canonical_name,canonical_text,input_text):
    key=sha(canonical_name+'\0'+sha(canonical_text)+'\0'+sha(input_text))[:32]
    values=(canonical_name,sha(canonical_text),sha(input_text),callback_image_scope(canonical_text))
    return 'namespace halo2::execution_accounting { inline constexpr UnitIdentity unit_'+key+'{'+','.join(json.dumps(value)for value in values)+'}; }\n'


QUOTED_INCLUDE=re.compile(r'^[ \t]*#[ \t]*include[ \t]+"([^"\n]+)"[^\n]*',re.M)


def instrument_translation(text,canonical_text,canonical_name,root):
    """Inline accounted local headers at their original inclusion boundary.

    Header functions retain their own canonical source/hash identity. Global
    identity declarations precede all original namespaces; lambda captures and
    service callbacks are untouched. Only the disposable CPP is rewritten.
    """
    root=Path(root).resolve();dependencies=set();embedded={};declarations={};active=set()
    def includes(value,source):
        mask=masked(value);edits=[]
        for match in QUOTED_INCLUDE.finditer(value):
            if not mask[match.start():match.end()].lstrip().startswith('#'):continue
            path=next((p.resolve()for p in (source.parent/match[1],root/'src'/match[1],root/'include'/match[1])if p.is_file()),None)
            if path is None or not path.is_relative_to(root/'src'):continue
            dependencies.add(path)
            if path in active:continue
            active.add(path);original=path.read_text();rewritten,children=includes(original,path);active.remove(path)
            relative=str(path.relative_to(root));own=any(literal_callbacks(masked(original)))
            if own:
                rewritten,receipt=instrument(original,original,relative,embedded=True)
                rewritten,_=includes(rewritten,path)
                declarations[relative]=unit_declaration(relative,original,original)
                receipt['embedded_in']=canonical_name;embedded[relative]=receipt
            if not own and not children:continue
            # Preserve pragma-once semantics when a header is embedded into the
            # containing CPP, including repeated includes from another header.
            if re.search(r'^\s*#\s*pragma\s+once\s*$',rewritten,re.M):
                guard='HALO2_ACCOUNTED_HEADER_'+sha(relative)[:24].upper()
                rewritten=re.sub(r'^\s*#\s*pragma\s+once\s*$',f'#ifndef {guard}\n#define {guard}',rewritten,count=1,flags=re.M)+'\n#endif\n'
            if own:embedded[relative]['output_sha256']=sha(rewritten)
            marker=sha(relative)[:24]
            replacement=f'// HALO2_ACCOUNTED_HEADER_BEGIN {marker}\n'+rewritten+f'\n// HALO2_ACCOUNTED_HEADER_END {marker}'
            edits.append((match.start(),match.end(),replacement))
        for start,end,replacement in reversed(edits):value=value[:start]+replacement+value[end:]
        return value,bool(edits)
    generated,manifest=instrument(text,canonical_text,canonical_name)
    generated,_=includes(generated,root/canonical_name)
    if declarations:generated=INCLUDE+''.join(declarations[key]for key in sorted(declarations))+generated
    manifest['embedded_units']=[embedded[key]for key in sorted(embedded)]
    manifest['output_sha256']=sha(generated)
    manifest['include_dependencies']=[str(path.relative_to(root))for path in sorted(dependencies)]
    return generated,manifest,dependencies


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--canonical',type=Path,required=True)
    parser.add_argument('--root',type=Path,default=ROOT)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--manifest',type=Path,required=True)
    parser.add_argument('--depfile',type=Path)
    args=parser.parse_args()
    source,canonical,output=args.source.resolve(),args.canonical.resolve(),args.output.resolve()
    if output in (source,canonical):parser.error('Instrument a separate disposable alias, never canonical/input source')
    owner=os.environ.get('HALO2_TEST_SCRATCH')
    if not owner or not output.is_relative_to(Path(owner).resolve()) or not args.manifest.resolve().is_relative_to(Path(owner).resolve()):
        parser.error('Instrumentation outputs require the enclosing HALO2_TEST_SCRATCH owner')
    relative=str(canonical.relative_to(args.root.resolve()))
    generated,manifest,dependencies=instrument_translation(source.read_text(),canonical.read_text(),relative,args.root)
    manifest.update(generator='tools/instrument_native_execution.py',generator_sha256=sha(Path(__file__).read_bytes()))
    output.parent.mkdir(parents=True,exist_ok=True);output.write_text(generated)
    args.manifest.parent.mkdir(parents=True,exist_ok=True)
    encoded=json.dumps(manifest,indent=2,sort_keys=True)+'\n'
    if len(encoded.encode())>1_048_576:raise ValueError('Instrumentation manifest exceeds bounded report size')
    args.manifest.write_text(encoded)
    if args.depfile:
        depfile=args.depfile.resolve()
        if not depfile.is_relative_to(Path(owner).resolve()):parser.error('Depfile requires the enclosing scratch owner')
        escape=lambda path:str(path).replace('\\','\\\\').replace(' ','\\ ').replace('#','\\#').replace('$','$$')
        depfile.parent.mkdir(parents=True,exist_ok=True)
        depfile.write_text(escape(output)+': '+' '.join(escape(path)for path in sorted(dependencies|{source,canonical}))+'\n')
    print(json.dumps({key:manifest[key] for key in ('canonical_source','function_count','literal_sites','output_sha256')}))
    return 0


if __name__=='__main__':raise SystemExit(main())
