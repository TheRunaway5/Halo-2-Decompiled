"""Exact object-material BCDF0 native prefix and retained source continuations."""
import importlib.util
from pathlib import Path
_spec=importlib.util.spec_from_file_location('material_lookup_object_body',Path(__file__).with_name('native_object_body_replacements.py'))
_body=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_body)
BODY_PREFIX,FRONTIERS=_body.PREFIX,_body.FRONTIERS
one,digest,body_original=_body.one,_body.digest,_body.original_body
PREFIX=BODY_PREFIX.replace('native_object_body_owner_detail::bridge','native_object_material_owner_detail::bridge')
INCLUDE='#include "gameplay/objects/native_tu15_object_material_lookup_source_owner.hpp"\n'
def original_body(body):
    if any(value in body for value in (BODY_PREFIX, _body.prefix(False), _body.prefix(True))):
        raise ValueError('Wrong source-owner lookup prefix')
    owned = entry_prefix(body)
    return body_original(body.replace(owned, BODY_PREFIX, 1) if owned else body)

def parts(source):
    return {
        'body':one(source,r'^static void run_bcdf0\(WorldState &s\) \{.*?^\}','material lookup'),
        'resolver':one(source,r'^std::uint32_t selected\([^\n]+','material resolver'),
        'dispatcher':one(source,r'^static void dispatch\([^\n]+','material dispatcher'),
        'state':one(source,r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;','material state'),
        'sign_extend':one(source,r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}','material sign extension'),
        'public_entry':one(source,r'^void source_run_object_material_tu15\(.*?^\}','material public entry'),
        'owns_entry':one(source,r'^bool source_object_material_tu15_owns_entry\([^\n]+','material public ownership'),
    }
PINS={
    'resolver':'1e418ecd5d9e170b1ea130f1123c708a5692f49349e2261c34b737cbc29672ad',
    'state':'457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
    'sign_extend':'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
    'public_entry':'70c8c16742ecdb2822353be42a2409fc041bd9a6c561f8d1383a764532930cf5',
    'owns_entry':'10c894f7815aa184ee899c6249510ef56c2aa3e41ca6ce2abada69fbfc0a22b6',
}
CONTEXTS={
    ('85928a501bfc9101c77ec90ea28bad7f671178173909f4a52220e06e3fc3320c','4e5ccfdbda9d04a0db79f8fecc02977aeda23d16ed56a5465a1495a4fc9f7f3d'),
    ('8fc06927ea71a7373c2f01b38037dfd22c5211a7e11976f9d386000de9ed9dfb','4e5857ae193a156396c5512b0030c4aa8faa3fc6d5d49f7ea4b7fbe922ac97a2'),
    ('7fb661173c58fe99bab29436f92e0842610961c1e965639ebe8cf8e8b184e88c','145cbcf1d8d79687a01c74df576fe0905d5f79d256f50ffc750b6550624fd43a'),
    ('7fb661173c58fe99bab29436f92e0842610961c1e965639ebe8cf8e8b184e88c','4e5857ae193a156396c5512b0030c4aa8faa3fc6d5d49f7ea4b7fbe922ac97a2'),
}
def canonical_body(body):return original_body(body)
def authenticate(source):
    if 'void source_run_object_material_tu15('not in source:
        if any(value in source for value in (PREFIX, prefix(False), prefix(True))) or INCLUDE in source:raise ValueError('Missing complete material owner')
        return None
    values=parts(source);body=original_body(values['body'])
    _body.validate_source_overlay(source,values['body'],PREFIX,'run_bcdf0')
    if (digest(body),digest(values['dispatcher']))not in CONTEXTS:
        raise ValueError('Complete material lookup body/dispatcher tuple drift')
    for key,pin in PINS.items():
        if digest(values[key])!=pin:raise ValueError('Complete material lookup context drift: '+key)
    if source.count(INCLUDE)>1 or(entry_prefix(values['body']) is not None)!=(INCLUDE in source):
        raise ValueError('Partial material lookup overlay')
    return values,body
def replace_object_material_lookup(source):
    found=authenticate(source)
    if found is None:return source
    values,body=found
    desired = prefix(_body.callback_format(body))
    current = entry_prefix(values['body'])
    if current:
        if current == desired:
            return source
        result = source.replace(values['body'], values['body'].replace(current, desired, 1), 1)
        authenticate(result)
        return result
    proposed=body
    for pc in FRONTIERS:
        callbacks=(f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}',f's.instruction_literal(0x{pc:x}u);')
        matches=[callback for callback in callbacks if proposed.count(callback)==1]
        if len(matches)!=1:raise ValueError('Missing material continuation instruction '+hex(pc))
        proposed=proposed.replace(matches[0],f'NativeObjectBody{pc:x}: '+matches[0],1)
    at=proposed.index('{')+1;proposed=proposed[:at]+desired+proposed[at:]
    result=INCLUDE+source.replace(values['body'],proposed,1);authenticate(result);return result
def remove_object_material_lookup(source):
    found=authenticate(source)
    if found is None:return source
    values,body=found;return source.replace(values['body'],body,1).replace(INCLUDE,'',1)
def installed_entries(source):
    found=authenticate(source)
    if found is None or entry_prefix(found[0]['body']) in (None, PREFIX)or source.count(INCLUDE)!=1:
        raise ValueError('Complete material lookup default missing')
    return ['bcdf0']

def prefix(literal):
    return _body.tagged_prefix(PREFIX, literal)

def entry_prefix(body, check_format=True):
    return _body.lookup_entry_prefix(body, PREFIX, 'run_bcdf0', check_format)

def retag_specialized(source, output):
    found = authenticate(source)
    if found is None:
        return output
    old = entry_prefix(found[0]['body'])
    if old is None:
        return output
    values = parts(output)
    current = entry_prefix(values['body'], check_format=False)
    if current != old:
        raise ValueError('Specializer changed the authenticated lookup prefix before retagging')
    normalized = output.replace(values['body'], values['body'].replace(current, PREFIX, 1), 1)
    # The installer reauthenticates the entire prepared context before choosing
    # its new format; an unbound input never reaches installation here.
    result = replace_object_material_lookup(normalized)
    installed_entries(result)
    return result
