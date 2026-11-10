"""Exact source-copy routes for the complete native16C000 parent."""
import re
import native_camera_instance_query_replacements as binding
from specialize_native_operands import specialize

UNITS=('src/physics/physics_live_support_tu15_generated.cpp',
       'src/campaign/ai/source_ai_frame_tu15_generated.cpp')


def prepare(root,scratch,require_binding=False):
    pieces=[];receipts=[];rejections=0
    for name,path in zip(('primary','ai'),UNITS,strict=True):
        source=(root/path).read_text();match,original=binding.authenticate(source)
        bound=binding.replace_camera_instance_query(source)
        if require_binding and bound!=source:
            raise ValueError('Production16C000 binding missing: '+path)
        assert binding.replace_camera_instance_query(bound)==bound
        prepared,_=specialize(bound);body=binding.BODY.search(prepared)[0]
        assert binding.PREFIX in body and 's.instruction_literal(' in body
        assert binding.digest(body) in binding.PREPARED_OUTPUT_PINS
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
        dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        for suffix,code in (('bound',binding.BODY.search(bound)[0]),('prepared',body)):
            pieces.append('namespace '+name+'_'+suffix+' {\n'+resolver+'\n'+code+'\n}\n')
        changes=[source.replace(match[0],match[0].replace('0x480u','0x484u',1),1),
                 source.replace(match[0],match[0].replace('s.gp[4]+=16u','s.gp[4]+=12u',1),1),
                 source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),
                 source.replace(dispatcher,dispatcher.replace('run_16c000(s);','run_16b810(s);',1),1),
                 source.replace(dispatcher,dispatcher.replace('s.required(site,target,tail);','s.required(site,target+1u,tail);',1),1),
                 bound.replace(binding.PREFIX,binding.PREFIX.replace('::bridge','::changed_bridge'),1)]
        for entry in (0x16b810,0x186e30,0x120fa0,0x1e68e0):
            child=re.search(rf'static void run_{entry:x}\(WorldState &s\) \{{.*?\n\}}',source,re.S)[0]
            changes.append(source.replace(child,'',1))
        if name=='primary':
            child=re.search(r'static void run_16b9a0\(WorldState &s\) \{.*?\n\}',source,re.S)[0]
            changes.append(source.replace(child,'',1))
        other=UNITS[1]if path==UNITS[0]else UNITS[0]
        other_body=binding.canonical_body(binding.BODY.search((root/other).read_text())[0])
        changes.append(source.replace(match[0],other_body,1))
        for changed in changes:
            if changed in (source,bound):raise ValueError('Missing16C000 authentication witness '+name)
            try:binding.replace_camera_instance_query(changed)
            except ValueError:rejections+=1
            else:raise ValueError('16C000 installer admitted changed complete owner '+name)
        receipts.append(dict(source=path,retained_body_sha256=binding.digest(original),
            resolver_sha256=binding.digest(resolver),dispatcher_sha256=binding.digest(dispatcher),
            prepared_output_sha256=binding.digest(body)))
    (scratch/'camera_query_parent_bodies.inc').write_text(''.join(pieces))
    return dict(owners=receipts,authentication_mutations_rejected=rejections,
                prepared_input_admitted=False)
