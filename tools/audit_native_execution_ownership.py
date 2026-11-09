#!/usr/bin/env python3
"""Keep frozen execution counts intact while qualifying their source ownership."""
import argparse,collections,hashlib,importlib,json,re
from pathlib import Path
from generated_routine_ownership import FUNCTION,BRACES,masked,function_bodies,qualified_dispatch_entries
from instrument_native_execution import instrument,close_parenthesis,LITERAL,NON_CODE,LITERAL_PC

ROOT=Path(__file__).resolve().parents[1]
EFFECT='src/gameplay/weapons/native_weapon_effect_dispatch_tu15.cpp'
EFFECT_PIN='5395464ef2b95c78e1a164b9970b85067d184d692da05804b440c2cf69089e4e'
WORLD='src/physics/physics_live_world_tu15.cpp'
WORLD_PIN='052dd455f52386bbbcd50b2c3cb77d9e40ad970ca6b7e937d47d67ad5562ab61'
OVERLAYS=(('1262b0','native_animation_resource_wait_replacements'),('1e68e0','native_bsp_segment_setup_replacements'),('13fe50','native_page_allocate_replacements'))
def sha(value):return hashlib.sha256(value.encode()if isinstance(value,str)else value).hexdigest()
def all_functions(text):
    code=masked(text);end=0;result={}
    for match in FUNCTION.finditer(code):
        if match.start()<end or match['name']in{'if','for','while','switch','catch'}:continue
        depth=1
        for brace in BRACES.finditer(code,match.end()):
            depth+=1 if brace[0]=='{'else -1
            if not depth:
                end=brace.end();result.setdefault(match['name'],[]).append(text[match.start():end]);break
    return result
def unique_function(text,name):
    found=all_functions(text).get(name,[])
    if len(found)!=1:raise ValueError('Expected one complete ownership function '+name)
    return found[0]
def selector(body,prefix=''):
    """Authenticate the entire simple literal-selector control-flow grammar."""
    code=body[body.index('{')+1:-1].strip()
    if prefix:
        if not code.startswith(prefix):raise ValueError('Selector initialization drift')
        code=code[len(prefix):]
    branch=re.compile(r'\s*(if|else\s+if)\(entry==(0x[0-9a-f]+u)\)(\w+)\(([^;{}]*)\);')
    result={};position=0
    while match:=branch.match(code,position):
        if match[1]!=('if'if not result else'else if'):raise ValueError('Export selector is not one exclusive chain')
        entry=int(match[2][:-1],16)
        if entry in result:raise ValueError('Duplicate actual export entry')
        result[entry]=match[3];position=match.end()
    if not result or not re.fullmatch(r'\s*else throw std::invalid_argument\("[^"\n]*"\);\s*',code[position:]):raise ValueError('Unsupported actual export selector')
    return result
def effect_exports(text):
    if sha(text)!=EFFECT_PIN:raise ValueError('Fixed Effect image-owner source pin changed')
    if 'SourcePhysicsContactImage' in masked(text):raise ValueError('Effect image selection requires review')
    top=unique_function(text,'native_weapon_effect_dispatch_tu15_run')
    integer=unique_function(text,'native_weapon_effect_integer_tu15_run')
    mapping=selector(top);integers=selector(integer,'IteratorRegisters a{gp,eflags};')
    for entry,name in list(mapping.items()):
        if name=='native_weapon_effect_integer_tu15_run':mapping[entry]=integers[entry]
        elif name=='native_weapon_effect_iterator_tu15_run':
            wrapper=unique_function(text,name)
            if not re.fullmatch(r'\s*IteratorRegisters a\{gp,eflags\};next_effect_location\(m,a,o,s\);\s*',wrapper[wrapper.index('{')+1:-1]):raise ValueError('Effect iterator facade drift')
            mapping[entry]='next_effect_location'
        elif name=='native_weapon_effect_datum_allocate_tu15_run':
            wrapper=unique_function(text,name)
            if not re.fullmatch(r'\s*IteratorRegisters a\{gp,eflags\};allocate_effect_datum\(m,a,o,s\);\s*',wrapper[wrapper.index('{')+1:-1]):raise ValueError('Effect datum facade drift')
            mapping[entry]='allocate_effect_datum'
    by_symbol={}
    for entry,name in mapping.items():by_symbol.setdefault(name,[]).append(f'{entry:x}')
    return by_symbol,{'source':EFFECT,'sha256':EFFECT_PIN,'dispatcher_body_sha256':sha(top),'integer_facade_body_sha256':sha(integer),
        'basis':'Complete fixed actual-TU1.5 export selector and integer/iterator/datum facades; no image argument or alternate image selector. Private header consumers are separately checked.'}
def historical_source(current,wanted):
    if sha(current)==wanted:return current,[]
    text=current;reversed_overlays=[]
    for entry,module_name in OVERLAYS:
        module=importlib.import_module(module_name)
        body=re.search(r'static void run_'+entry+r'\(WorldState &s\) \{.*?\n\}',text,re.S)
        if not body:continue
        canonical=module.canonical_body(body[0])
        if canonical==body[0]:continue
        if text.count(module.INCLUDE)!=1:raise ValueError('Overlay include ownership changed '+module_name)
        text=text[:body.start()]+canonical+text[body.end():];text=text.replace(module.INCLUDE,'',1);reversed_overlays.append(module_name)
    return (text,reversed_overlays)if sha(text)==wanted else(None,[])
def forwarder_kind(body,forwarders):
    if not forwarders:return 'no_dynamic_instruction_forwarder'
    code=masked(body)
    # Source-qualified literals remain with this owning function. A nested
    # lambda forwarding its unchanged parameter is not a new callee identity.
    lambdas=[]
    for match in re.finditer(r'\b(?:const\s+)?auto\s+(\w+)\s*=\s*\[[^]]*\]\s*\((?:auto|std::uint32_t)\s+(\w+)\)\s*\{',code):
        depth=1
        for brace in BRACES.finditer(code,match.end()):
            depth+=1 if brace[0]=='{'else -1
            if not depth:lambdas.append((match.start(),brace.end(),match[1],match[2]));break
    covered=[]
    for start,end,name,parameter in lambdas:
        for callback in LITERAL.finditer(code,start,end):
            close=close_parenthesis(code,callback.end()-1)
            if code[callback.end():close-1].strip()==parameter:covered.append(body[callback.start():close])
    if set(forwarders)<=set(covered):return 'local_unchanged_pc_callback_lambda'
    return 'dynamic_callback_or_pc_mapping_requires_owner_audit'
def handoffs(body):
    names=sorted(set(re.findall(r'\b(?:source_run_\w+|native_weapon_effect_\w+_run|handoff_\w+)\s*(?=\()',masked(body))))
    calls=[dict(instruction=a.lower(),target=b.lower())for a,b in re.findall(r'\bcall\(m,a,o,s,(0x[0-9a-f]+)u?,(0x[0-9a-f]+)u?\)',body)]
    return {'named_owner_candidates':names,'literal_effect_required_owner_calls':calls,'virtual_required_owner_present':bool(re.search(r'\b(?:required|dispatch|required_owner)\s*\(',masked(body))),'execution_of_candidates_proved':False}
def audit(root,checkpoint,cohort,hot_queue=None):
    root=root.resolve();data=json.loads(checkpoint.read_text());frozen=data['cohorts'][cohort];rows=frozen['unresolved_rows']
    effect=(root/EFFECT).read_text();exports,export_proof=effect_exports(effect)
    coverage_path=root/frozen['coverage']['path']
    if sha(coverage_path.read_bytes())!=frozen['coverage']['sha256']:raise ValueError('Frozen include-ownership coverage changed')
    coverage=json.loads(coverage_path.read_text());embedded={}
    for alias in coverage['aliases']:
        embedded.setdefault((alias['source'],alias['canonical_sha256']),set()).add(alias.get('embedded_in'))
    current={};bodies={};metadata={};source_pins={};resolution=[];kinds=collections.Counter();image_count=0;history_count=0
    variants={}
    if hot_queue:
        queue=json.loads(hot_queue.read_text())
        if(queue['cohort_report_sha256']!=frozen['report']['sha256']or queue['runtime_sha256']!=frozen['runtime_sha256']or
           queue['coverage_sha256']!=frozen['coverage']['sha256']):raise ValueError('Hot queue belongs to a different frozen execution cohort')
        for values in queue['rows']:
            row=dict(zip(queue['row_fields'],values));unit=queue['units'][row['unit']]
            variants[(unit[0],unit[1],row['canonical_symbol'],row['image'])]=row['input_sha256_variants']
    for original in rows:
        source=original['source'];path=(root/source).resolve()
        if not path.is_relative_to(root/'src')or not path.is_file():raise ValueError('Unowned census source '+source)
        if source not in current:current[source]=path.read_text();source_pins[source]=sha(current[source])
        key=(source,original['canonical_sha256'])
        if key not in bodies:
            historical,overlays=historical_source(current[source],original['canonical_sha256'])
            if historical is None:bodies[key]=(None,{},[],None)
            else:
                _,manifest=instrument(historical,historical,source)
                bodies[key]=(historical,{m[1]:body for m,body in function_bodies(historical)},overlays,{r['symbol']:r for r in manifest['functions']})
        historical,functions,overlays,functions_meta=bodies[key];symbol=original['symbol'];body=functions.get(symbol)if historical else None
        evidence={'raw_identity':{k:original[k]for k in('source','canonical_sha256','symbol','image')},'raw_counts':original['counts'],
            'raw_reasons':original['reasons'],'original_row_sha256':sha(json.dumps(original,sort_keys=True,separators=(',',':'))),
            'input_sha256_variants':variants.get((source,original['canonical_sha256'],symbol,original['image']),[]),
            'current_source_sha256':source_pins[source],'historical_source_authenticated':historical is not None,
            'exact_later_overlays_reversed':overlays,'native_dependency_closure_proved':False}
        if body is not None:
            history_count+=1;meta=functions_meta[symbol];evidence['canonical_function_body_sha256']=sha(body)
            kind=forwarder_kind(body,meta['dynamic_instruction_forwarders']);kinds[kind]+=1
            evidence.update(forwarder_kind=kind,dynamic_forwarder_expressions=meta['dynamic_instruction_forwarders'],literal_site_count=meta['literal_sites'],real_owner_handoff_candidates=handoffs(body),observed_dynamic_forwarder_execution_proved=False)
            actual=qualified_dispatch_entries(historical,original['image']).get(symbol)
            if actual:evidence['internal_actual_dispatch_owner']=actual
            if original['image']=='unknown':
                proof=None
                if source==WORLD and symbol=='required_entry_original' and sha(historical)==WORLD_PIN:
                    declaration='constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;'
                    if historical.count(declaration)!=1 or re.search(r'\b(?:auto|SourcePhysicsContactImage)\s+image\b',body):raise ValueError('World image namespace witness drift')
                    proof={'owner':WORLD,'owner_sha256':WORLD_PIN,'basis':'Exact frozen source has the namespace constexpr TU1.5 image; required_entry_original has no shadowing image parameter/local selector. Actual routed target remains dynamic.'}
                elif symbol in exports:
                    if source==EFFECT:included=True
                    else:
                        name=Path(source).name
                        included=(effect.count('#include "'+name+'"')==1 and embedded.get(key)=={EFFECT})
                    if included:
                        proof={**export_proof,'actual_export_entries':exports[symbol],'private_include_owner_verified':True,
                            'frozen_include_coverage_sha256':frozen['coverage']['sha256'],
                            'caller_image_gate_source':'tools/native_weapon_effect_state_bridge_tu15.py',
                            'caller_image_gate_source_sha256':sha((root/'tools/native_weapon_effect_state_bridge_tu15.py').read_bytes())}
                if proof:evidence['fixed_code_image_resolution']={'image':'tu15','evidence':proof,'actual_callback_operand_mapping_independently_qualified':False};image_count+=1
        else:evidence['forwarder_kind']='historical_source_unavailable_current_source_must_not_be_substituted'
        if 'no_literal_sites'in original['reasons']:evidence['no_site_scope']='No local literal sites is an attribution fact, not native callee/dependency closure.'
        resolution.append(evidence)
    drift=[source for source,pin in source_pins.items()if sha((root/source).read_text())!=pin]
    if drift:raise ValueError('Ownership audit sources changed '+str(drift))
    dependencies=('tools/generated_routine_ownership.py','tools/instrument_native_execution.py','tools/native_animation_resource_wait_replacements.py','tools/native_bsp_segment_setup_replacements.py','tools/native_page_allocate_replacements.py','tools/native_weapon_effect_state_bridge_tu15.py')
    return dict(schema='native-execution-ownership-audit-v1',cohort=cohort,checkpoint_sha256=sha(checkpoint.read_bytes()),hot_queue_sha256=sha(hot_queue.read_bytes())if hot_queue else None,
        runtime_sha256=frozen['runtime_sha256'],source_inventory_sha256=frozen['source_inventory_sha256'],frozen_include_coverage=frozen['coverage'],rows=resolution,
        totals=dict(source_qualified_identities=len(rows),historical_sources_authenticated=history_count,fixed_image_rows_qualified=image_count,raw_unknown_image_rows=sum(r['image']=='unknown'for r in rows),forwarder_kind_counts=dict(kinds)),
        source_sha256=source_pins,validation_dependency_sha256={p:sha((root/p).read_bytes())for p in dependencies},raw_counts_rewritten=False,literal_sites_recategorized=False,missing_events_inferred=False,native_closure_proved=False,
        limitations=['Fixed code-image evidence is separate from observed raw image tags and actual operand/callee identity.','Static forwarder/handoff presence does not prove that callback or callee executed.','Source-qualified literal counts remain unchanged, including instruction-shaped Effect bodies.','No-site/native-prefix returns do not establish complete native dependencies or alternate platform/image closure.'])
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,default=ROOT);p.add_argument('--checkpoint',type=Path,required=True);p.add_argument('--cohort',default='r9');p.add_argument('--hot-queue',type=Path);p.add_argument('--output',type=Path,required=True);args=p.parse_args()
    result=audit(args.root,args.checkpoint,args.cohort,args.hot_queue);result['generator_sha256']=sha(Path(__file__).read_bytes());encoded=json.dumps(result,indent=2,sort_keys=True)+'\n'
    if len(encoded.encode())>1048576:raise ValueError('Ownership receipt exceeds1MiB')
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(encoded);print(json.dumps(result['totals'],indent=2));return 0
if __name__=='__main__':raise SystemExit(main())
