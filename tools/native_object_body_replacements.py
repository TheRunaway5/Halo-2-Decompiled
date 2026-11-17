"""Exact UnitView BCDF0 native entry and retained-provider continuations."""
import hashlib,re
FRONTIERS=(0xbcdf1,0xbcdf2,0xbce01,0xbce04,0xbce13,0xbce19,0xbce1c,
           0xbce36,0xbce41,0xbce46,0xbce47,0xbce48)
def one(source,pattern,name):
    values=re.findall(pattern,source,re.M|re.S)
    if len(values)!=1:raise ValueError('Missing/ambiguous complete UnitView '+name)
    return values[0]
def parts(source):
    return {
        'body':one(source,r'^static void run_bcdf0\(WorldState &s\) \{.*?^\}','lookup'),
        'resolver':one(source,r'^std::uint32_t selected\([^\n]+','resolver'),
        'dispatcher':one(source,r'^static void dispatch\([^\n]+','dispatcher'),
        'state':one(source,r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;','state'),
        'sign_extend':one(source,r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}','sign extension'),
        'public_entry':one(source,r'^void source_run_unit_view_reset_tu15\(.*?^\}','public entry'),
        'owns_entry':one(source,r'^bool source_unit_view_reset_tu15_owns_entry\([^\n]+','public ownership'),
    }
def retained_literal_projection(value):
    return re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',
        r'if(s.services.instruction){s.services.instruction(\1);}',value)
INCLUDE='#include "gameplay/objects/native_tu15_object_body_source_owner.hpp"\n'
PINS={
    'body':'8fc06927ea71a7373c2f01b38037dfd22c5211a7e11976f9d386000de9ed9dfb',
    'dispatcher':'052b7b082a0c9eb8b44996c8d319c6aa1440411a30f8fd2eccd21012537d7ba2',
    'owns_entry':'2d82180f0562f39ed80ef8c421bb066c379da18247f66dbb48be682b7f48b984',
    'public_entry':'015e8fc223593221ea91fd231b7f74041d5bcd163ec20aa033d6c099364b688c',
    'resolver':'717655ef34d13abab546ab82b97f1b8f216dd8996e6f460e82b385c8210af0fc',
    'sign_extend':'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
    'state':'457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
}
PREFIX='\n    {switch(halo2::native_object_body_owner_detail::bridge(s)){case 0u:break;case 0xffffffffu:return;'+''.join(
    f'case 0x{pc:x}u:goto NativeObjectBody{pc:x};'for pc in FRONTIERS)+'default:throw std::runtime_error("Invalid native object lookup continuation");}}'
def digest(value):return hashlib.sha256(value.encode()).hexdigest()
def original_body(body):
    owned = entry_prefix(body)
    if owned:
        if body.count(owned)!=1:raise ValueError('Ambiguous object lookup candidate prefix')
        body=body.replace(owned,'',1)
        for pc in FRONTIERS:
            label=f'NativeObjectBody{pc:x}: '
            if body.count(label)!=1:raise ValueError('Missing object lookup continuation label')
            callbacks=(f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}',f's.instruction_literal(0x{pc:x}u);')
            if sum(body.count(label+callback)for callback in callbacks)!=1:
                raise ValueError('Object lookup continuation label is not at its exact next instruction')
            body=body.replace(label,'',1)
    elif 'NativeObjectBody'in body:raise ValueError('Partial object lookup continuation overlay')
    return body
def canonical_body(body):
    return original_body(body)
def authenticate(source):
    if 'void source_run_unit_view_reset_tu15('not in source:
        if any(value in source for value in (PREFIX, prefix(False), prefix(True))) or INCLUDE in source:raise ValueError('Missing actual UnitView owner')
        return None
    values=parts(source);body=original_body(values['body'])
    validate_source_overlay(source,values['body'],PREFIX,'run_bcdf0')
    for name,pin in PINS.items():
        value=retained_literal_projection(body)if name=='body'else values[name]
        if digest(value)!=pin:raise ValueError('Complete object lookup context drift: '+name)
    if source.count(INCLUDE)>1 or (entry_prefix(values['body']) is not None)!=(INCLUDE in source):
        raise ValueError('Incomplete object lookup candidate include/prefix')
    return values,body
def replace_object_body(source):
    found=authenticate(source)
    if found is None:return source
    values,body=found
    desired = prefix(callback_format(body))
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
        if len(matches)!=1:raise ValueError('Missing exact object lookup continuation '+hex(pc))
        proposed=proposed.replace(matches[0],f'NativeObjectBody{pc:x}: '+matches[0],1)
    at=proposed.index('{')+1;proposed=proposed[:at]+desired+proposed[at:]
    result=INCLUDE+source.replace(values['body'],proposed,1)
    authenticate(result);return result
def remove_object_body(source):
    found=authenticate(source)
    if found is None:return source
    values,body=found
    return source.replace(values['body'],body,1).replace(INCLUDE,'',1)
def installed_entries(source):
    found=authenticate(source)
    if found is None or entry_prefix(found[0]['body']) in (None, PREFIX) or source.count(INCLUDE)!=1:
        raise ValueError('Complete object lookup candidate is not installed')
    return ['bcdf0']

def tagged_prefix(legacy, literal):
    return legacy.replace('::bridge(s)', '::bridge<' + ('true' if literal else 'false') + '>(s)')

def callback_format(body):
    raw = re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}', body)
    literal = re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);', body)
    if (len(raw), len(literal)) == (35, 0):
        return False
    if (len(raw), len(literal)) == (0, 35):
        return True
    raise ValueError('Lookup body must contain exactly35 uniform callback sites')

def lookup_entry_prefix(body, legacy, entry, check_format=True):
    literal = callback_format(body)
    variants = (legacy, tagged_prefix(legacy, False), tagged_prefix(legacy, True))
    found = [value for value in variants if value in body]
    if not found:
        return None
    if len(found) != 1 or body.count(found[0]) != 1:
        raise ValueError('Ambiguous lookup entry prefix')
    header = 'static void ' + entry + '(WorldState &s) {'
    if not body.startswith(header) or not body.startswith(found[0], len(header)):
        raise ValueError('Lookup prefix is not at its exact function entry')
    current = found[0]
    if check_format and current != legacy and current != tagged_prefix(legacy, literal):
        raise ValueError('Lookup format tag disagrees with authenticated callback format')
    return current

LOOKUP_OWNER_NAMESPACES = (
    'native_object_body_owner_detail',
    'native_object_material_owner_detail',
    'native_object_lookup_copies_detail',
    'native_object_body_canonical_owner_detail',
)

def validate_source_overlay(source, body, legacy, entry):
    current = lookup_entry_prefix(body, legacy, entry)
    expected = int(current is not None)
    variants = (legacy, tagged_prefix(legacy, False), tagged_prefix(legacy, True))
    if sum(source.count(value) for value in variants) != expected:
        raise ValueError('Lookup source contains an overlay outside its exact owner entry')
    owners = re.findall(r'halo2::([A-Za-z_]+)::bridge', legacy)
    if len(owners) != 1 or owners[0] not in LOOKUP_OWNER_NAMESPACES:
        raise ValueError('Unknown lookup source-owner namespace')
    for owner in LOOKUP_OWNER_NAMESPACES:
        count = source.count('halo2::' + owner + '::bridge')
        if count != (expected if owner == owners[0] else 0):
            raise ValueError('Lookup source contains a misplaced or foreign owner bridge')

def prefix(literal):
    return tagged_prefix(PREFIX, literal)

def entry_prefix(body, check_format=True):
    return lookup_entry_prefix(body, PREFIX, 'run_bcdf0', check_format)

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
    result = replace_object_body(normalized)
    installed_entries(result)
    return result
