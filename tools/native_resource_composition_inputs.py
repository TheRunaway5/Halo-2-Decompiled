"""Source-authenticated test-only platform owners for resource composition.

This compiles the existing World retained-read algorithm in a frozen fixture
context. It does not expose or replace the live World's private epoch state.
"""
import hashlib
import importlib
from pathlib import Path
import re
from audit_native_execution_ownership import unique_function
from generated_routine_ownership import function_bodies,qualified_dispatch_entries
ROOT=Path(__file__).resolve().parents[1]
REGISTRY_METHOD_PIN='04ecf76da8482b3fc6e2f97cf89e10f1c24766c7303df498dddb190e8225aecc'
ANIMATION_PROVIDER='''        if(source_try_animation_cache_io_tu15(m,bank,c,a,main_thread,
            [this](auto descriptor){return resources.animation(descriptor);},
            [this](auto descriptor){return bank.view(descriptor,12);},
            [this](const SourceAnimationCacheReadTu15& read){return resources.load(read.encoded_offset,read.bytes);},
            [this](const SourceAnimationCacheReadTu15& read){return retain_resource_read({read.descriptor,read.destination,read.bytes},true);}))return;'''
def freeze_resource_registry(scratch):
    world=ROOT/'src/physics/physics_live_world_tu15.cpp';header=ROOT/'include/halo2/physics_live_world_tu15.hpp'
    source=world.read_text();method=unique_function(source,'retain_resource_read')
    if hashlib.sha256(method.encode()).hexdigest()!=REGISTRY_METHOD_PIN:raise ValueError('Complete World retained resource-read owner drift')
    if source.count(ANIMATION_PROVIDER)!=1:raise ValueError('Actual World animation-provider routing/epoch argument changed')
    declaration='struct ResourceRead {std::uint32_t descriptor,destination,bytes;};'
    if header.read_text().count(declaration)!=1:raise ValueError('Actual World retained-read schema changed')
    initialization='std::uint64_t next_resource_read=0xf8000001u;'
    if source.count(initialization)!=1:raise ValueError('Actual World retained-read initial namespace changed')
    output=('struct ResourceReadOwner {\n'+declaration+'\nusing NativeResourceRead=ResourceRead;\n'
        'bool body_pose_publication_active{};\nstd::map<std::uint32_t,NativeResourceRead> resource_reads;\n'+initialization+'\n'+method+'\n};\n')
    path=Path(scratch)/'resource_composition_registry.inc';path.write_text(output)
    return dict(world_source_sha256=hashlib.sha256(source.encode()).hexdigest(),world_method_sha256=REGISTRY_METHOD_PIN,
        animation_provider_sha256=hashlib.sha256(ANIMATION_PROVIDER.encode()).hexdigest(),fixture_body_pose_publication_active=False,
        schema_header_sha256=hashlib.sha256(header.read_bytes()).hexdigest(),fixture_owner_sha256=hashlib.sha256(output.encode()).hexdigest(),
        scope='Exact existing native World map/counter/epoch-admission method in a test-owned context; live World private epoch/restore integration remains unproved.')

# Whole production owners only. Imported iterator/delete/stack owners retain
# their genuine AI implementation; this never invents a missing Primary body.
CONTROLLERS=(
    ('1e5c80','native_node_resource_lifecycle_replacements','replace_node_resource_lifecycle'),
    ('1e5af0','native_node_resource_replacements','replace_node_resource'),
    ('1e3270','native_record_lookup_replacements','replace_record_lookup'),
    ('126200','native_resource_touch_replacements','replace_resource_touch'),
    ('126190','native_resource_record_search_replacements','replace_resource_record_search'),
    ('126480','native_resource_record_count_replacements','replace_resource_record_count'),
    ('1264e0','native_resource_maintenance_replacements','replace_resource_maintenance'),
    ('126060','native_resource_page_replacements','replace_resource_page'),
    ('1260c0','native_animation_resource_read_replacements','replace_animation_resource_read'),
    ('1262b0','native_animation_resource_wait_replacements','replace_animation_resource_wait'),
    ('13fe50','native_page_allocate_replacements','install_native_page_allocate'),
    ('13fe00','native_page_window_replacements','install_native_page_window'),
    ('140310','native_page_retire_replacements','install_native_page_retire'),
    ('32a660','native_stack_probe_replacements','install_native_stack_probe'),
    ('16e6a0','native_effect_datum_allocate_replacements','install_native_effect_datum_allocate'),
    ('16e770','native_datum_specific_replacements','replace_datum_specific'),
    ('16e860','native_datum_initialize_replacements','replace_datum_initialize'),
    ('16ea60','native_effect_slot_scan_replacements','install_native_effect_slot_scan'),
    ('16e8a0','native_datum_delete_replacements','install_native_datum_delete'),
    ('33b590','native_millisecond_clock_replacements','replace_millisecond_clock'),
    ('1236a0','native_async_promote_replacements','install_native_async_promote'),
    ('2dbf74','native_kernel_yield_replacements','replace_kernel_yield'),
)
CONTROLLER_SOURCES=('src/physics/physics_live_support_tu15_generated.cpp','src/campaign/ai/source_ai_frame_tu15_generated.cpp')
def freeze_controller_sources(scratch):
    snapshots={name:(ROOT/name).read_text()for name in CONTROLLER_SOURCES}
    indexes={name:{signature[1]:body for signature,body in function_bodies(text)}for name,text in snapshots.items()}
    includes=['#include "physics/runtime/physics_world_runtime_internal.hpp"\n',
        '#include "physics/runtime/native_tu15_physical_epoch_abi.hpp"\n',
        '#include "platform/native_tu15_tick_conversion.hpp"\n',
        '#include "physics/runtime/native_tu15_resource_busy_abi.hpp"\n'];native=[];reference=[];proof=[]
    entries={entry for entry,_,_ in CONTROLLERS}|{'1232c0','1ab330'}
    for entry,module_name,install_name in CONTROLLERS:
        module=importlib.import_module(module_name)
        source_name=next((name for name in CONTROLLER_SOURCES if entry in indexes[name]),None)
        if source_name is None:raise ValueError('Missing real resource child owner '+entry)
        source=snapshots[source_name];body=indexes[source_name][entry];bound=getattr(module,install_name)(source)
        if bound!=source:raise ValueError('Resource composition requires current native-default owner '+entry)
        owner=qualified_dispatch_entries(source,'tu15').get(entry)
        if not owner or owner['entry']!=int(entry,16):raise ValueError('Resource composition actual child identity drift '+entry)
        canonical=module.canonical_body(body)
        if canonical==body:raise ValueError('Resource composition lacks exact native admission '+entry)
        # Binders have already authenticated their complete selected resolver
        # and identity operands. Literal transport keeps this one TU1.5 scope.
        literal=lambda text:re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',text)
        native.append(literal(body));reference.append(literal(canonical))
        includes.append('#include "halo2/native_weapon_effect_dispatch_tu15.hpp"\n'if module_name=='native_effect_datum_allocate_replacements'else module.INCLUDE)
        proof.append(dict(entry=entry,source=source_name,source_sha256=hashlib.sha256(source.encode()).hexdigest(),
            bound_body_sha256=hashlib.sha256(body.encode()).hexdigest(),canonical_body_sha256=hashlib.sha256(canonical.encode()).hexdigest(),
            binder=module_name,dispatcher=owner,primary_body_present=entry in indexes[CONTROLLER_SOURCES[0]]))
        proof[-1]['binder_source_sha256']=hashlib.sha256(Path(module.__file__).read_bytes()).hexdigest()
    # The live promotion prefix already composes these qualified typed owners.
    # Its retained control still names their complete literal functions; use
    # the actual production algorithms for that reference path, not results.
    from native_async_insert_replacements import authenticate as authenticate_insert
    from native_async_order_replacements import authenticate as authenticate_compare
    async_sources=(('1232c0','src/rendering/shadows/source_shadow_frame_tu15_generated.cpp',authenticate_insert),
        ('1ab330','src/physics/physics_game_arena_runtime_generated.cpp',authenticate_compare))
    for entry,source_name,authenticate in async_sources:
        text=(ROOT/source_name).read_text()
        _,body=authenticate(text,'1232c0')if entry=='1232c0'else authenticate(text)
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',text,re.S)[0]
        mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
        body=re.sub(r'selected\(0x([0-9a-f]+)u,s.image\)',lambda m:f'0x{mapping[int(m[1],16)]:x}u',body)
        if entry=='1ab330':
            # Same authenticated mapped comparator used by the standalone
            # promotion proof: observation labels become actual TU1.5 sites.
            body=re.sub(r'0x([0-9a-f]+)u',lambda m:f'0x{int(m[1],16)+0x62b0:x}u'if 0x1a5080<=int(m[1],16)<=0x1a5109 else m[0],body)
            body=body.replace('run_1a5080(','run_1ab330(',1)
        reference.append(body)
        proof.append(dict(entry=entry,source=source_name,source_sha256=hashlib.sha256(text.encode()).hexdigest(),body_sha256=hashlib.sha256(body.encode()).hexdigest(),
            role='Authenticated mapped literal child control; native promotion composes the existing async ABI owner, whose instruction-shaped internals remain separately inventoried.'))
    native.extend(('static void run_1ab330(WorldState&s){if(!native_async_order_detail::compare(s,0x1ab330u))throw std::runtime_error("Actual async comparator refused");}',
        'static void run_1232c0(WorldState&s){if(!native_async_order_detail::insert(s,0x1232c0u,[](auto&v){run_1ab330(v);}))throw std::runtime_error("Actual async insertion refused");}'))
    prefix=('using namespace halo2;using namespace halo2::physics_world_runtime_detail;\n'
        'inline std::uint32_t sign_extend(std::uint32_t value,unsigned width){return width==1?std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value)))):std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(value))));}\n'
        'inline std::uint32_t canonical_branch(std::uint32_t value,SourcePhysicsContactImage image){if(image!=SourcePhysicsContactImage::TitleUpdate15)throw std::runtime_error("Unqualified resource image");return value;}\n'
        +''.join('static void run_'+entry+'(WorldState&);\n'for entry in sorted(entries))
        +'static void dispatch(WorldState&s,std::uint32_t target,std::uint32_t site,bool tail){before_owner(s,target,site,tail);'
        +'switch(target){'
        +''.join(f'case 0x{entry}u:run_{entry}(s);return;'for entry in sorted(entries))
        +'case 0x12f040u:case 0x12f0c0u:case 0x13fd10u:'
        +'if(native_partition_temporary_detail::admitted(s)&&native_physical_epoch_detail::run(s,target,[](auto&v,std::uint32_t owner,std::uint32_t at,bool is_tail){dispatch(v,owner,at,is_tail);}))return;break;'
        +'case 0x22a5c0u:case 0x32ae70u:case 0x32acd0u:'
        +'if(native_partition_temporary_detail::admitted(s)&&native_tick_conversion_detail::run(s,target))return;break;'
        +'case 0x12fcb0u:if(native_resource_busy_detail::callback(s,[](auto&v,std::uint32_t owner,std::uint32_t at,bool is_tail){dispatch(v,owner,at,is_tail);}))return;break;'
        +'case 0x4057f0u:if(native_resource_busy_detail::query(s))return;break;'
        +'default:break;}platform_owner(s,target,site,tail);}\n')
    generated=''.join(dict.fromkeys(includes))+'\nnamespace resource_composition {\n'
    for name,bodies in(('native',native),('reference',reference)):
        generated+='namespace '+name+' {\n'+prefix+'\n'.join(bodies)+'\n}\n'
    generated+='}\n';target=Path(scratch)/'resource_composition_bodies.inc';target.write_text(generated)
    return dict(owners=proof,source_sha256={name:hashlib.sha256(text.encode()).hexdigest()for name,text in snapshots.items()},
        generated_sha256=hashlib.sha256(generated.encode()).hexdigest(),
        imported_owner_subset=[f'{entry:x}'for entry in(0x12f040,0x12f0c0,0x13fd10,0x22a5c0,0x32ae70,0x32acd0,0x12fcb0,0x4057f0)],
        imported_owner_source_sha256=hashlib.sha256((ROOT/'src/physics/runtime/native_tu15_allocator_owner_dispatch.hpp').read_bytes()).hexdigest(),
        scope='Exact current bound algorithm copies, including genuine imported AI children; listed imported CPU entries invoke their actual qualified ABI bridges with the same admission. This test dispatcher does not execute the whole production imported-owner shim; other imports stop explicitly. Production cross-unit dispatcher routing and host services remain separately qualified.')

def composition_rows(image):
    """Backed cache-owner inputs plus declared nonzero caller resources.

    Page spans, budgets, backing and allocator rows come from the existing
    authenticated physical capture. Queue/animation descriptors are explicit
    test inputs; no page, job, allocation or child return is supplied.
    """
    import copy
    from validate_native_tu15_resource_maintenance import corpus,page_inputs
    rows,provenance=page_inputs(image,corpus())
    seeds=[row for row in rows if '-genuine-page-1000'in row['name']][:4]
    result=[]
    for ordinal,seed in enumerate(seeds):
        for parent in(0x1264e0,0x126200,0x1e5af0,0x1e5c80):
            row=copy.deepcopy(seed);index=len(result);row.update(id=index,name=f'backed-{parent:x}-{ordinal}',entry=parent,stop_before_entries=[])
            def put(at,value,width=4):
                choices=[bank for bank in row['banks']if bank['address']<=at<=bank['address']+len(bank['before'])//2-width]
                if len(choices)!=1:raise ValueError('Composition input escaped backing '+hex(at))
                bank=choices[0];raw=bytearray.fromhex(bank['before']);offset=at-bank['address'];raw[offset:offset+width]=(value&((1<<(width*8))-1)).to_bytes(width,'little');bank['before']=raw.hex()
            resource=0x61006000;pool=0x61000100;rows_address=0x61001000;bits=0x61002000
            extent=(1,511,513,4095)[ordinal];put(resource,0x12345678+index);put(resource+4,extent);put(resource+8,64+ordinal*4096);put(resource+12,0xffffffff);put(resource+16,1,1)
            put(0x4f0420,0x61008000);put(0x51df20,1,1);put(0x51df21,1,1)
            # Normalize the declared request queue to one owned request for
            # maintenance, or empty so the complete caller must allocate it.
            queued=parent==0x1264e0
            for offset,value in((0x20,16),(0x24,12),(0x34,1 if queued else 0),(0x38,1 if queued else 0),(0x3c,1 if queued else 0),(0x40,0x8000),(0x44,rows_address),(0x48,bits)):
                put(pool+offset,value)
            put(pool+0x29,1,1);put(bits,1 if queued else 0)
            for n in range(16):
                put(rows_address+n*12,0,2);put(rows_address+n*12+4,0);put(rows_address+n*12+8,0)
            if queued:put(rows_address,0x8000,2);put(rows_address+4,1 if ordinal%2==0 else 2);put(rows_address+8,resource)
            descriptor=0x6100b000;nodes=0x6100b100;slots=0x6100c000;objects=0x6100d000
            put(descriptor+0x30,nodes);put(descriptor+0xb0,resource);put(descriptor+0x4c,1);put(descriptor+0x50,slots);put(0x4f0414,objects)
            put(nodes+0x30,0xffff,2);put(nodes+0x32,0xffff,2);put(nodes+0x16,0x10,1);put(nodes+0x24,0)
            stack=row['gp'][4];put(stack,0x63000000);put(stack+4,0xffff);put(stack+8,0)
            if parent==0x126200:row['gp'][0]=resource;row['gp'][1]=(row['gp'][1]&0xffffff00)|(1 if ordinal%2==0 else 0)
            elif parent==0x1e5af0:row['gp'][0]=0;row['gp'][2]=descriptor
            elif parent==0x1e5c80:row['gp'][2]=descriptor
            row.update(matrix=0xffff,source=0,resource=resource,resource_extent=extent,file_offset=64+ordinal*4096,expected_io_jobs=1)
            result.append(row)
    expanded=[];live_values=('0000000000000080ff3f','00000000000000000080','35c26821a2da0fc90040','45230100000000c0ff7f')*2
    for row in result:
        for depth in(0,1,4,8):
            value=copy.deepcopy(row);value.update(id=len(expanded),name=row['name']+f'-live{depth}',live_x87=list(live_values[:depth]));expanded.append(value)
    provenance.update(scope='Unchanged captured page-owner state with declared nonzero queue/node/resource and caller FP inputs; every allocation, datum, job and read result is computed by its real owner.',live_x87_depths=[0,1,4,8])
    return expanded,provenance
