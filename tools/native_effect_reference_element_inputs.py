"""Freeze exact complete current/fresh raw/prepared effect-reference owners."""
import re,subprocess,sys
import native_effect_reference_element_replacements as binding
from audit_native_effect_reference_generation_tu15 import UNITS
from specialize_native_operands import specialize

def prepare(root,scratch,require_binding=False):
    pieces=[];receipts=[];includes=set();rejected=0;routes=[];route_owners=[];route_external=[]
    for name,unit,generator,run,owns in UNITS:
        folder=scratch/name;folder.mkdir();fresh=folder/'fresh.cpp'
        command=[sys.executable,'-B',str(root/generator),'--output',str(fresh),'--directory',str(folder/'generation')]
        generated=subprocess.run(command,cwd=root,capture_output=True,text=True,timeout=300)
        if generated.returncode:raise ValueError('Effect-reference fresh whole generation failed '+generated.stderr[-7000:])
        current=(root/unit).read_text()
        old_bodies={entry:body for _,entry,body in binding.authenticate(current)}
        for origin,source in(('current',current),('fresh',fresh.read_text())):
            authenticated=binding.authenticate(source);bound=binding.replace_effect_reference_element(source)
            if require_binding and source!=bound:raise ValueError('Installed effect-reference source binding missing '+name+'/'+origin)
            if binding.replace_effect_reference_element(bound)!=bound:raise ValueError('Effect-reference source binding nonidempotent')
            if origin=='fresh':
                for _,entry,body in authenticated:
                    projection=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',old_bodies[entry])
                    if projection!=body:raise ValueError('Fresh effect-reference body has an unqualified nonidentity-literal delta')
            prepared,receipt=specialize(bound)
            state=re.search(r'^struct LiveState :.*?^\};',source,re.M|re.S)[0]
            resolver=binding.function(source,'selected');dispatcher=binding.function(source,'dispatch')
            local=set(re.findall(r'\brun_([0-9a-f]+)\(s\);',dispatcher));own={f'{entry:x}'for _,entry,_ in authenticated}
            includes.update(re.findall(r'^#include .+$',source,re.M))
            for variant,text in(('raw',bound),('prepared',prepared)):
                namespace=name+'_'+origin+'_'+variant;routes.append(namespace);route_owners.append(own)
                route_external.append({'22b790','22aca0','22b710'}-local)
                code='namespace '+namespace+' {\nusing namespace halo2::physics_world_runtime_detail;\n'+state+'\nusing WorldState=LiveState;\n'+resolver+'\n'+binding.function(source,'sign_extend')+'\n'
                code+='\n'.join('static void run_'+entry+'(WorldState&);'for entry in sorted(local|own))+'\nstatic void dispatch(WorldState&,std::uint32_t,std::uint32_t,bool);\n'
                for entry in sorted(local-own):code+='static void run_'+entry+'(WorldState&){throw std::runtime_error("Unqualified fixture owner '+entry+'");}\n'
                for _,entry,_ in authenticated:
                    body=binding.function(text,f'run_{entry:x}')
                    body=re.sub(r'(?m)^((?:L[0-9a-f]+: )?)(?=if\(s\.services\.instruction\)|s\.instruction_literal\()',r'\1::effect_reference_statement(0x'+f'{entry:x}'+r'u);',body)
                    code+=body+'\n'
                code+=dispatcher+'\n'
                entered='switch(entry){'+''.join('case 0x'+entry+'u:run_'+entry+'(state);break;'for entry in sorted(own))+'default:throw std::runtime_error("Unqualified fixture entry");}'
                code+='''static void route(SourcePhysicsContactMemory&m,const SourcePhysicsContactRecordObserver&o,
const SourcePhysicsWorldRuntimeServices&s,SourcePhysicsWorldRuntimeAbi&a,std::uint32_t entry){
 WorldState state{m,o,s,SourcePhysicsContactImage::TitleUpdate15};state.load(a);
 auto*previous=::effect_reference_live;::effect_reference_live=&state;
 try{'''+entered+'''}catch(...){a=state.save();::effect_reference_live=previous;throw;}
 a=state.save();::effect_reference_live=previous;
}
}\n'''
                pieces.append(code)
            resolver_body=next(match[0]for match,entry,_ in authenticated if entry==0x178070)
            mutations=[source.replace(resolver_body,'',1),source.replace(resolver_body,resolver_body*2,1),
                source.replace(resolver_body,resolver_body.replace('0x4f0414u','0x4f0418u',1),1),
                source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),
                source.replace(dispatcher,dispatcher.replace('run_178070(s);','run_13a6b0(s);',1),1),
                bound.replace(binding.INCLUDE,'',1),bound.replace(binding.prefix(0x178070),binding.prefix(0x178070).replace('run_13a6b0(child)','run_178070(child)'),1),
                source.replace(binding.function(source,'sign_extend'),binding.function(source,'sign_extend').replace('if(n==2)','if(n==3)',1),1)]
            for index,changed in enumerate(mutations):
                if changed in(source,bound):raise ValueError('Missing effect-reference source mutation witness '+name+'/'+origin+'/'+str(index))
                try:binding.authenticate(changed);binding.replace_effect_reference_element(changed)
                except ValueError:rejected+=1
                else:raise ValueError('Changed complete effect-reference context admitted')
            receipts.append(dict(name=name,origin=origin,source_sha256=binding.digest(source),prepared_sha256=binding.digest(prepared),
                body_sha256={f'{entry:x}':binding.digest(body)for _,entry,body in authenticated},dispatcher_sha256=binding.digest(dispatcher),
                local_owner_subset=sorted(own),untouched_dispatcher=True,other_fixture_owners_explicitly_unqualified=True))
    pieces.append('using EffectReferenceRoute=void(*)(SourcePhysicsContactMemory&,const SourcePhysicsContactRecordObserver&,const SourcePhysicsWorldRuntimeServices&,SourcePhysicsWorldRuntimeAbi&,std::uint32_t);\n')
    pieces.append('inline const std::array<EffectReferenceRoute,'+str(len(routes))+'>effect_reference_routes{'+','.join(name+'::route'for name in routes)+'};\n')
    pieces.append('inline const std::array<std::set<std::uint32_t>,'+str(len(routes))+'>effect_reference_local_entries{'+','.join('std::set<std::uint32_t>{'+','.join('0x'+entry+'u'for entry in sorted(owners))+'}'for owners in route_owners)+'};\n')
    pieces.append('inline const std::array<std::set<std::uint32_t>,'+str(len(routes))+'>effect_reference_external_entries{'+','.join('std::set<std::uint32_t>{'+','.join('0x'+entry+'u'for entry in sorted(owners))+'}'for owners in route_external)+'};\n')
    (scratch/'effect_reference_source_includes.inc').write_text('\n'.join(sorted(includes))+'\n')
    (scratch/'effect_reference_source_bodies.inc').write_text(''.join(pieces))
    return dict(source_files=[unit for _,unit,*_ in UNITS],owners=receipts,source_variants=len(routes),authentication_mutations_rejected=rejected,
        fresh_delta='Only complete-pinned identity-selected literal projection; neither retained source body rewritten',production_binding_required=require_binding,
        entry_scope='Actual complete compiled owner bodies; their genuine internal dispatcher is unchanged, including external accessor routes; public source routing separately pinned')
