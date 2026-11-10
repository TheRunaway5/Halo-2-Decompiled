"""Authenticate the existing public affine-inverse policy before shared reuse."""
import hashlib,re
from audit_native_execution_ownership import unique_function
PUBLIC='src/rendering/world/native_tu15_camera_projection.cpp'
PUBLIC_PIN='da4407e33228af6377a54c8ca21b166ce444401cb0581a0c943b4c58d4bba964'
ENV_PIN='d1a7224fb574b2a916b8a13fb9a1f9d15a52ef751377126ba2676a9231619675'
HELPERS={'read': '36bebc1001ddf599c523daa0c6c255e04a4f9a9ddf4ed6a6665bd0d5e94b7c46', 'publish': 'c2347051c37d2044b308cfa19be7b8c276576d3737ae87ae4291ec52e5aa2372', 'scalar': 'b667db2bcb75cbb9637f15f43fb38ee30fa7784eb9fafed009ea59cb0f8c2b15', 'bits': '39fa12cb02e714ef3e34fe5c49ac79e9f325b8a62a1a20938937ec6434efd381', 'add': '0ff74b8978558b3991d5445c9d0755f1db4afa0593810a2036540ca7f6631c9b', 'sub': '5170eda1a0213920b4b4f8df0682aa11513d1c57597910b70b2c9ab94dfb720d', 'mul': 'f07376d0a394c88a484e61acc32f05f411ec6a0bfd43691c789c88e00c5d1aaa', 'div': 'f7e6816b7bab8d21d2218f22f9c684e50ae69a5e4923b227eb69e3ae4f001a10', 'equal': '021a542e135d1fd54b4381a04c11889e9c0493a16b38c7a2fe7b7349d441c6f3'}
SHARED_BODY='void inverse_frame(SourcePhysicsContactMemory &memory,std::uint32_t source,\n    std::uint32_t destination,const SourcePhysicsContactRecordObserver &observe) {\n    auto read_word=[&](std::uint32_t address){return read(memory,address);};\n    auto write_word=[&](std::uint32_t pc,std::uint32_t address,std::uint32_t value){publish(memory,observe,pc,address,value);};\n    native_affine_inverse_detail::Values view{read_word,write_word,source,destination};\n    native_affine_inverse_detail::Progress data;data.source=source;data.destination=destination;\n    native_affine_inverse_detail::invert(view,data);\n}'
UNITS=('src/campaign/ai/source_ai_frame_tu15_generated.cpp','src/runtime/source_map_world_support_tu15_generated.cpp','src/physics/runtime/physics_world_runtime_generated.cpp','src/physics/vehicles/physics_vehicle_motion_generated.cpp')
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def prepare_public(root,scratch):
    source=(root/PUBLIC).read_text();current=unique_function(source,'inverse_frame')
    body=(root/'tests/native_tu15_affine_inverse_reference.inc').read_text().rstrip('\n');assert digest(body)==PUBLIC_PIN
    if current!=SHARED_BODY and digest(current)!=PUBLIC_PIN:raise ValueError('Public144090 inverse body drift')
    environment=re.search(r'class SimdEnvironment \{.*?\n\};',source,re.S)[0];assert digest(environment)==ENV_PIN
    helpers=[]
    for name,pin in HELPERS.items():
        helper=unique_function(source,name);assert digest(helper)==pin;helpers.append(helper)
    common='\n'.join(('using Scalar=__m128;',environment,*helpers))
    (scratch/'inverse_values.inc').write_text('namespace public_reference {\n'+common+'\n'+body+'\n}\nnamespace public_shared {\n'+common+'\n'+SHARED_BODY+'\n}\n')
    return dict(original_body=PUBLIC_PIN,shared_body=digest(SHARED_BODY),helpers=HELPERS,environment=ENV_PIN)
def prepare_source(root,scratch,require_binding=False):
    import native_affine_inverse_replacements as binding
    from specialize_native_operands import specialize
    bodies=[];receipts=[]
    for index,unit in enumerate(UNITS):
        source=(root/unit).read_text();match,body,mode,owner=binding.authenticate(source)
        bound=binding.replace_affine_inverse(source)
        if require_binding and bound!=source:raise ValueError('Production affine-inverse binding missing '+unit)
        assert binding.replace_affine_inverse(bound)==bound
        prepared,_=specialize(bound);prepared_body=binding.BODY.search(prepared)[0]
        assert binding.prefix(mode)in prepared_body
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)[0]
        helper=re.search(r'std::uint32_t world_observation_pc\(.*?\n\}',source,re.S)[0]if mode=='world'else''
        bodies.append('namespace copy'+str(index)+' {\nusing WorldState=InstrumentedState;using ObservedWorldState=InstrumentedState;using MotionState=InstrumentedState;\n'+resolver+'\n'+helper+'\nnamespace bound {\n'+binding.BODY.search(bound)[0]+'\n}\nnamespace prepared {\n'+prepared_body+'\n}\n}\n')
        dispatcher=re.search(r'static void dispatch\('+match[2]+r' &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)[0]
        controls=[source.replace(match[0],match[0].replace('_mm_mul_ss','_mm_add_ss',1),1),source.replace(match[0],match[0].replace('0x28u','0x2cu',1),1),source.replace(resolver,resolver.replace('return p;','return p+1u;',1),1),source.replace(dispatcher,dispatcher.replace('run_'+match[1]+'(s);','run_30bd0(s);',1),1),bound.replace(binding.prefix(mode),binding.prefix(mode).replace('::bridge','::changed_bridge'),1)]
        if helper:controls.append(source.replace(helper,helper.replace(' && ',' || ',1),1))
        for changed in controls:
            assert changed not in(source,bound),'Missing affine-inverse drift witness'
            try:binding.replace_affine_inverse(changed)
            except ValueError:pass
            else:raise ValueError('Affine-inverse body/resolver/dispatcher/mode drift admitted')
        receipts.append(dict(unit=unit,owner=owner,observation=mode,original_body=digest(body),resolver=digest(resolver),dispatcher=digest(dispatcher),prepared_body=digest(prepared_body),source_drift_rejections=len(controls)))
    (scratch/'inverse_bodies.inc').write_text('\n'.join(bodies))
    # Genuine inherited storage/API provenance is separate from the test's
    # instrumented State alias. Whole-unit compilation still uses real types.
    motion=(root/'src/physics/vehicles/physics_vehicle_motion_internal.hpp').read_text()
    vehicle=(root/'src/physics/vehicles/physics_vehicle_runtime_internal.hpp').read_text()
    assert 'struct MotionState : VehicleState {'in motion and 'struct VehicleState : WorldState {'in vehicle
    return dict(copies=receipts,motion_state_header=digest(motion),vehicle_state_header=digest(vehicle),test_state_alias_scope='Same inherited WorldState ABI; genuine production type compilation is a separate whole-unit gate')
