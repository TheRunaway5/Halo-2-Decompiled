"""Exact canonical BBCB0 damage/vitality lookup owner candidates."""
import importlib.util,re
from pathlib import Path
_spec=importlib.util.spec_from_file_location('canonical_lookup_body',Path(__file__).with_name('native_object_body_replacements.py'))
_shared=importlib.util.module_from_spec(_spec);_spec.loader.exec_module(_shared)
digest,one=_shared.digest,_shared.one
FRONTIERS=tuple(pc-0x1140 for pc in _shared.FRONTIERS)
INCLUDE='#include "gameplay/objects/native_tu15_object_body_canonical_source_owner.hpp"\n'
PREFIX='\n    {switch(halo2::native_object_body_canonical_owner_detail::bridge(s)){case 0u:break;case 0xffffffffu:return;'+''.join(f'case 0x{pc:x}u:goto CanonicalObjectBody{pc:x};'for pc in FRONTIERS)+'default:throw std::runtime_error("Invalid native canonical object lookup continuation");}}'
BODY_PIN='cae448bb249c9782915b92c9d81ba6830d84bb4d26108c6cc8227b356577e0cf'
PREPARED_PIN='e441bb76b6f032a4d49b5d237d250d1e49e3d5a68abf11f9a3c3a8afce56e176'
PINS={
    'direct_damage':{
        'resolver':'b070e98cf4774810ca643f3626c3b205ec60fdc48b8c4784888044b327a9d090',
        'public_entry':'3b6987eefcf69301c47318c8b5931d4fa5020701f99c277ece18ddf784275aa3',
        'contexts':{
            (BODY_PIN,'3fc298a903dc32e58683262d6fad4ac84d57b750c260a0be27ad3149dff2f78e','727ef5637dd347c62459d3acff2d40d01d64d20ca809ffa383797c8aa4e73aaf'),
            (PREPARED_PIN,'7004459cd6aa865a4bafc72289b36262614fc3ccd3ea18184d784e86a2d00aad','4849fd314d14b4dc8ba52536b233aac71a2db9551d1d639209873e5ea4274990'),
        },
    },
    'vitality':{
        'resolver':'3185bc16db86ecd29059efbfb8205fd02eefe382f81ba3c28fa4e5e991917d33',
        'public_entry':'357821722505a180e77c6e20e76b63cd84cf36a668e76d3d75e5000f4b314094',
        'contexts':{
            (BODY_PIN,'e6cb58b012661005cbfb232eebd7d2d8ced648886ffd82fc9c1d61fd7a9815e4','85d395c918a48edc1f73f550cf917b37193d75dd8517896ac067c4d5de85f072'),
            (PREPARED_PIN,'6681ab4bfa73740c6edc551b2b5f60c271ff80be008c8d92198eb65e424eed1f','1a2250fdebfed75d2addffd0444a1039823b263eb5d703fd6c5c435f0205346f'),
        },
    },
}
def parts(source,owner):
    body=one(source,r'^static void run_bbcb0\(WorldState &s\) \{.*?^\}',owner+' lookup')
    return dict(body=body,
        preamble=source[:source.index(body)].replace(INCLUDE,'',1),
        resolver=one(source,r'^std::uint32_t selected\([^\n]+',owner+' resolver'),
        dispatcher=one(source,r'^static void dispatch\([^\n]+',owner+' dispatcher'),
        sign_extend=one(source,r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}',owner+' sign extension'),
        public_entry=one(source,r'^void source_run_'+owner+r'_runtime\(.*?^\}',owner+' public entry'))
def canonical_body(body):
    owned = entry_prefix(body)
    if owned:
        if body.count(owned)!=1:raise ValueError('Ambiguous canonical lookup prefix')
        body=body.replace(owned,'',1)
        for pc in FRONTIERS:
            label=f'CanonicalObjectBody{pc:x}: '
            callbacks=(f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}',f's.instruction_literal(0x{pc:x}u);')
            if body.count(label)!=1 or sum(body.count(label+callback)for callback in callbacks)!=1:raise ValueError('Canonical lookup continuation is not at its exact next instruction')
            body=body.replace(label,'',1)
    elif 'CanonicalObjectBody'in body:raise ValueError('Partial canonical lookup labels')
    return body
def authenticate(source):
    owners=[owner for owner in PINS if 'void source_run_'+owner+'_runtime('in source]
    if not owners:
        if any(value in source for value in (PREFIX, prefix(False), prefix(True))) or INCLUDE in source:raise ValueError('Missing complete canonical lookup source owner')
        return None
    if len(owners)!=1:raise ValueError('Ambiguous canonical lookup source owner')
    owner=owners[0];values=parts(source,owner);body=canonical_body(values['body']);pins=PINS[owner]
    _shared.validate_source_overlay(source,values['body'],PREFIX,'run_bbcb0')
    if (digest(body),digest(values['dispatcher']),digest(values['preamble']))not in pins['contexts']:raise ValueError('Complete canonical lookup body/dispatcher/preamble tuple drift: '+owner)
    for key in('resolver','public_entry'):
        if digest(values[key])!=pins[key]:raise ValueError('Complete canonical lookup context drift: '+owner+' '+key)
    if digest(values['sign_extend'])!='cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b':raise ValueError('Canonical lookup sign-extension drift')
    if source.count(INCLUDE)>1 or (entry_prefix(values['body']) is not None)!=(INCLUDE in source):raise ValueError('Partial canonical lookup include/prefix')
    return owner,values,body
def replace_canonical_object_lookup(source):
    found=authenticate(source)
    if found is None:return source
    owner,values,body=found
    desired = prefix(_shared.callback_format(body))
    current = entry_prefix(values['body'])
    if current:
        if current == desired:
            return source
        result = source.replace(values['body'], values['body'].replace(current, desired, 1), 1)
        authenticate(result)
        return result
    proposed=body
    for pc in FRONTIERS:
        needles=(f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}',f's.instruction_literal(0x{pc:x}u);')
        matches=[value for value in needles if proposed.count(value)==1]
        if len(matches)!=1:raise ValueError('Canonical lookup next-PC callback drift')
        proposed=proposed.replace(matches[0],f'CanonicalObjectBody{pc:x}: '+matches[0],1)
    at=proposed.index('{')+1;proposed=proposed[:at]+desired+proposed[at:]
    result=INCLUDE+source.replace(values['body'],proposed,1);authenticate(result);return result
def remove_canonical_object_lookup(source):
    found=authenticate(source)
    if found is None:return source
    _,values,body=found
    return source.replace(values['body'],body,1).replace(INCLUDE,'',1)
def installed_entries(source):
    found=authenticate(source)
    if found is None or entry_prefix(found[1]['body']) in (None, PREFIX)or source.count(INCLUDE)!=1:raise ValueError('Complete canonical lookup default missing')
    return ['bbcb0']

def prefix(literal):
    return _shared.tagged_prefix(PREFIX, literal)

def entry_prefix(body, check_format=True):
    return _shared.lookup_entry_prefix(body, PREFIX, 'run_bbcb0', check_format)

def retag_specialized(source, output):
    found = authenticate(source)
    if found is None:
        return output
    old = entry_prefix(found[1]['body'])
    if old is None:
        return output
    values = parts(output, found[0])
    current = entry_prefix(values['body'], check_format=False)
    if current != old:
        raise ValueError('Specializer changed the authenticated lookup prefix before retagging')
    normalized = output.replace(values['body'], values['body'].replace(current, PREFIX, 1), 1)
    # The installer reauthenticates the entire prepared context before choosing
    # its new format; an unbound input never reaches installation here.
    result = replace_canonical_object_lookup(normalized)
    installed_entries(result)
    return result
