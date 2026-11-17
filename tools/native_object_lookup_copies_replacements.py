"""Exact native BCDF0 bindings in independently pinned LiveState owners."""
import importlib.util
from pathlib import Path

_spec = importlib.util.spec_from_file_location('lookup_copies_object_body', Path(__file__).with_name('native_object_body_replacements.py'))
_body = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(_body)
FRONTIERS, BODY_PREFIX = _body.FRONTIERS, _body.PREFIX
digest, one, body_original = _body.digest, _body.one, _body.original_body

PREFIX = BODY_PREFIX.replace('native_object_body_owner_detail::bridge', 'native_object_lookup_copies_detail::bridge')
INCLUDE = '#include "gameplay/objects/native_tu15_object_lookup_copies_source_owner.hpp"\n'
PINS = {
    'scene_device_constructor': {
        'resolver': '618bc56800beb42be63e189783dd7cf9a98d7b904e56fa92de6523bf86490d7a',
        'public_entry': '6d590b75f644574c52984ae3abd22643e4f2c2041b3512d7de67dd2d41ebdd29',
        'owns_entry': '945e6e777ec1e57140228f167fbb54b4ffbff1bd9387777588c5ce3b8b03b199',
    },
    'sandbox_special': {
        'resolver': '0fddb23fd6f15344aa915c1c50b845847dd34460782d49467474e9e1cfed5fed',
        'public_entry': '97e57106fff2fdabcfb9ff01fbcb3f9289a2468d05a4a7b40eafd8ead03947cb',
        'owns_entry': 'e25a3a6d86b26e2f26d43c270e03206b26aefbd76c1724cc565436199ba3318c',
    },
    'effect_particle_draw': {
        'resolver': '892cf508106b0c182d59e4dba45804e434a1833003cfb3c322f9a8875c54d7fb',
        'public_entry': '62beb6ccb71e96fa1d5f6898a2ba46ebabd8af12d33d19aa1fcc551a28b81fa0',
        'owns_entry': '03c9ff021ad2f333214cf1e82ddc2fd3e843aeaef234d88f714aa746f92503c2',
    },
    'ai_frame': {
        'owns_entry': '796da27b6844970e694891694e65fd57c777fab941c9a599312de009570be603',
    },
}
COMMON_PINS = {
    'state': '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
    'sign_extend': 'cb782506d89e5292b91cf2af63c700e9db63fdb29726f7c9975645104dff802b',
}
SELECTED_BODY = '85928a501bfc9101c77ec90ea28bad7f671178173909f4a52220e06e3fc3320c'
LITERAL_BODY = '8fc06927ea71a7373c2f01b38037dfd22c5211a7e11976f9d386000de9ed9dfb'
PREPARED_BODY = '7fb661173c58fe99bab29436f92e0842610961c1e965639ebe8cf8e8b184e88c'
# Current and genuine freshly generated AI contexts have different complete
# resolver/dispatcher/public-entry implementations. Admit only these correlated
# four-tuples, with the common complete State/helper/ownership pins.
AI_CONTEXTS = {
    (SELECTED_BODY, '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657', '1cf61f52187c986d02510a40d28a466d95589b66af5521ade12ae9640085e28b', 'b68a8359dfc33546046f9c0681947872dd65da4edabe2e91d03338f223d1c789'),
    (LITERAL_BODY, 'b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8', '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a', '379089fe73835acd3b88fe2d6e9c3f77b961546e49c4fc823d783a12956968c5'),
    (PREPARED_BODY, '5eab69b03c4be4556290b5ba3e7765897c12d0064ea5b20267317b365ea4a657', '06f9ce13109229ec52f698071f0abb7f403a23f29cd2efaf9e27828d4057423e', 'b68a8359dfc33546046f9c0681947872dd65da4edabe2e91d03338f223d1c789'),
    (PREPARED_BODY, 'b366b77691cb314f6c6d0f8f20e71c1bda7114225c8d7b1f4043ee80e12354b8', '974fddd715271a3ccdc93c266782b8f84d13f514bd11653b34af22a1f8be267a', '379089fe73835acd3b88fe2d6e9c3f77b961546e49c4fc823d783a12956968c5'),
}
CONTEXTS = {
    'scene_device_constructor': {
        (SELECTED_BODY, '1a68d583722bac0a950594a436cbb9f1b28b09624517589c9ee8ad507712f605'),
        (PREPARED_BODY, '30d34d2c58b7adae1e2a3132d26e1da47e7278086c1e5ad7904fdfb58626241f'),
    },
    'sandbox_special': {
        (SELECTED_BODY, '961134517848136bf248393b533a030ae2607735d71fec8f570e0332e5ce4bc6'),
        (LITERAL_BODY, 'd46a1635c618577534bfdd091f45ecea819b4886662ca1309400154271d5a987'),
        (PREPARED_BODY, 'dffd3beb5abbf920720b8ab5d8cb462f41bd953860b2eb02c0d072a28a65f2da'),
        (PREPARED_BODY, 'd46a1635c618577534bfdd091f45ecea819b4886662ca1309400154271d5a987'),
    },
    'effect_particle_draw': {
        (LITERAL_BODY, 'c0e112fb90d7ea32e73e6b3b37d333b6a375456791c5f2eaf9a08906d613f6a9'),
        (PREPARED_BODY, 'c0e112fb90d7ea32e73e6b3b37d333b6a375456791c5f2eaf9a08906d613f6a9'),
    },
}

def parts(source, owner):
    return {
        'body': one(source, r'^static void run_bcdf0\(WorldState &s\) \{.*?^\}', owner + ' lookup'),
        'resolver': one(source, r'^std::uint32_t selected\([^\n]+', owner + ' resolver'),
        'dispatcher': one(source, r'^static void dispatch\([^\n]+', owner + ' dispatcher'),
        'state': one(source, r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;', owner + ' state'),
        'sign_extend': one(source, r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}', owner + ' sign extension'),
        'public_entry': one(source, r'^void source_run_' + owner + r'_tu15\(.*?^\}', owner + ' public entry'),
        'owns_entry': one(source, r'^bool source_' + owner + r'_tu15_owns_entry\([^\n]+', owner + ' ownership'),
    }

def original_body(body):
    if any(value in body for value in (BODY_PREFIX, _body.prefix(False), _body.prefix(True))):
        raise ValueError('Wrong source-owner lookup prefix')
    owned = entry_prefix(body)
    return body_original(body.replace(owned, BODY_PREFIX, 1) if owned else body)

def canonical_body(body):
    return original_body(body)

def authenticate(source):
    owners = [owner for owner in PINS if 'void source_run_' + owner + '_tu15(' in source]
    if not owners:
        if any(value in source for value in (PREFIX, prefix(False), prefix(True))) or INCLUDE in source:
            raise ValueError('Missing complete lookup copy owner')
        return None
    if len(owners) != 1:
        raise ValueError('Ambiguous complete lookup copy owner')
    owner = owners[0]
    values = parts(source, owner)
    body = original_body(values['body'])
    _body.validate_source_overlay(source, values['body'], PREFIX, 'run_bcdf0')
    if owner == 'ai_frame':
        context = tuple(digest(value) for value in (body, values['resolver'], values['dispatcher'], values['public_entry']))
        if context not in AI_CONTEXTS:
            raise ValueError('Complete lookup copy correlated AI context drift')
    elif (digest(body), digest(values['dispatcher'])) not in CONTEXTS[owner]:
        raise ValueError('Complete lookup copy body/dispatcher tuple drift: ' + owner)
    for key, pin in (COMMON_PINS | PINS[owner]).items():
        if digest(values[key]) != pin:
            raise ValueError('Complete lookup copy context drift: ' + owner + ' ' + key)
    if source.count(INCLUDE) > 1 or (entry_prefix(values['body']) is not None) != (INCLUDE in source):
        raise ValueError('Partial lookup copy overlay')
    return owner, values, body

def replace_object_lookup_copies(source):
    found = authenticate(source)
    if found is None:
        return source
    owner, values, body = found
    desired = prefix(_body.callback_format(body))
    current = entry_prefix(values['body'])
    if current:
        if current == desired:
            return source
        result = source.replace(values['body'], values['body'].replace(current, desired, 1), 1)
        authenticate(result)
        return result
    proposed = body
    for pc in FRONTIERS:
        callbacks = (f'if(s.services.instruction){{s.services.instruction(0x{pc:x}u);}}', f's.instruction_literal(0x{pc:x}u);')
        matches = [callback for callback in callbacks if proposed.count(callback) == 1]
        if len(matches) != 1:
            raise ValueError('Missing exact lookup copy continuation ' + owner + ' ' + hex(pc))
        proposed = proposed.replace(matches[0], f'NativeObjectBody{pc:x}: ' + matches[0], 1)
    at = proposed.index('{') + 1
    proposed = proposed[:at] + desired + proposed[at:]
    result = INCLUDE + source.replace(values['body'], proposed, 1)
    authenticate(result)
    return result

def remove_object_lookup_copies(source):
    found = authenticate(source)
    if found is None:
        return source
    owner, values, body = found
    return source.replace(values['body'], body, 1).replace(INCLUDE, '', 1)

def installed_entries(source):
    found = authenticate(source)
    if found is None or entry_prefix(found[1]['body']) in (None, PREFIX) or source.count(INCLUDE) != 1:
        raise ValueError('Complete lookup copy native default missing')
    return ['bcdf0']

def prefix(literal):
    return _body.tagged_prefix(PREFIX, literal)

def entry_prefix(body, check_format=True):
    return _body.lookup_entry_prefix(body, PREFIX, 'run_bcdf0', check_format)

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
    result = replace_object_lookup_copies(normalized)
    installed_entries(result)
    return result
