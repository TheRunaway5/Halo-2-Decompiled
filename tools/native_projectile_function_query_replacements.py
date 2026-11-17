"""Exact current/fresh CombatAction 100E70 native overlay."""
import hashlib
import re

INCLUDE = '#include "gameplay/objects/native_tu15_projectile_function_query_source_owner.hpp"\n'
INCLUDE_ANCHOR = '#include "halo2/source_effect_compiled_symbols_tu15.hpp"\n'
HEADER = 'static void run_100e70(WorldState &s) {'
BRIDGE = 'halo2::native_projectile_function_query_owner_detail::bridge'
OWNER_NAMESPACE = 'native_projectile_function_query_owner_detail'
CORE_NAMESPACE = 'native_projectile_function_query_detail'
INCLUDE_BASENAME = 'native_tu15_projectile_function_query_source_owner.hpp'
LEGACY_MULTIPLY = '_mm_mul_ss(s.xmm[0],'
ORDERED_MULTIPLY = 'halo2::native_projectile_function_query_detail::ordered_multiply(s.xmm[0],'
RESOLVER = '7b1e3e35e2876b19b074a87b5235526135a7effaaa31c63b2e86450091613d75'
PUBLIC = '8babe9ef20df7e057fde057724996687cced5f124bc6fc80910b5a777ad39132'
OWNERSHIP = '5ffaf3d432c95512967e86067d95492571ed9f7989aa4b897ccc5457c5de9135'
CURRENT_STATE = '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46'
FRESH_STATE = '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575'
PREPARED_BODY = 'c5d1d0c81a46d4c0422ebaa88db32ff361eba6b68aa4470259503cf4b80d0fbc'
FRESH_DISPATCH = 'f434da6ab6b8f8239fe575d6e75efb8e00f4a67a5188ac30cfff70e1f8d3a0ef'
# Keep these correlated tuples. The two complete State declarations and
# dispatchers must not be substituted independently of their real subjects.
CONTEXTS = {
    (False, '1cc7aa2f4564c9854dd07cce158fb1f3149425562bf797d61c0154ba378d4d3f',
     '2f716e064af3a72690bbe766be88580a1927daa39319590d18444e94e00114ef', CURRENT_STATE),
    (False, 'f992252190424c58c1e18d2cea265f2188f0d33ae23f11473e9bcb105b42f525', FRESH_DISPATCH, FRESH_STATE),
    (True, PREPARED_BODY, 'ba92d4566306f2d5010c8f6b8276d209a573f7dfa97835e511faf7bc77644f5d', CURRENT_STATE),
    (True, PREPARED_BODY, FRESH_DISPATCH, FRESH_STATE),
}


def digest(value):
    return hashlib.sha256(value.encode()).hexdigest()


def one(source, pattern, name):
    values = re.findall(pattern, source, re.M | re.S)
    if len(values) != 1:
        raise ValueError('Missing or ambiguous projectile query '+name)
    return values[0]


def body(source):
    return one(source, r'^static void run_100e70\(WorldState &s\) \{.*?^\}', 'body')


def literal_format(text):
    raw = re.findall(r'if\(s\.services\.instruction\)\{s\.services\.instruction\(0x[0-9a-f]+u\);\}', text)
    prepared = re.findall(r's\.instruction_literal\(0x[0-9a-f]+u\);', text)
    if (len(raw), len(prepared)) == (91, 0):
        return False
    if (len(raw), len(prepared)) == (0, 91):
        return True
    raise ValueError('Projectile query must retain 91 uniform callback sites')


def prefix(literal):
    return '\n    if('+BRIDGE+'<'+('true' if literal else 'false')+'>(s))return;'


def entry_prefix(text):
    found = [value for value in (prefix(False), prefix(True)) if value in text]
    if not found and BRIDGE not in text:
        return ''
    if len(found) != 1 or text.count(found[0]) != 1 or not text.startswith(HEADER+found[0]):
        raise ValueError('Projectile query bridge moved away from its entry')
    return found[0]


def order_fallback(text):
    if text.count(LEGACY_MULTIPLY) != 1 or ORDERED_MULTIPLY in text:
        raise ValueError('Missing exact projectile fallback multiply')
    return text.replace(LEGACY_MULTIPLY, ORDERED_MULTIPLY, 1)


def canonical_body(text):
    entry = entry_prefix(text)
    if not entry:
        return text
    base = text.replace(entry, '', 1)
    if base.count(ORDERED_MULTIPLY) != 1 or LEGACY_MULTIPLY in base:
        raise ValueError('Missing ordered original projectile fallback multiply')
    return base.replace(ORDERED_MULTIPLY, LEGACY_MULTIPLY, 1)


def authenticate(source):
    if 'void source_run_combat_action_tu15(' not in source:
        if any(marker in source for marker in (OWNER_NAMESPACE, CORE_NAMESPACE, INCLUDE_BASENAME)):
            raise ValueError('Projectile query owner missing')
        return None
    original = body(source)
    entry = entry_prefix(original)
    base = canonical_body(original)
    literal = literal_format(base)
    if entry and entry != prefix(literal):
        raise ValueError('Wrong projectile query callback format tag')
    resolver = one(source, r'^std::uint32_t selected\([^\n]+', 'selected resolver')
    dispatch = one(source, r'^static void dispatch\([^\n]+', 'dispatcher')
    state = one(source, r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;', 'State')
    api = one(source, r'^void source_run_combat_action_tu15\(.*?^\}', 'public entry')
    owns = one(source, r'^bool source_combat_action_tu15_owns_entry\([^\n]+', 'public ownership')
    if (digest(resolver), digest(api), digest(owns)) != (RESOLVER, PUBLIC, OWNERSHIP):
        raise ValueError('Complete projectile query resolver/public context changed')
    if (literal, digest(base), digest(dispatch), digest(state)) not in CONTEXTS:
        raise ValueError('Complete correlated projectile query body/dispatcher/State changed')
    owned = int(bool(entry))
    if any(source.count(marker) != owned for marker in (
        INCLUDE, BRIDGE, INCLUDE_ANCHOR+INCLUDE, ORDERED_MULTIPLY,
        OWNER_NAMESPACE, CORE_NAMESPACE, INCLUDE_BASENAME,
    )):
        raise ValueError('Partial, moved, external or duplicate projectile query overlay')
    if source.count(INCLUDE_ANCHOR) != 1:
        raise ValueError('Ambiguous projectile query include site')
    return original, base, literal, entry


def replace_projectile_function_query(source):
    found = authenticate(source)
    if not found or found[3]:
        return source
    original, _, literal, _ = found
    fallback = order_fallback(original)
    result = source.replace(original, HEADER+prefix(literal)+fallback[len(HEADER):], 1)
    result = result.replace(INCLUDE_ANCHOR, INCLUDE_ANCHOR+INCLUDE, 1)
    authenticate(result)
    return result


def remove_projectile_function_query(source):
    found = authenticate(source)
    if not found or not found[3]:
        return source
    return source.replace(found[0], found[1], 1).replace(INCLUDE, '', 1)


def installed_entries(source):
    found = authenticate(source)
    if not found or not found[3]:
        raise ValueError('Complete projectile query default missing')
    return ['100e70']


def retag_specialized(source, output):
    found = authenticate(source)
    if not found or not found[3]:
        return output
    current = body(output)
    if entry_prefix(current) != found[3]:
        raise ValueError('Specializer moved the authenticated projectile query bridge')
    result = output.replace(current, current.replace(found[3], prefix(literal_format(current)), 1), 1)
    installed_entries(result)
    return result
