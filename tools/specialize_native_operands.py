#!/usr/bin/env python3
"""Fold literal executable operands using the canonical generated mapping.

Only a build-local copy changes. Dynamic/unmapped calls retain the original
checked function, and each replacement is recorded in a bounded receipt. The
original instruction order, state operations and service calls stay intact.
"""
import argparse
from collections import Counter
import hashlib
import importlib.util
import json
from pathlib import Path
import re

HEADER = 'std::uint32_t selected(std::uint32_t p,SourcePhysicsContactImage image)'
MAPPING = re.compile(re.escape(HEADER) +
    r'\{if\(image==SourcePhysicsContactImage::MapPack11\)return p;switch\(p\)\{'
    r'(?P<cases>(?:case 0x[0-9a-f]+u:return 0x[0-9a-f]+u;)+)'
    r'default:throw std::runtime_error\("[^"\n]*"\);}}')
CASE = re.compile(r'case (0x[0-9a-f]+)u:return (0x[0-9a-f]+)u;')
CALL = re.compile(r'\bselected\(\s*(0x[0-9a-f]+)u\s*,\s*'
                  r'([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)*)\s*\)')
INSTRUCTION = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')
NON_CODE = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
CURSOR_STATE_HEADERS = ('physics/runtime/physics_contact_runtime_internal.hpp', 'physics/runtime/physics_world_runtime_internal.hpp',
    'physics/vehicles/physics_vehicle_runtime_internal.hpp', 'physics/vehicles/physics_vehicle_motion_internal.hpp',
    'physics/vehicles/physics_vehicle_motion_math_internal.hpp')


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


# Native leaf kernels (src/physics/runtime/native_tu15_matrix_leaves.hpp),
# bound to the exact canonical bodies they replace (state type normalized to
# STATE). A changed or unknown body keeps its translated route.
LEAF_INCLUDE = '#include "physics/runtime/native_tu15_matrix_leaves.hpp"\n'
BSP_LEAF_INCLUDE = '#include "physics/runtime/native_tu15_bsp_leaves.hpp"\n'
BROADPHASE_LEAF_INCLUDE = '#include "physics/runtime/native_tu15_broadphase_leaves.hpp"\n'
PORTAL_LEAF_INCLUDE = '#include "physics/runtime/native_tu15_portal_predicate_leaves.hpp"\n'
BOUNDS_LEAF_INCLUDE = '#include "physics/runtime/native_tu15_broadphase_bounds_leaves.hpp"\n'
TEMPORARY_ABI_INCLUDE = '#include "physics/runtime/native_tu15_partition_temporary_abi.hpp"\n'
ARRAY_ABI_INCLUDE = '#include "physics/runtime/native_tu15_array_abi.hpp"\n'
BROADPHASE_REMOVE_INCLUDE = '#include "physics/runtime/native_tu15_broadphase_remove.hpp"\n'
BROADPHASE_REMOVE_PIN = '05e43e089a2d68b9de10cdd71ea033cb1942290cf70a61a3c645332e2ee13b28'
BROADPHASE_ADD_INCLUDE = '#include "physics/runtime/native_tu15_broadphase_add.hpp"\n'
BROADPHASE_INSERT_PIN = 'd6345ef66de6fb1b13280d965e7110b8c306f22ea61c60c06b5935d47afc6c2f'
BROADPHASE_ADDITION_INCLUDE = '#include "physics/runtime/native_tu15_broadphase_add_object.hpp"\n'
BROADPHASE_ADDITION_PIN = 'eefb2790b72700eef45d327ca0048a1efd2ab9bd5b97b1a89842d6979cb26bb3'
POOL_ALLOCATOR_INCLUDE = '#include "physics/runtime/native_tu15_pool_allocator_abi.hpp"\n'
POOL_RELEASE_PIN = '6db190f9f0c502059c3959d31c52f045182f642bb9adbe6e2ae231196408404c'
POOL_ALLOCATION_PIN = '798542b1fe41446e7eccb7618950a624dbff19a9612066611a03c37ce82f479b'
ARRAY_ALLOCATOR_INCLUDE = '#include "physics/runtime/native_tu15_array_allocator_abi.hpp"\n'
ARRAY_ALLOCATOR_ENTRIES = {
    '148a10': ('72002f4da21c2acb36286b36809b9f5dcf92a41bb3a7dcf1cd4c6b424fc69b68','allocate','149da0'),
    '148a70': ('cdef2db04a49c45c541f9aacd44839ec8036a532e87622bdbc75a6319ab7339a','release','149e00'),
}
STACK_ARENA_INCLUDE = '#include "physics/runtime/native_tu15_stack_arena_abi.hpp"\n'
STACK_ARENA_ENTRIES = {
    '2e10a0': ('c1ca294932e90476c71618e9c24faf2c912632c0a9e1ee6c29b7f3a5ddb9076a','allocate'),
    '2e1110': ('d377b74d4008703ea138d1604770181a53cbd262950bdd06223df60d41385afb','release'),
}
MAIN_HEAP_INCLUDE = '#include "physics/runtime/native_tu15_main_heap_abi.hpp"\n'
MAIN_HEAP_ENTRIES = {
    '22e380': ('3ab21efb89135cb4f33b43af186b40ad9dc68d389f2a7f0f6e60951649fa6657','remove_allocation','232c80'),
    '22e420': ('059a0daf55e7e819cbca46b43679e443d77cea578f2df0467ad287c123ba27c8','release','232d20'),
}
MAIN_HEAP_ALLOCATION_INCLUDE = '#include "physics/runtime/native_tu15_main_heap_allocate_abi.hpp"\n'
MAIN_HEAP_ALLOCATION_ENTRIES = {
    '22e1d0': ('f9daae5d1a669d1eaf5d47d741e7852cc31b0eaa8be298fb306bb76db2832730','allocate','232ad0'),
    '22e400': ('666a315b334fe888a6fdf9c53a517e1ae5aaf279fe02c9138aa0bad4c89e2200','allocate_sized','232d00'),
    '22e430': ('601dd12505fe3191bd7a875709276985589476bc516eba97acd3f4f4e9881edd','probe_gap','232d30'),
}
RAW_ALLOCATOR_INCLUDE = '#include "physics/runtime/native_tu15_raw_allocator_abi.hpp"\n'
RAW_ALLOCATOR_ENTRIES = {
    '149bf0': ('03ac6fd16ecb8c3503f31f371e46eb0783b8baae5dde715a209e26bb940ef6ae','allocate'),
    '149c40': ('5d1237c330a03699fde8f3bd593359ebd7e03c8cd3fb3d8b5ef877efd4bf9dba','release'),
    '233160': ('ca120647f371729b1b296b451f436d85262a7c8fa77bc7dc80ec53cf597641f1','allocate_tagged'),
    '2331a0': ('eec890a6d0fde139830e4b0533f52f66c10c5ddc79ae11edba4858b197277539','release_tagged'),
}
ALLOCATOR_OWNER_INCLUDE = '#include "physics/runtime/native_tu15_allocator_owner_dispatch.hpp"\n'
ALLOCATOR_OWNER_DISPATCH_PIN = '5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0'
# The fresh generator additionally routes the already recovered FA5D0 body.
# Its exact removal must restore the original complete dispatcher. This is
# routing authentication; the seven-instruction FA5D0 owner stays translated.
ALLOCATOR_OWNER_FRESH_DISPATCH_PIN = 'aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849'
ALLOCATOR_OWNER_FRESH_CASE = 'case 0xfa5d0u:run_fa5d0(s);return;'
ALLOCATOR_OWNER_FRESH_BODY_PIN = 'caa1c7398c487d028f00ed11ed4a20a15937a234e7b424a4ea15406150796346'
# Original TU1.5 bytes for actual owner entries lacking a generated body.
# The independent physical validator authenticates each whole byte sequence.
ALLOCATOR_ACTUAL_OWNER_BYTES = {
    0x232aa0:'8b5424088b01528b542408526a01ff5008c20800',
    0x232ac0:'8b01ff600c',0x2e1030:'e92ba5eeff',
}
TICK_CONVERSION_ACTUAL_OWNER_IMAGES = {
    0x22a5c0:(100,'32d5a2d8e508ef41dc6e366efaf26e8aab00aa9f10156a9c5f4aad550e5a9830'),
    0x32ae70:(170,'8964dcf0d769cb61dd4354feac00491ae8c1b34eb9627a4c5ec2d4350243d395'),
    0x32acd0:(52,'3d0898d998acc8af3ea17032ff56fd1b963c685b072f669c52d4b8267f9d0a78'),
}
PHYSICAL_EPOCH_ACTUAL_OWNER_IMAGES = {
    0x12f040:(122,'ab1d322d207234f005c2eb22e9f29da5aa35e7d0990dc92e0eb03da160d3889c'),
    0x12f0c0:(73,'a60b8f3171bddd5813033c2adbd04502bae86a9b4bb2556372a35162918fd8fb'),
    0x13fd10:(120,'f6eee0fd7c7a53a33a058ab7903d7ef6b713e896f7d570f5f7e5a93012bbea29'),
}
BSP_SURFACE_2D_ACTUAL_OWNER_IMAGES = {
    0x2a0230:(94,'c362b64b0f3c686f7f63e8e15b269d313d89019acf82bcb33f6c78661f2cfa07'),
}
BSP_SURFACE_POLYGON_ACTUAL_OWNER_IMAGES = {
    0x1e6e80:(303,'bc6e0eca62f0fef7c1384acf728217722636e0fa37827dd294461b2568e890ed'),
}
DATUM_DELETE_ACTUAL_OWNER_IMAGES = {
    0x16e8a0:(146,'23b798b8683109fa7356d9ad3286103f8a10c6b359d4913263b3565af7392996'),
}
STACK_PROBE_ACTUAL_OWNER_IMAGES = {
    0x32a660:(65,'b77ad187f6f3db2a5ae4f8e214b023d7c62ce25a38ce769a993d2a8c81643760'),
}
RESOURCE_BUSY_ACTUAL_OWNER_IMAGES = {
    0x12fcb0:(83,'1df96e347163f4151ccb82bf996d1aea2abae0e06e9c425b1525e2b450b4861a'),
    0x4057f0:(130,'26b21d4d8c3d411324b2a5c9f96dff30ef8103fe860c412947cdd57bf6cd6e8b'),
}
OBJECT_PREDICTION_COPY_ACTUAL_OWNER_IMAGES = {
    0xadcd0:(27,'514e40a41f0f14fb43d196e57373d3444e54db20f032faa895eb6621c1494a2c'),
}
ARRAY_ABI_ENTRIES = {
    '2df710': ({'db5cb2be041e3c5e58e52ad598ab7dd1ec0661ed1d8507123bb0263b4e8861f0',
        '5193583aca1257afd2c4dfb91f2e4c53942a25081221bdccaeb8767e9a5f87cf'}, 'reserve', '2e31e0'),
    '2df780': ({'d6ecec0126c79e4b734918c7642be8218cea4e6b28292d75eca7b28261e61c1b',
        '6bc91e7a2f8a56f78f976cf2b6b69ee860aaab014c3cc5ab2c3b0e42cfecdbfc'}, 'grow', '2e3250'),
    '2e3250': ({'776370c7b81df39130c8d0d8595682fddeea1a2426bb7f544bc2a89d5c431a42'}, 'grow', '2e3250'),
    '2df360': ({'108d18b60e51fa6a6908ce08a1bf9e7b4fb87001b7347c6baf19ee7289481b92',
        '8ad8ca8252bd5ca0a5dbc6c45eb31e8e34ac7d696deb1d735700e8e2ae3585f9'}, 'copy', '2e2e30'),
    '2e2e30': ({'8b7462e723269692792bdc0754da9d9512474204c2eb2335ce34f6f6951d19c3'}, 'copy', '2e2e30'),
}
TEMPORARY_ABI_ENTRIES = {
    '2e7da0': ('4adb3703ae6017c570f5c9070e773390b74d2a037666d64eec5a352a62fed5f2', 'construct', '2eba20'),
    '2e8090': ('d38beefa42c7df62021c16fea3fab90b62ad0ee2d0947897c0abad3315d7ae3e', 'destroy', '2eb850'),
}
LEAVES = {
    '145560': ('3ce39c1046f43fec27fe7f65c35755ca56024dcdae65bdebd4ec4e2377171b62', 'kernel_145560', 0x14566e),
    '144cf0': ('c6d3ad70a57319f692018045a3de73c341b85bf996fbd1330fcd7e9afc1fc032', 'kernel_144cf0', 0x144e4e),
    '2e44d0': ('9f79312f94c4df9caf23dd8e6c012cefe64749e9f7548b557d7375d8618919f1', 'kernel_2e44d0', 0x2e4540),
    '2e46e0': ('198a8866775ff17db3d72b629e04d42e4b9772cddbe9e79257d986711ee667c9', 'kernel_2e46e0', 0x2e4753),
    '14cdb0': ('f747aad6a36fcee70a3af5ab504d7fa8d9027e392c2175bee747f0bbb2752e30', 'kernel_14cdb0', 0),
    '14ce30': ('e88ee3cb90b6c51bf61c5eeed47db65740c8eef039f31ae55adb65a05f55d069', 'kernel_14ce30', 0),
    '2e9df0': ('6d0173a5660575e2d10e605580b31fa4c5eaca12deb0f83607575e41b80dd3f7', 'kernel_2e9df0', 0x2e9ee6),
    '2e6240': ('2f8cc6c161f3f2f220916571455d2f5b57a11488928c48298d61b7c58ea086c0', 'kernel_2e9cf0', 0x2e9de6),
    '2e9fd0': ('19cb3e6987d8e8dab83d61963c5faf298ebf3839676d0a899d99e6079f3eb972', 'kernel_2e9fd0', 0x2ea020),
    '14cea0': (('8f35497269f2009c9b3c3d453e4b606b2d3abc8b09e573544fed5a69ebb628aa', 'f7ef481b3620719039ba8356992ba31ce37a8e980e418542ee0b65c9d74e9c74'), 'kernel_14cea0', 0),
    '123210': (('8728c0405f5059e10d21fbbba7ccdee7494e1cd16f0c272f2884a3357c0b43ae', 'be69075d1612d6662186c8845da8834e82af5919cf744361842417c378754ebb'), 'kernel_123210', 0),
    '243450': (('a0308dda34f7a7f07103c6863266b0ba6a263b9d73f4e6d6f140226a700bb659', 'c966df7ae118b3c92387ceadfb02d3368e01f0189cba0f89e0fa23387063266a'), 'kernel_243450', 0),
    '2ea050': ('b60d187abad84cd9544150fb40183325fa20cbd6e3af70d32e6883d159ded7c2', 'kernel_2ea050', 0x2ea146),
    '2eb0c0': ('46e52cdff199e798899b7fb86f9c83e8ff00c93a0819efc0546fb061d1e2470a', 'kernel_2eb0c0', 0x2ea146),
}


def leaf_body(text, name):
    return re.search(r'static void run_' + name + r'\((\w+) &s\) \{.*?\n\}', text, re.S)


def page_window_bindings(text):
    current=leaf_body(text,'13fe00')
    if not current or 'if(halo2::native_page_window_detail::run(s))return;'not in current[0]:return[]
    # This module is also loaded directly with importlib by test and build
    # callers that do not put tools on sys.path. Resolve its optional sibling
    # against this owning file without changing the caller's import search.
    spec=importlib.util.spec_from_file_location('native_page_window_bindings',Path(__file__).with_name('native_page_window_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper.installed_entries(text)


def page_retire_bindings(text):
    current=leaf_body(text,'140310')
    if not current or 'if(halo2::native_page_retire_detail::run(s,'not in current[0]:return[]
    spec=importlib.util.spec_from_file_location('native_page_retire_bindings',Path(__file__).with_name('native_page_retire_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper.installed_entries(text)


def page_allocate_bindings(text):
    current=leaf_body(text,'13fe50')
    if not current or 'if(halo2::native_page_allocate_detail::run(s,'not in current[0]:return[]
    spec=importlib.util.spec_from_file_location('native_page_allocate_bindings',Path(__file__).with_name('native_page_allocate_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper.installed_entries(text)


def stack_probe_bindings(text):
    if 'if(halo2::native_stack_probe_detail::run(s'not in text:return[]
    spec=importlib.util.spec_from_file_location('native_stack_probe_bindings',Path(__file__).with_name('native_stack_probe_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper.installed_entries(text)


def resource_busy_bindings(text):
    if not any(current and 'if(halo2::native_resource_busy_detail::'in current[0]for current in(leaf_body(text,'12fcb0'),leaf_body(text,'4057f0'))):return[]
    spec=importlib.util.spec_from_file_location('native_resource_busy_bindings',Path(__file__).with_name('native_resource_busy_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper.installed_entries(text)


def halfword_bit_query_helper():
    spec=importlib.util.spec_from_file_location('native_halfword_bit_query_bindings',Path(__file__).with_name('native_halfword_bit_query_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    return helper


def object_node_bindings(text):
    if 'if(halo2::native_object_node_rebuild_detail::bridge(s,'not in text and 'if(halo2::native_object_animation_detail::bridge(s,'not in text:return([],[])
    spec=importlib.util.spec_from_file_location('native_object_node_bindings',Path(__file__).with_name('native_object_node_rebuild_replacements.py'))
    helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper)
    installed=helper.installed_entries(text)
    return([entry for entry in installed if entry=='bf0c0'],[entry for entry in installed if entry!='bf0c0'])


def install_halfword_bit_query(text,output):
    original=leaf_body(text,'fa5d0')
    if not original or original[1]!='WorldState':return output,[]
    helper=halfword_bit_query_helper();canonical=helper.canonical_body(original[0])
    # A second specialization may already have the exact cursor primitive.
    # Restore only that emitted statement shape before the whole-body pin.
    canonical=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',
        r'if(s.services.instruction){s.services.instruction(\1);}',canonical)
    if digest(canonical)!=helper.PIN:raise ValueError('Complete FA5D0 specialization body drift')
    current=leaf_body(output,'fa5d0');body=current[0];start=body.index('{')+1
    if not body[start:].startswith(helper.PREFIX):body=body[:start]+helper.PREFIX+body[start:]
    body=body.replace(helper.ORIGINAL_SHIFT,helper.ORDERED_SHIFT)
    output=output[:current.start()]+body+output[current.end():]
    if helper.INCLUDE not in output:output=helper.INCLUDE+output
    return output,['fa5d0']


def install_leaves(canonical, output):
    installed = []
    for name, (expected, kernel, final_pc) in LEAVES.items():
        original = leaf_body(canonical, name)
        if not original:
            continue
        state = original[1]
        if digest(original[0].replace(state + ' &s', 'STATE &s')) not in (expected if isinstance(expected, tuple) else (expected,)):
            continue
        if name in ('14cea0', '123210', '243450'):
            mapping=MAPPING.search(canonical)
            addresses={int(a,16):int(b,16) for a,b in CASE.findall(mapping['cases'])}
            if any(addresses.get(int(call[1],16))!=int(call[1],16) for call in CALL.finditer(original[0])):
                continue
        current = leaf_body(output, name)
        if not current:
            continue
        if name in ('2e44d0', '2e46e0'):
            # The recovered SSE destination owns dual-NaN payload selection.
            # Pin it in the translated reference as well as the native kernel;
            # callback removal otherwise lets the compiler swap operands.
            body = current[0].replace('_mm_add_ps(', 'halo2::native_tu15_leaves::ordered_add_ps(')
            body = body.replace('_mm_mul_ps(', 'halo2::native_tu15_leaves::ordered_mul_ps(')
            output = output[:current.start()] + body + output[current.end():]
            current = leaf_body(output, name)
        if name in ('14cea0', '243450'):
            body = current[0].replace('_mm_add_ss(', 'halo2::native_tu15_leaves::ordered_add_ss(')
            body = body.replace('_mm_mul_ss(', 'halo2::native_tu15_leaves::ordered_mul_ss(')
            if name=='14cea0':
                body=body.replace('u(spill(s.fp.st(0))));s.fp.pop();',
                    'halo2::native_tu15_leaves::portal_spill_word(s.fp.st(0),s.fp.status));s.fp.pop();')
            output = output[:current.start()] + body + output[current.end():]
            current = leaf_body(output, name)
        if name=='2ea050':
            body=current[0].replace('u(spill(s.fp.st(0)))',
                'halo2::native_tu15_leaves::portal_spill_word(s.fp.st(0),s.fp.status)')
            output=output[:current.start()]+body+output[current.end():]
            current=leaf_body(output,name)
        if name in ('2e44d0','2e46e0','2ea050'):
            body=current[0].replace('vw(s.memory,','halo2::native_tu15_leaves::aligned_vw(s.memory,')
            body=body.replace('v(s.memory,','halo2::native_tu15_leaves::aligned_v(s.memory,')
            output=output[:current.start()]+body+output[current.end():]
            current=leaf_body(output,name)
        # An admission prefix inside the original body, as the normalize
        # bridge: the partitioner owns only hex-named bodies with the
        # canonical trailer, and the original instructions stay intact.
        start = current[0].index('{\n') + 2
        admission='s.selected_observation_pcs && ' if name=='2e6240' else ''
        marker = (f'    // BEGIN native leaf{name}\n'
                  f'    if({admission}halo2::native_tu15_leaves::run(s,[](auto &io,auto &m){{halo2::native_tu15_leaves::{kernel}(io,m);}},\n'
                  f'        [](auto &r){{run_{name}(r);}},0x{final_pc:x}u))return;\n'
                  f'    // END native leaf{name}\n')
        body = current[0][:start] + marker + current[0][start:]
        output = output[:current.start()] + body + output[current.end():]
        installed.append(name)
    if installed:
        output = LEAF_INCLUDE + output
        if any(name in installed for name in ('14cdb0', '14ce30')):
            output = BSP_LEAF_INCLUDE + output
        if any(name in installed for name in ('2e6240', '2e9df0', '2e9fd0')):
            output = BROADPHASE_LEAF_INCLUDE + output
        if any(name in installed for name in ('14cea0', '123210', '243450')):
            output = PORTAL_LEAF_INCLUDE + output
        if any(name in installed for name in ('2ea050','2eb0c0')):
            output = BOUNDS_LEAF_INCLUDE + output
    return output, installed


def install_temporary_abi(canonical,output):
    installed=[]
    bindings={0x37a5f8:0x37e0a8,0x487638:0x48c938,0x2e7df1:0x2eba71,
        0x2e80e0:0x2eb8a0,0x2e811a:0x2eb8da}
    mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(canonical)['cases'])}
    for name,(pin,operation,actual) in TEMPORARY_ABI_ENTRIES.items():
        original=leaf_body(canonical,name)
        if not original or original[1]!='ObservedWorldState':
            continue
        if digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:
            continue
        if any(mapping.get(int(call[1],16))!=bindings.get(int(call[1],16)) for call in CALL.finditer(original[0])):
            continue
        current=leaf_body(output,name)
        start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_partition_temporary_detail::{operation}(s,\n'
            '        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():]
        installed.append(actual)
    if installed:output=TEMPORARY_ABI_INCLUDE+output
    return output,installed


def install_array_abi(canonical,output):
    installed=[]
    bindings={0x487638:0x48c938,0x2df72f:0x2e31ff,0x2df745:0x2e3215,
        0x2df768:0x2e3238,0x2df7ae:0x2e327e,0x2df7c0:0x2e3290,0x2df7e3:0x2e32b3}
    mapping={int(a,16):int(b,16) for a,b in CASE.findall(MAPPING.search(canonical)['cases'])}
    for name,(pins,operation,actual) in ARRAY_ABI_ENTRIES.items():
        original=leaf_body(canonical,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s')) not in pins:
            continue
        if any(mapping.get(int(call[1],16))!=bindings.get(int(call[1],16),int(call[1],16)) for call in CALL.finditer(original[0])):
            continue
        actual_pcs='s.selected_observation_pcs' if original[1]=='ObservedWorldState' else ('true' if name.startswith('2e') else 'false')
        if operation=='copy':prefix=f'    if(halo2::native_array_abi_detail::copy(s,{actual_pcs}))return;\n'
        else:
            reserve='true' if operation=='reserve' else 'false'
            call_site='site+0x3ad0u' if name.startswith('2e') else 'site'
            prefix=('    if(halo2::native_array_abi_detail::run(s,\n'
                f'        [](auto &v,std::uint32_t target,std::uint32_t site){{dispatch(v,target,{call_site},false);}}, {reserve},{actual_pcs}))return;\n')
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];installed.append(actual)
    if installed:output=ARRAY_ABI_INCLUDE+output
    return output,installed


def specialize(text):
    mopp_aabb_entries=[]
    if 'static void run_305e40(ObservedWorldState &s)' in text:
        from native_mopp_aabb_replacements import installed_entries as mopp_aabb_bindings
        mopp_aabb_entries=mopp_aabb_bindings(text)
    mopp_aabb_canonical_entries=[]
    if 'static void run_305e40(State &s)' in text:
        from native_mopp_aabb_canonical_replacements import installed_entries as canonical_mopp_bindings
        mopp_aabb_canonical_entries=canonical_mopp_bindings(text)
    # Preserve the older complete MOPP owner's authenticated rejection order.
    polygon_support_entries=[]
    if 'halo2::native_polygon_support_detail::bridge(s)' in text or '#include "physics/runtime/native_tu15_polygon_support_frame.hpp"' in text:
        import native_polygon_support_replacements as polygon_support_owner
        from native_polygon_prepared_owners import installed as installed_polygon_owner
        installed_polygon_owner(text,polygon_support_owner,'support')
        polygon_support_entries=['1f6610']
    polygon_feature_entries=[]
    if 'halo2::native_polygon_features_detail::bridge(s)' in text or '#include "physics/runtime/native_tu15_polygon_features_abi.hpp"' in text:
        feature_spec=importlib.util.spec_from_file_location('native_polygon_feature_binding',Path(__file__).with_name('native_polygon_features_replacements.py'))
        feature_helper=importlib.util.module_from_spec(feature_spec);feature_spec.loader.exec_module(feature_helper)
        from native_polygon_prepared_owners import installed as installed_polygon_owner
        installed_polygon_owner(text,feature_helper,'features')
        polygon_feature_entries=['1f6690']
    mapping = MAPPING.search(text)
    if not mapping or text.count(HEADER) != 1:
        raise ValueError('Unsupported canonical native operand mapping')
    pairs = [(int(a, 16), int(b, 16)) for a, b in CASE.findall(mapping['cases'])]
    addresses = dict(pairs)
    if len(addresses) != len(pairs):
        raise ValueError('Duplicate canonical native operand mapping')
    masked = NON_CODE.sub(lambda match: ' ' * len(match[0]), text)
    result, cursor, unknown = [], 0, 0
    counts = Counter()
    for call in CALL.finditer(masked):
        literal = int(call[1], 16)
        if literal not in addresses:
            unknown += 1
            continue
        translated = addresses[literal]
        replacement = (f'0x{literal:x}u' if literal == translated else
                       f'({call[2]}==SourcePhysicsContactImage::MapPack11?'
                       f'0x{literal:x}u:0x{translated:x}u)')
        result.extend((text[cursor:call.start()], replacement))
        cursor = call.end()
        counts[(literal, translated)] += 1
    result.append(text[cursor:])
    output = ''.join(result)
    cursor_calls = 0
    if any(f'#include "{header}"' in text for header in CURSOR_STATE_HEADERS):
        # These canonical states implement/inherit the same cursor contract.
        # Other generated modules keep their existing callback expression.
        masked = NON_CODE.sub(lambda match: ' ' * len(match[0]), output)
        pieces, cursor = [], 0
        for call in INSTRUCTION.finditer(masked):
            pieces.extend((output[cursor:call.start()], f's.instruction_literal({call[1]});'))
            cursor = call.end()
            cursor_calls += 1
        pieces.append(output[cursor:])
        output = ''.join(pieces)
    leaves = []
    if cursor_calls or '#include "physics/runtime/physics_world_runtime_internal.hpp"' in text:
        output, leaves = install_leaves(text, output)
    output,temporary_entries=install_temporary_abi(text,output)
    output,array_entries=install_array_abi(text,output)
    main_heap_entries=[]
    for name,(pin,operation,actual) in MAIN_HEAP_ENTRIES.items():
        original=leaf_body(text,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:continue
        if not all(addresses.get(int(m[1],16))=={0x22e42a:0x232d2a}.get(int(m[1],16)) for m in CALL.finditer(original[0])):continue
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_main_heap_detail::{operation}(s'+
            (',\n        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}' if operation=='release' else '')+'))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];main_heap_entries.append(actual)
    if main_heap_entries:output=MAIN_HEAP_INCLUDE+output
    heap_allocation_entries=[]
    heap_allocation_addresses={0x22e253:0x232b53,0x22e367:0x232c67,0x22e35c:0x232c5c,
        0x22e411:0x232d11,0x22e47b:0x232d7b,0x22e470:0x232d70}
    for name,(pin,operation,actual) in MAIN_HEAP_ALLOCATION_ENTRIES.items():
        original=leaf_body(text,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:continue
        if not all(addresses.get(int(m[1],16))==heap_allocation_addresses.get(int(m[1],16)) for m in CALL.finditer(original[0])):continue
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_main_heap_allocation_detail::{operation}(s'+
            (',\n        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}' if operation!='probe_gap' else '')+'))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];heap_allocation_entries.append(actual)
    if heap_allocation_entries:output=MAIN_HEAP_ALLOCATION_INCLUDE+output
    raw_entries=[]
    for name,(pin,operation) in RAW_ALLOCATOR_ENTRIES.items():
        original=leaf_body(text,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:continue
        if any(CALL.finditer(original[0])):continue
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_raw_allocator_detail::{operation}(s,\n'
            '        [](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];raw_entries.append(name)
    if raw_entries:output=RAW_ALLOCATOR_INCLUDE+output
    allocator_owner_entries=[];tick_conversion_entries=[];physical_epoch_entries=[];bsp_surface_2d_entries=[];bsp_polygon_entries=[];datum_delete_entries=[];stack_probe_entries=[];resource_busy_entries=[];object_prediction_copy_entries=[]
    dispatch_pattern=r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}'
    original_dispatch=re.search(dispatch_pattern,text)
    fresh_dispatch=False
    if original_dispatch and digest(original_dispatch[0])==ALLOCATOR_OWNER_FRESH_DISPATCH_PIN:
        extra=leaf_body(text,'fa5d0')
        fresh_dispatch=(original_dispatch[0].count(ALLOCATOR_OWNER_FRESH_CASE)==1 and
            digest(original_dispatch[0].replace(ALLOCATOR_OWNER_FRESH_CASE,'',1))==ALLOCATOR_OWNER_DISPATCH_PIN and
            extra is not None and digest(halfword_bit_query_helper().canonical_body(extra[0]))==ALLOCATOR_OWNER_FRESH_BODY_PIN)
    if original_dispatch and(digest(original_dispatch[0])==ALLOCATOR_OWNER_DISPATCH_PIN or fresh_dispatch):
        current_dispatch=re.search(dispatch_pattern,output);body=current_dispatch[0]
        fallback='default:s.required(site,target,tail);return;}}'
        if body.count(fallback)!=1:raise ValueError('Authenticated native allocator dispatch fallback changed')
        adapter=('if(halo2::native_allocator_owner_detail::dispatch(s,target,'
            '[](auto &v,std::uint32_t owner,std::uint32_t call_site,bool is_tail){dispatch(v,owner,call_site,is_tail);}))return;')
        body=body.replace(fallback,'default:'+adapter+'s.required(site,target,tail);return;}}')
        output=ALLOCATOR_OWNER_INCLUDE+output[:current_dispatch.start()]+body+output[current_dispatch.end():]
        allocator_owner_entries=[f'{owner:x}' for owner in ALLOCATOR_ACTUAL_OWNER_BYTES]
        tick_conversion_entries=[f'{owner:x}' for owner in TICK_CONVERSION_ACTUAL_OWNER_IMAGES]
        physical_epoch_entries=[f'{owner:x}' for owner in PHYSICAL_EPOCH_ACTUAL_OWNER_IMAGES]
        bsp_surface_2d_entries=[f'{owner:x}' for owner in BSP_SURFACE_2D_ACTUAL_OWNER_IMAGES]
        bsp_polygon_entries=[f'{owner:x}' for owner in BSP_SURFACE_POLYGON_ACTUAL_OWNER_IMAGES]
        datum_delete_entries=[f'{owner:x}' for owner in DATUM_DELETE_ACTUAL_OWNER_IMAGES]
        stack_probe_entries=[f'{owner:x}' for owner in STACK_PROBE_ACTUAL_OWNER_IMAGES]
        resource_busy_entries=[f'{owner:x}' for owner in RESOURCE_BUSY_ACTUAL_OWNER_IMAGES]
        object_prediction_copy_entries=[f'{owner:x}' for owner in OBJECT_PREDICTION_COPY_ACTUAL_OWNER_IMAGES]
    arena_entries=[]
    for name,(pin,operation) in STACK_ARENA_ENTRIES.items():
        original=leaf_body(text,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:continue
        if not all(addresses.get(int(m[1],16))==int(m[1],16) for m in CALL.finditer(original[0])):continue
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_stack_arena_detail::{operation}(s,\n'
            '        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];arena_entries.append(name)
    if arena_entries:output=STACK_ARENA_INCLUDE+output
    allocator_entries=[]
    allocator_bindings={0x480d98:0x486064,0x480d9c:0x486068,0x480da0:0x48606c,0x480da4:0x486070,
        0x148a2f:0x149dbf,0x148a40:0x149dd0,0x1485a0:0x149930,0x148a49:0x149dd9,
        0x148a52:0x149de2,0x148aa5:0x149e35,0x148adb:0x149e6b,0x148aed:0x149e7d,
        0xf0340:0xf0340,0x15030:0x15030}
    for name,(pin,operation,actual) in ARRAY_ALLOCATOR_ENTRIES.items():
        original=leaf_body(text,name)
        if not original or digest(original[0].replace(original[1]+' &s','STATE &s'))!=pin:continue
        if not all(addresses.get(int(m[1],16))==allocator_bindings.get(int(m[1],16)) for m in CALL.finditer(original[0])):continue
        current=leaf_body(output,name);start=current[0].index('{\n')+2
        prefix=(f'    if(halo2::native_array_allocator_detail::{operation}(s,\n'
            '        [](auto &v,std::uint32_t target,std::uint32_t site,bool tail){dispatch(v,target,site,tail);}))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=output[:current.start()]+body+output[current.end():];allocator_entries.append(actual)
    if allocator_entries:output=ARRAY_ALLOCATOR_INCLUDE+output
    pool_entries=[]
    original=leaf_body(text,'22eb20')
    if original and digest(original[0].replace(original[1]+' &s','STATE &s'))==POOL_RELEASE_PIN:
        bindings={0xf0388:0xf0388,0x46927c:0x46e33c,0x22eb83:0x233483}
        if all(addresses.get(int(m[1],16))==bindings.get(int(m[1],16)) for m in CALL.finditer(original[0])):
            current=leaf_body(output,'22eb20');start=current[0].index('{\n')+2
            prefix=('    if(halo2::native_pool_allocator_detail::release(s,\n'
                '        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);}))return;\n')
            body=current[0][:start]+prefix+current[0][start:]
            output=POOL_ALLOCATOR_INCLUDE+output[:current.start()]+body+output[current.end():];pool_entries=['233420']
    original=leaf_body(text,'22e950')
    if original and digest(original[0].replace(original[1]+' &s','STATE &s'))==POOL_ALLOCATION_PIN:
        if all(addresses.get(int(m[1],16))==int(m[1],16) for m in CALL.finditer(original[0])):
            current=leaf_body(output,'22e950');start=current[0].index('{\n')+2
            prefix='    if(halo2::native_pool_allocator_detail::allocate(s))return;\n'
            body=current[0][:start]+prefix+current[0][start:]
            output=output[:current.start()]+body+output[current.end():]
            if POOL_ALLOCATOR_INCLUDE not in output:output=POOL_ALLOCATOR_INCLUDE+output
            pool_entries.append('233250')
    addition_entries=[]
    original=leaf_body(text,'2e8a00')
    if original and digest(original[0].replace(original[1]+' &s','STATE &s'))==BROADPHASE_ADDITION_PIN:
        bindings={0x37a6b8:0x37e168,0x418c20:0x41c6a0,0x418c10:0x41c690,0x418c00:0x41c680,
            0x2e8b1e:0x2ec5ce,0x2e8b5b:0x2ec60b,0x2e8b78:0x2ec628,0x2e8b98:0x2ec648,
            0x2e8ba6:0x2ec656,0x2e8bc7:0x2ec677,0x2e8e37:0x2ec8e7,0x2e8e77:0x2ec927,0x2e8ede:0x2ec98e}
        if all(addresses.get(int(m[1],16))==bindings.get(int(m[1],16)) for m in CALL.finditer(original[0])):
            current=leaf_body(output,'2e8a00')
            body=current[0].replace('_mm_add_ps(','halo2::native_tu15_leaves::ordered_add_ps(')
            body=body.replace('_mm_mul_ps(','halo2::native_tu15_leaves::ordered_mul_ps(')
            body=body.replace('vw(s.memory,','halo2::native_tu15_leaves::aligned_vw(s.memory,')
            body=body.replace('v(s.memory,','halo2::native_tu15_leaves::aligned_v(s.memory,')
            output=output[:current.start()]+body+output[current.end():];current=leaf_body(output,'2e8a00');start=current[0].index('{\n')+2
            prefix=('    if(halo2::native_broadphase_add_detail::add_object(s,\n'
                '        [](auto &v){run_2df780(v);},[](auto &v){run_2e7500(v);},[](auto &v){run_2e6240(v);},\n'
                '        [](auto &v){run_2e7da0(v);},[](auto &v){run_2e8090(v);}))return;\n')
            body=current[0][:start]+prefix+current[0][start:]
            output=BROADPHASE_ADDITION_INCLUDE+output[:current.start()]+body+output[current.end():];addition_entries=['2ec4b0']
    insertion_entries=[]
    original=leaf_body(text,'2e7500')
    if original and digest(original[0].replace(original[1]+' &s','STATE &s'))==BROADPHASE_INSERT_PIN:
        if addresses.get(0x2e7527)==0x2eafd7:
            current=leaf_body(output,'2e7500');start=current[0].index('{\n')+2
            prefix=('    if(halo2::native_broadphase_add_detail::insert_edges(s,[](auto &v){run_2df710(v);}))return;\n')
            body=current[0][:start]+prefix+current[0][start:]
            output=BROADPHASE_ADD_INCLUDE+output[:current.start()]+body+output[current.end():];insertion_entries=['2eafb0']
    removal_entries=[]
    original=leaf_body(text,'2ec9b0')
    if original and digest(original[0].replace(original[1]+' &s','STATE &s'))==BROADPHASE_REMOVE_PIN:
        current=leaf_body(output,'2ec9b0')
        aligned=current[0].replace('vw(s.memory,','halo2::native_tu15_leaves::aligned_vw(s.memory,')
        output=output[:current.start()]+aligned+output[current.end():]
        current=leaf_body(output,'2ec9b0');start=current[0].index('{\n')+2
        prefix=('    if(halo2::native_broadphase_remove_detail::run(s,\n'
            '        [](auto &v,std::uint32_t target,std::uint32_t site){dispatch(v,target,site,false);},\n'
            '        [](auto &v){run_2e9fd0(v);},[](auto &v){run_2e9df0(v);}))return;\n')
        body=current[0][:start]+prefix+current[0][start:]
        output=BROADPHASE_REMOVE_INCLUDE+output[:current.start()]+body+output[current.end():]
        removal_entries=['2ec9b0']
    output,halfword_bit_query_entries=install_halfword_bit_query(text,output)
    receipt = dict(schema='native-operand-specialization-v1',
        canonical_sha256=digest(text), specialized_sha256=digest(output),
        mapping_sha256=digest(mapping[0]), folded_calls=sum(counts.values()),
        unmapped_literal_calls=unknown,
        instruction_cursor_calls=cursor_calls,
        native_leaves=leaves,
        native_temporary_abi_entries=temporary_entries,
        native_array_abi_entries=array_entries,
        native_broadphase_removal_entries=removal_entries,
        native_broadphase_insertion_entries=insertion_entries,
        native_broadphase_addition_entries=addition_entries,
        native_pool_allocator_entries=pool_entries,
        native_array_allocator_entries=allocator_entries,
        native_stack_arena_entries=arena_entries,
        native_main_heap_entries=main_heap_entries,
        native_main_heap_allocation_entries=heap_allocation_entries,
        native_raw_allocator_entries=raw_entries,
        native_halfword_bit_query_entries=halfword_bit_query_entries,
        native_allocator_actual_owner_entries=allocator_owner_entries,
        native_tick_conversion_actual_owner_entries=tick_conversion_entries,
        native_physical_epoch_actual_owner_entries=physical_epoch_entries,
        native_bsp_surface_2d_actual_owner_entries=bsp_surface_2d_entries,
        native_bsp_surface_polygon_actual_owner_entries=bsp_polygon_entries,
        native_datum_delete_actual_owner_entries=datum_delete_entries,
        native_stack_probe_actual_owner_entries=stack_probe_entries,
        native_resource_busy_actual_owner_entries=resource_busy_entries,
        native_object_prediction_copy_actual_owner_entries=object_prediction_copy_entries,
        native_actual_owner_dispatch_variant=('fresh-fa5d0'if fresh_dispatch else'canonical')if allocator_owner_entries else None,
        native_page_window_entries=page_window_bindings(text),
        native_page_retire_entries=page_retire_bindings(text),
        native_page_allocate_entries=page_allocate_bindings(text),
        native_stack_probe_entries=stack_probe_bindings(text),
        native_resource_busy_entries=resource_busy_bindings(text),
        native_mopp_aabb_entries=mopp_aabb_entries,
        native_mopp_aabb_canonical_entries=mopp_aabb_canonical_entries,
        native_polygon_support_entries=polygon_support_entries,
        native_polygon_feature_entries=polygon_feature_entries,
        bindings=[dict(canonical=a, tu15=b, uses=n) for (a, b), n in sorted(counts.items())])
    # Preserve the older owner-specific authenticated rejection order. New
    # object-node receipts follow the existing Page/Stack/Busy validations.
    object_node_entries,object_animation_entries=object_node_bindings(text)
    receipt.update(native_object_node_rebuild_entries=object_node_entries,
        native_object_animation_entries=object_animation_entries)
    return output, receipt


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error('Specialized operands must use a separate build-local output')
    output, receipt = specialize(args.source.read_text())
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(output)
    args.output.with_suffix('.operands.json').write_text(json.dumps(receipt, indent=2) + '\n')
    print(json.dumps({key: value for key, value in receipt.items() if key != 'bindings'}))


if __name__ == '__main__':
    main()
