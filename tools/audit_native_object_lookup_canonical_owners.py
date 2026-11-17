#!/usr/bin/env python3
"""Prove canonical damage/vitality lookup bodies match the actual TU15 owner."""
import argparse,hashlib,json,os,re,subprocess,sys
from pathlib import Path
from specialize_native_operands import specialize
ROOT=Path(__file__).resolve().parents[1]
OWNERS={
    'direct_damage':('src/gameplay/units/source_direct_damage_runtime_generated.cpp','tools/generate_source_direct_damage_runtime.py'),
    'vitality':('src/gameplay/units/source_vitality_runtime_generated.cpp','tools/generate_source_vitality_runtime.py'),
}
REFERENCE='analysis/damage-shields-goal-20261011/vitality-runtime/pairing-full/report.json'
REFERENCE_PIN='e0f135ff479f39a091589fa7393d3a72654b8d91ffe8aa334a4a874c6a17c3d1'
ACTUAL_BODY_PIN='8fc06927ea71a7373c2f01b38037dfd22c5211a7e11976f9d386000de9ed9dfb'
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def one(source,pattern):
    values=re.findall(pattern,source,re.M|re.S)
    if len(values)!=1:raise ValueError('Missing/ambiguous complete canonical lookup context')
    return values[0]
def parts(source,owner):
    return dict(body=one(source,r'^static void run_bbcb0\(WorldState &s\) \{.*?^\}'),resolver=one(source,r'^std::uint32_t selected\([^\n]+'),dispatcher=one(source,r'^static void dispatch\([^\n]+'),sign_extend=one(source,r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}'),public_entry=one(source,r'^void source_run_'+owner+r'_runtime\(.*?^\}'))
def mapping():
    from oracle_smg_firing_1_1 import Xbe,_Code,_trace,PIN
    from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
    reference=ROOT/REFERENCE
    if hashlib.sha256(reference.read_bytes()).hexdigest()!=REFERENCE_PIN:raise ValueError('Canonical lookup correspondence changed')
    row=next(row for row in json.loads(reference.read_text())['owners']if row['old_entry']=='000bbcb0')
    if not row['full_normalized_instruction_internal_branch_topology_equal']or row['u5_entry']!='000bcdf0':raise ValueError('Lookup original correspondence differs')
    listing=[]
    for entry in row['listings']:
        path=reference.parent/entry['file']
        if hashlib.sha256(path.read_bytes()).hexdigest()!=entry['sha256']:raise ValueError('Original lookup instruction listing changed')
        listing.append([(int(line.split()[0],16),bytes.fromhex(line.split()[1]))for line in path.read_text().splitlines()])
    if any(len(rows)!=35 for rows in listing):raise ValueError('Original lookup instruction set incomplete')
    images=((ROOT/'assets/multiplayer-map-pack/demos/update/Updates/default.xbe.03',PIN),(Path(NEW_DEFAULT),NEW_PIN))
    for (path,pin),rows in zip(images,listing,strict=True):
        if hashlib.sha256(path.read_bytes()).hexdigest()!=pin:raise ValueError('Original lookup executable changed')
        image=Xbe(path);code=_Code(image,'.text')
        for pc,raw in rows:
            if image.read(pc,len(raw))!=raw:raise ValueError('Lookup listing differs from actual executable bytes')
        if sorted(_trace(code,rows[0][0],{rows[0][0]}).code)!=[pc for pc,_ in rows]:raise ValueError('Lookup listing is not the complete original CFG')
    return dict(zip((p for p,_ in listing[0]),(p for p,_ in listing[1]),strict=True)),listing
def project(context,sites):
    body=context['body'];body=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',body)
    callbacks=[int(value,16)for value in re.findall(r's\.services\.instruction\(0x([0-9a-f]+)u\)',body)]
    if callbacks!=list(sites):raise ValueError('Complete canonical lookup observation order changed')
    resolver={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',context['resolver'])}
    if resolver.get(0xbbcb0)!=0xbcdf0 or resolver.get(0x4e78d0)!=0x4ecbd0:raise ValueError('Actual canonical TU15 entry/root changed')
    body=body.replace('selected(0x4e78d0u,s.image)','0x4ecbd0u')
    # Specialization retains this exact nonidentity image choice. This audit
    # proves only its TU1.5 branch; MapPack's root remains a separate domain.
    body=body.replace('(s.image==SourcePhysicsContactImage::MapPack11?0x4e78d0u:0x4ecbd0u)','0x4ecbd0u')
    body=re.sub(r'0x([0-9a-f]+)u',lambda match:'0x'+format(sites.get(int(match[1],16),int(match[1],16)),'x')+'u',body)
    body=re.sub(r'\bL([0-9a-f]+)\b',lambda match:'L'+format(sites.get(int(match[1],16),int(match[1],16)),'x'),body)
    body=body.replace('run_bbcb0(','run_bcdf0(',1)
    if digest(body)!=ACTUAL_BODY_PIN:raise ValueError('Complete canonical lookup projection differs from the qualified actual body')
    return body
def generate(scratch,owner):
    unit,generator=OWNERS[owner];target=scratch/(owner+'.cpp')
    command=[sys.executable,'-B',str(ROOT/generator),'--output',str(target),'--manifest',str(scratch/(owner+'-generation/generation.json'))]
    done=subprocess.run(command,capture_output=True,text=True)
    if done.returncode:raise ValueError('Genuine canonical lookup generation failed: '+(done.stdout+done.stderr)[-7000:])
    return [(ROOT/unit).read_text(),target.read_text()],command
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--candidate',action='store_true');p.add_argument('--require-binding',action='store_true');a=p.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):p.error('Run through tools/run_test.py')
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);a.output.mkdir(parents=True,exist_ok=False)
    names=[name for pair in OWNERS.values()for name in pair]+[REFERENCE,'tools/audit_native_object_lookup_canonical_owners.py','tools/generate_physics_world_bootstrap_runtime_20261011.py','tools/specialize_native_operands.py','src/physics/runtime/physics_world_runtime_internal.hpp','src/physics/runtime/physics_contact_runtime_internal.hpp']
    pins={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in names};report=dict(passed=False,production_bound=False,owned_scratch=str(scratch),source_before=pins,retained_large_artifacts=[])
    try:
        binding=None
        if a.candidate:
            import native_object_lookup_canonical_replacements as binding
            for name in('tools/native_object_lookup_canonical_replacements.py','src/gameplay/objects/native_tu15_object_body_canonical_source_owner.hpp','src/gameplay/objects/native_tu15_object_body_canonical_sites.hpp'):
                names.append(name);pins[name]=hashlib.sha256((ROOT/name).read_bytes()).hexdigest()
        sites,listings=mapping();contexts=[]
        for owner in OWNERS:
            actual,command=generate(scratch,owner)
            if a.require_binding:
                for source in actual:
                    if binding.replace_canonical_object_lookup(source)!=source:raise ValueError('Canonical current/fresh native default missing')
                    binding.installed_entries(source)
            values=[binding.remove_canonical_object_lookup(value)if binding else value for value in actual]
            sources=values+[specialize(value)[0]for value in values];labels=['current','fresh','current-prepared','fresh-prepared']
            if binding:
                proposals=[]
                for source in sources:
                    candidate=binding.replace_canonical_object_lookup(source)
                    if candidate==source or binding.remove_canonical_object_lookup(candidate)!=source or binding.replace_canonical_object_lookup(candidate)!=candidate:raise ValueError('Canonical exact overlay inverse/idempotence drift')
                    proposals.append(candidate)
                sources+=proposals;labels+=[label+'-candidate'for label in labels]
            for index,(label,source)in enumerate(zip(labels,sources,strict=True)):
                context=parts(source,owner)
                if binding:context['body']=binding.canonical_body(context['body'])
                try:projection=project(context,sites)
                except ValueError as error:raise ValueError(owner+' '+label+': '+str(error))from error
                syntax=[];refusals={}
                if binding:
                    for key,part in binding.parts(source,owner).items():
                        at=part.index('{')+1;changed=source.replace(part,part[:at]+'/* exact owner drift */'+part[at:],1)
                        try:binding.authenticate(changed)
                        except ValueError as error:refusals[key]=str(error)
                        else:raise ValueError('Canonical context drift admitted: '+key)
                    if binding.entry_prefix(binding.parts(source,owner)['body']) is not None:
                        for pc in binding.FRONTIERS:
                            label_pc=f'CanonicalObjectBody{pc:x}: ';entry='s.instruction_literal(0xbbcb0u);'if 's.instruction_literal(0xbbcb0u);'in source else'if(s.services.instruction){s.services.instruction(0xbbcb0u);}'
                            changed=source.replace(label_pc,'',1).replace(entry,label_pc+entry,1)
                            try:binding.authenticate(changed)
                            except ValueError as error:refusals[f'relocated_{pc:x}']=str(error)
                            else:raise ValueError('Canonical continuation relocation admitted')
                target=scratch/(owner+'-'+label+'.cpp');target.write_text(source)
                for compiler in('g++','clang++'):
                    cmd=[compiler,'-std=c++20','-fsyntax-only','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(target)];done=subprocess.run(cmd,capture_output=True,text=True)
                    if done.returncode:raise ValueError('Complete canonical owner syntax failed: '+(done.stdout+done.stderr)[-7000:])
                    syntax.append(dict(command=cmd,passed=True))
                contexts.append(dict(owner=owner,variant=label,context_sha256={key:digest(value)for key,value in context.items()},whole_source_sha256=digest(source),exact_projection_sha256=digest(projection),whole_cpp_syntax=syntax,strict_context_refusals=refusals,genuine_generator=command))
        report.update(passed=True,contexts=contexts,canonical_to_actual_pc={hex(p):hex(q)for p,q in sites.items()},both_original_executable_bytes_and_complete_cfg_authenticated=True,production_bound=a.require_binding,qualified_parent_or_default=False)
    except BaseException as error:report['error']=repr(error);raise
    finally:
        report['source_after']={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest()for name in names}
        if report['source_after']!=pins:report.update(passed=False,source_drift=True)
        (a.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    return 0
if __name__=='__main__':raise SystemExit(main())
