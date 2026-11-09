"""Authenticate complete stack-probe bodies and their owning image resolver."""
import hashlib
import importlib.util
from pathlib import Path
import re

LITERAL_PIN='b512ebfeb86ca27a74fa2c187fd12d3c885487db9b947472e9d3644d131a5418'
SELECTED_PIN='1c6710f8390cbe6867523b59fbe445913f2544e01eba06d0eef487ae1be5e074'
VEHICLE_PIN='2b748c94c036f3af71578260a2d834cc87bd75aa7c30c6cad5946420419b704e'
SPECIALIZED_PIN='1f014c8e0b73b9160b5589fe1cf7f089723d9e45298714da25db46c8233471a5'
SPECIALIZED_VEHICLE_PIN='6fab68f335c56a555b27123ebfeb9c5d686b1cad53bd0b5875316be557e6bf32'
RESOLVER_PINS=('5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657',
    'dd420e389ace10e043973198d59268c875bebb8a0e7ed2d2f2d04ea2f4f46981',
    '99fe1f8662f1d46d270c5fe9567e4a3bc4978cf6d00dfeaeeea495f041232042')
VEHICLE_RESOLVER_PIN='8b24d8c367404f75f9c9dd0e077dfbd5498ab846a32a03ec32c06b461e417682'
INCLUDE='#include "physics/runtime/native_tu15_stack_probe_abi.hpp"\n'


def prefix(vehicle=False):
    return('    if(halo2::native_stack_probe_detail::run(s,halo2::native_page_window_detail::Noop{},0x3b70u))return;\n'if vehicle else
        '    if(halo2::native_stack_probe_detail::run(s))return;\n')


def canonical_body(value,vehicle=False):
    start=value.index('{\n')+2;binding=prefix(vehicle)
    return value[:start]+value[start+len(binding):]if value[start:].startswith(binding)else value


def authenticate(source,value,vehicle):
    pin=hashlib.sha256(value.encode()).hexdigest()
    if vehicle:
        if pin!=VEHICLE_PIN:raise ValueError('Complete mapped vehicle stack-probe body drift')
        resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
        if not resolver or hashlib.sha256(resolver[0].encode()).hexdigest()!=VEHICLE_RESOLVER_PIN:
            raise ValueError('Complete vehicle stack-probe image resolver drift')
        if 'case 0x326af0u:return 0x32a660u;'not in resolver[0]:raise ValueError('Mapped stack-probe owner changed')
    else:
        if pin not in(LITERAL_PIN,SELECTED_PIN):raise ValueError('Complete TU1.5 stack-probe body drift')
        spec=importlib.util.spec_from_file_location('native_stack_selected_operand_binding',Path(__file__).with_name('native_selected_operand_binding.py'))
        helper=importlib.util.module_from_spec(spec);spec.loader.exec_module(helper);helper.require_selected_identity(source,value,RESOLVER_PINS)


def installed_entries(source):
    entries=[]
    for entry,vehicle in(('32a660',False),('326af0',True)):
        match=re.search(r'static void run_'+entry+r'\(\w+ &s\) \{.*?\n\}',source,re.S)
        if not match:continue
        original=canonical_body(match[0],vehicle)
        if original==match[0]:continue
        specialized=SPECIALIZED_VEHICLE_PIN if vehicle else SPECIALIZED_PIN
        if hashlib.sha256(original.encode()).hexdigest()!=specialized:authenticate(source,original,vehicle)
        entries.append('32a660')
    return sorted(set(entries))


def install_native_stack_probe(source):
    for entry,vehicle in(('32a660',False),('326af0',True)):
        match=re.search(r'static void run_'+entry+r'\(\w+ &s\) \{.*?\n\}',source,re.S)
        if not match:continue
        original=canonical_body(match[0],vehicle);authenticate(source,original,vehicle)
        if original==match[0]:
            start=original.index('{\n')+2;source=source[:match.start()]+original[:start]+prefix(vehicle)+original[start:]+source[match.end():]
        if INCLUDE not in source:
            anchor=('#include "physics/vehicles/physics_vehicle_runtime_internal.hpp"\n'if vehicle else'#include "physics/runtime/physics_world_runtime_internal.hpp"\n')
            if source.count(anchor)!=1:raise ValueError('Missing stack-probe include seam')
            source=source.replace(anchor,anchor+INCLUDE,1)
    return source
