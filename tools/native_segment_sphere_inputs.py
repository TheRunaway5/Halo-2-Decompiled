"""Pin source bodies and the existing public120FA0 numerical view."""
import hashlib,re
from audit_native_execution_ownership import unique_function
import native_segment_sphere_replacements as binding
PUBLIC='src/physics/native_tu15_camera_query_leaves.cpp'
UNITS=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp')
MEMORY_PIN='a46657fe6dcc3c7cd4c9c9d88f4edfbecb0d76bee3a9adc01783c57ffbd6dd19'
ENV_PIN='ca3a19c0f6cd7d48e6228ac852e026322b645c18ff80f8cbfc17abcae9824444'
COMPARISON_PIN='302f08d48a25ed0d9cb887c992dca07f3fb17568c994ee4e7e1159ac10ebe11d'
HELPERS={'scalar': 'd7040084745a69e69905c306de6e38366c75bf9f851852c27320a8cf9a93d9cc', 'mul': '18d4d1a301c5f62be01a02031b37731eeb01e01690acb6160b48c2d8c08e3836', 'add': '81267e61fe0e9952ef9845ea925504c51fa4d46ead2c8c14632ef33484cb0cbc', 'sub': 'c4000a5fa48bdc7b1fefe1df55ff77eaf6092fbdc6bef1a22e9f061d52e7c167', 'compare': '8a1abc122ad8d69a4f60eaae87c81ece78352baf380a518d97629e98a9494c69'}
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def prepare_public(root,scratch):
    source=(root/PUBLIC).read_text();current=unique_function(source,'native_tu15_segment_near_sphere')
    body=(root/'tests/native_tu15_segment_sphere_reference.inc').read_text().rstrip('\n');assert digest(body)==binding.PUBLIC_PIN
    if current!=binding.PUBLIC_BODY and digest(current)!=binding.PUBLIC_PIN:raise ValueError('Public120FA0 source drift')
    memory=re.search(r'class Memory \{.*?\n\};',source,re.S)[0];assert digest(memory)==MEMORY_PIN
    environment=re.search(r'class SseEnvironment \{.*?\n\};',source,re.S)[0];assert digest(environment)==ENV_PIN
    comparison=re.search(r'struct Comparison\{[^\n]*\};',source)[0];assert digest(comparison)==COMPARISON_PIN
    helpers=[]
    for name,pin in HELPERS.items():
        helper=unique_function(source,name);assert digest(helper)==pin;helpers.append(helper)
    common='\n'.join(('using F=__m128;',memory,environment,comparison,*helpers))
    (scratch/'sphere_values.inc').write_text('namespace public_reference {\n'+common+'\n'+body+'\n}\nnamespace public_shared {\n'+common+'\n'+binding.PUBLIC_BODY+'\n}\n')
    return dict(original_public_body=binding.PUBLIC_PIN,shared_public_body=digest(binding.PUBLIC_BODY),memory=MEMORY_PIN,environment=ENV_PIN,helpers=HELPERS)
def prepare_source(root,scratch,require_binding=False):
    from specialize_native_operands import specialize
    bodies=[];receipts=[]
    for index,unit in enumerate(UNITS):
        source=(root/unit).read_text();match,original=binding.authenticate(source);bound=binding.replace_segment_sphere(source)
        if require_binding and bound!=source:raise ValueError('Production120FA0 binding missing '+unit)
        assert binding.replace_segment_sphere(bound)==bound
        prepared,_=specialize(bound);body=binding.BODY.search(prepared)[0]
        assert binding.PREFIX in body and 's.instruction_literal('in body
        if binding.PREPARED_OUTPUT_PIN is not None:assert digest(body)==binding.PREPARED_OUTPUT_PIN
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
        bodies.append('namespace copy'+str(index)+' {\n'+resolver+'\nnamespace bound {\n'+binding.BODY.search(bound)[0]+'\n}\nnamespace prepared {\n'+body+'\n}\n}\n')
        dispatch=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        controls=(source.replace(match[0],match[0].replace('0x4u','0x5u',1),1),source.replace(match[0],match[0].replace('_mm_sub_ss','_mm_add_ss',1),1),source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),source.replace(dispatch,dispatch.replace('run_120fa0(s);','run_16b810(s);',1),1),bound.replace(binding.PREFIX,binding.PREFIX.replace('::bridge','::changed_bridge'),1))
        for changed in controls:
            assert changed not in(source,bound),'Missing120FA0 drift witness'
            try:binding.replace_segment_sphere(changed)
            except ValueError:pass
            else:raise ValueError('Complete120FA0 body/resolver/dispatch drift admitted')
        receipts.append(dict(unit=unit,original_body=digest(original),prepared_body=digest(body),source_drift_rejections=len(controls)))
    (scratch/'sphere_bodies.inc').write_text('\n'.join(bodies))
    return receipts
