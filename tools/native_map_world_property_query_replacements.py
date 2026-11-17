"""Exact MapWorld attachment caller and property-query source binding."""
import hashlib
import re

ENTRIES=('11e490','1db800')
INCLUDE='#include "gameplay/objects/native_tu15_map_world_property_owners.hpp"\n'
ANCHOR='#include "halo2/source_effect_compiled_symbols_tu15.hpp"\n'
BRIDGE='halo2::native_map_world_property_owner_detail::bridge_'
OWNER_NAMESPACE='native_map_world_property_owner_detail'
CORE_NAMESPACE='native_map_world_property_query_detail'
INCLUDE_BASENAME='native_tu15_map_world_property_owners.hpp'
OLD_MUL='_mm_mul_ss(s.xmm[0],'
ORDERED_MUL='halo2::native_map_world_property_query_detail::ordered_multiply(s.xmm[0],'
PARTS=(
    r'^std::uint32_t selected\([^\n]+',
    r'^static void dispatch\([^\n]+',
    r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;',
    r'^void source_run_map_world_support_tu15\(.*?^\}',
    r'^bool source_map_world_support_tu15_owns_entry\([^\n]+',
)

def digest(value):return hashlib.sha256(value.encode()).hexdigest()

def one(text,pattern,name):
    values=re.findall(pattern,text,re.M|re.S)
    if len(values)!=1:raise ValueError('Missing or ambiguous MapWorld property '+name)
    return values[0]

def header(entry):return 'static void run_'+entry+'(WorldState &s) {'

def body(text,entry):
    return one(text,'^'+re.escape(header(entry))+r'.*?^\}',entry+' body')

def prefix(entry,literal):
    tag='true' if literal else 'false'
    if entry=='1db800':return '\n    if('+BRIDGE+'query<'+tag+'>(s))return;'
    if entry=='11e490':return ('\n    if('+BRIDGE+'caller<'+tag+'>(s,'
        '[](auto& child){run_1db800(child);}))return;')
    raise ValueError('Unknown MapWorld property owner')

def callback_format(text,entry):
    raw=re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}',text)
    literal=re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);',text)
    expected=30 if entry=='11e490' else 34
    if (len(raw),len(literal))==(expected,0):return False
    if (len(raw),len(literal))==(0,expected):return True
    raise ValueError('Mixed MapWorld property callback formats')

def entry_prefix(text,entry):
    found=[p for p in (prefix(entry,False),prefix(entry,True))if p in text]
    if not found and BRIDGE not in text:return ''
    if len(found)!=1 or text.count(found[0])!=1 or not text.startswith(header(entry)+found[0]):
        raise ValueError('Moved MapWorld property bridge')
    return found[0]

def canonical_body(text):
    entry=one(text,r'^static void run_(11e490|1db800)\(','target signature')
    overlay=entry_prefix(text,entry)
    if not overlay:return text
    raw=text.replace(overlay,'',1)
    if entry=='1db800':
        if raw.count(ORDERED_MUL)!=2 or OLD_MUL in raw:
            raise ValueError('MapWorld property fallback must retain both original MULSS operand orders')
        raw=raw.replace(ORDERED_MUL,OLD_MUL,2)
    return raw

def authenticate(text):
    if 'void source_run_map_world_support_tu15(' not in text:
        if any(x in text for x in (OWNER_NAMESPACE,CORE_NAMESPACE,INCLUDE_BASENAME)):raise ValueError('Missing MapWorld property owner')
        return None
    owners={entry:body(text,entry)for entry in ENTRIES}
    unbound={entry:canonical_body(value)for entry,value in owners.items()}
    formats={entry:callback_format(value,entry)for entry,value in unbound.items()}
    if len(set(formats.values()))!=1:raise ValueError('Mixed MapWorld property owner contexts')
    literal=formats[ENTRIES[0]]
    entries={entry:entry_prefix(value,entry)for entry,value in owners.items()}
    active=sum(bool(x)for x in entries.values())
    if active not in (0,2) or any(value and value!=prefix(entry,literal)for entry,value in entries.items()):
        raise ValueError('Partial or incorrectly tagged MapWorld property overlay')
    context=(literal,*(digest(one(text,pattern,'owner context'))for pattern in PARTS),
             *(digest(unbound[entry])for entry in ENTRIES))
    if context not in CONTEXTS:raise ValueError('Complete correlated MapWorld property owner changed')
    owned=int(bool(active))
    if (text.count(ANCHOR),text.count(INCLUDE),text.count(ANCHOR+INCLUDE),text.count(BRIDGE),
        text.count(ORDERED_MUL),text.count(OWNER_NAMESPACE),text.count(CORE_NAMESPACE),
        text.count(INCLUDE_BASENAME))!=(1,owned,owned,active,2*owned,active,2*owned,owned):
        raise ValueError('Moved, duplicated or external MapWorld property binding')
    return owners,unbound,literal,entries

def replace_map_world_property_queries(text):
    found=authenticate(text)
    if not found or found[3][ENTRIES[0]]:return text
    owners,unbound,literal,_=found
    for entry in ENTRIES:
        original=unbound[entry]
        if entry=='1db800':
            if original.count(OLD_MUL)!=2:raise ValueError('Missing both MapWorld property multiply sites')
            original=original.replace(OLD_MUL,ORDERED_MUL,2)
        opening=header(entry)
        text=text.replace(owners[entry],opening+prefix(entry,literal)+original[len(opening):],1)
    text=text.replace(ANCHOR,ANCHOR+INCLUDE,1)
    authenticate(text)
    return text

def remove_map_world_property_queries(text):
    found=authenticate(text)
    if not found or not found[3][ENTRIES[0]]:return text
    for entry in ENTRIES:text=text.replace(found[0][entry],found[1][entry],1)
    return text.replace(INCLUDE,'',1)

def installed_entries(text):
    found=authenticate(text)
    if not found or not found[3][ENTRIES[0]]:raise ValueError('Complete MapWorld property binding missing')
    return list(ENTRIES)

def retag_specialized(source,output):
    found=authenticate(source)
    if not found or not found[3][ENTRIES[0]]:return output
    for entry in ENTRIES:
        value=body(output,entry)
        old=entry_prefix(value,entry)
        if old!=found[3][entry]:raise ValueError('Specializer moved MapWorld property prefix')
        output=output.replace(value,value.replace(old,prefix(entry,callback_format(value,entry)),1),1)
    installed_entries(output)
    return output

# Exact tuples from actual current and independently regenerated source.
CONTEXTS={
    (True,
     '6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9',
     'abcda7bb63b840d4f2ce25ec163dd66c9964b2a928af4ac897bd7b33765aa599',
     '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
     '7eea402b148be600ea6ed5d42d41558af6cbb06b45379e66d8196035a9fa4b53',
     '198b2f6d6c55da3880d6f52f5a1623a9d6e276602a70f71c376eb2b84cd13554',
     '038aa0e13b0067f9db55ae1cae455a5a61d1dedfc406dbbb90a09ac6fc2ef351',
     '51dd3d8808ea27010d233e13719b94e1e390add295d300df44837586003567b4'), # current-prepared
    (False,
     '6df9d932e8db914ca60dfe34f67353abaca30ba9147ce5ff1ae1c01b3e0cc3b9',
     'f754c70cbc7f2e502ea1e757ef67688c89a38f38b665e7e2ba66f711571dc84c',
     '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
     '7eea402b148be600ea6ed5d42d41558af6cbb06b45379e66d8196035a9fa4b53',
     '198b2f6d6c55da3880d6f52f5a1623a9d6e276602a70f71c376eb2b84cd13554',
     'a95fc0a2a7f7fd9573754b371206b514e586fbc03156465fe93111f67991d65b',
     '9aec69524b78ece77f6b8f3252b282cb4b9c3e42e2125f4fb41623b55c2ee819'), # current-raw
    (True,
     '5dda1bc027ca22bb0dfb680fea8c6ac399b3de4002aea0733e29373ed7f7b08d',
     'e32f2927198be567ac60d36c8f00767d8941832313a643fd964f3297e651f5f4',
     '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
     '4f4ae0165fbbbb4d37d893eb82f487055376425b7aebf9385dc0ac152f184739',
     '766f168a1357ae9fb01edc1e8489cb71741e3971db052a6ae4d80a0799646c49',
     '038aa0e13b0067f9db55ae1cae455a5a61d1dedfc406dbbb90a09ac6fc2ef351',
     '51dd3d8808ea27010d233e13719b94e1e390add295d300df44837586003567b4'), # fresh-prepared
    (False,
     '5dda1bc027ca22bb0dfb680fea8c6ac399b3de4002aea0733e29373ed7f7b08d',
     'e32f2927198be567ac60d36c8f00767d8941832313a643fd964f3297e651f5f4',
     '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
     '4f4ae0165fbbbb4d37d893eb82f487055376425b7aebf9385dc0ac152f184739',
     '766f168a1357ae9fb01edc1e8489cb71741e3971db052a6ae4d80a0799646c49',
     '60e5866d16b1cc554ab587813a1912695a4671c59f447ca9e83b0eb1a15254fa',
     '3bc5d8116e749b8cc9cb98e12e6b03d0c46c5e3d03ded28fbb32d5e885db7cd3'), # fresh-raw
}
