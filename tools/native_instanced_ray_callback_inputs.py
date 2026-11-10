"""Compile authenticated raw/prepared callback source copies, with bounded controls."""
import re

import native_instanced_ray_callback_replacements as binding
from specialize_native_operands import specialize

UNITS=('src/physics/physics_live_support_tu15_generated.cpp',
       'src/campaign/ai/source_ai_frame_tu15_generated.cpp')
PREPARED=('6e684d3f71dfd4e0eaa698c4bc8007d8343b4f4dcdd9a35c04584c219b5ceccb',
          'd95311062165d965bb1480a598fb2bd6771daa7b283b82ffd910e2f3cefa1c8a')

def prepare(root,scratch,require_binding=False):
    pieces=[];receipts=[];rejections=0
    for name,path in zip(('primary','ai'),UNITS,strict=True):
        source=(root/path).read_text();owners=binding.authenticate(source)
        bound=binding.replace_instanced_ray_callbacks(source)
        if require_binding and bound!=source:raise ValueError('Missing production callback bindings '+path)
        assert binding.replace_instanced_ray_callbacks(bound)==bound
        prepared,_=specialize(bound)
        prepared_bodies=[pattern.search(prepared)[0]for pattern in binding.BODIES.values()]
        assert tuple(map(binding.digest,prepared_bodies))==PREPARED
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
        dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        declarations='static void run_24def0(WorldState&);\nstatic void run_24df20(WorldState&);\n'
        for suffix,text in (('bound',bound),('prepared',prepared)):
            bodies='\n'.join(pattern.search(text)[0]for pattern in binding.BODIES.values())
            pieces.append('namespace '+name+'_'+suffix+' {\n'+resolver+'\n'+declarations+bodies+'\n}\n')
        mutations=[source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),
                   source.replace(dispatcher,dispatcher.replace('run_24def0(s);','run_24df20(s);',1),1),
                   source.replace(dispatcher,dispatcher.replace('s.required(site,target,tail);','s.required(site,target+1u,tail);',1),1)]
        for entry,match,body in owners:
            offset='0x10u'if entry==0x24def0 else '0x4u'
            mutations.extend((source.replace(match[0],'',1),
                              source.replace(match[0],match[0]+match[0],1),
                              source.replace(match[0],match[0].replace(offset,'0x14u'if entry==0x24def0 else '0x8u',1),1),
                              bound.replace(binding.PREFIXES[entry],binding.PREFIXES[entry].replace('_detail::','_changed_detail::'),1)))
        query=binding.query.BODY.search(source)[0]
        mutations.append(source.replace(query,'',1))
        mutations.append(source.replace(binding.query.PREFIX,'',1))
        for changed in mutations:
            if changed in (source,bound):raise ValueError('Missing callback authentication witness '+name)
            try:binding.replace_instanced_ray_callbacks(changed)
            except ValueError:rejections+=1
            else:raise ValueError('Admitted callback whole-context mutation '+name)
        receipts.append(dict(source=path,resolver_sha256=binding.digest(resolver),
            dispatcher_sha256=binding.digest(dispatcher),
            bodies={hex(e):binding.digest(body)for e,_,body in owners},
            prepared_bodies=PREPARED))
    (scratch/'instanced_ray_callback_bodies.inc').write_text(''.join(pieces))
    return dict(owners=receipts,authentication_mutations_rejected=rejections,
                production_binding_required=require_binding,prepared_inputs_admitted=False)
