"""Bind only the authenticated 3EB10 bodies in their original image owners."""
import hashlib
import re

OWNER = 'native_indexed_datum_gate_owner_detail'
BASENAME = 'native_tu15_indexed_datum_gate_source_owner.hpp'
INCLUDE = f'#include "gameplay/objects/{BASENAME}"\n'
ENTRY = '3eb10'
PRIMARY_RAW = 'ed2f524c9d73ccb277241b16f8ad038116dd68c3a8a9493d34cd1716437a68c4'
PARTICLE_RAW = '4d5fa54cdba4b296fbd4a5c9a5c8f214db1d3aad96b6df06f083bdf4ae2eaa8e'
DEATH_RAW = '23c0d446fa8388321a6b56e749b3f53a7c9dd6ee3cffac5c131e07c51a257f1c'
WORLD_PREPARED = '6421d3c97342a04660d6087f032a7f5ebd3cbc886e999db5693d53f92b8aac93'
WORLD_HEADER = 'static void run_3eb10(WorldState &s) {'
DEATH_HEADER = '[[gnu::noinline]] static void run_3eb10(State &s) {'
BODY = re.compile(r'^(' + re.escape(WORLD_HEADER) + '|' + re.escape(DEATH_HEADER) + r').*?^\}', re.M | re.S)
WORLD_STATE = re.compile(r'^struct LiveState : physics_world_runtime_detail::WorldState \{.*?^\};\nusing WorldState=LiveState;', re.M | re.S)
DEATH_STATE = re.compile(r'^struct State : physics_contact_runtime_detail::State \{.*?^\};', re.M | re.S)
SELECTED = re.compile(r'^std::uint32_t selected\([^\n]+', re.M)
SELECTED_OPERAND = re.compile(r'selected\((0x[0-9a-f]+u),s\.image\)')
WORLD_STATE_PINS = {
    '086a7e9e0eaa7ac687afb8b8094118d1211250698a0911a0beac4447b116ec46',
    '457d3eb1f274d921aee28c9dd02f321d20e0b442a4469a1734c55e74cd543575',
}
DEATH_STATE_PIN = 'c76b02b00b991ca6af9f81e5ace29ec73d56985f69269c8d7c1498486a6e92a6'
CONTEXTS = (
    ('physics_live_support_tu15.hpp', 'source_run_physics_live_support_tu15', (PRIMARY_RAW,), 'TitleUpdate15'),
    ('source_effect_particle_frame_tu15.hpp', 'source_run_effect_particle_frame_tu15', (PRIMARY_RAW,PARTICLE_RAW), 'TitleUpdate15'),
    ('death_startup_runtime.hpp', 'source_run_death_startup_bodies_1_1', (DEATH_RAW,), 'MapPack11'),
)


def sha(text):
    return hashlib.sha256(text.encode()).hexdigest()


def prefix(literal, image):
    tag='true' if literal else 'false'
    return f'\n    if(halo2::{OWNER}::bridge<{tag},SourcePhysicsContactImage::{image}>(s))return;'


def authenticate(source,*,transition=False):
    contexts=[(header,symbol,pins,image) for header,symbol,pins,image in CONTEXTS
              if 'void '+symbol+'(' in source]
    if not contexts:
        if OWNER in source or BASENAME in source:
            raise ValueError('Foreign 3EB10 binding outside an authenticated owner')
        return None
    if len(contexts)!=1:
        raise ValueError('Ambiguous 3EB10 module')
    header,symbol,raw_pins,image=contexts[0]
    death=image=='MapPack11'
    anchor=f'#include "halo2/{header}"\n'
    if source.count(anchor)!=1:
        raise ValueError('Missing exact 3EB10 owner header')
    rows=list(BODY.finditer(source))
    if len(rows)!=1 or rows[0][1]!=(DEATH_HEADER if death else WORLD_HEADER):
        raise ValueError('Missing or ambiguous complete 3EB10')
    match=rows[0];body=match[0];installed=None
    for literal in (False,True):
        marker=prefix(literal,image)
        if body.startswith(match[1]+marker):
            body=match[1]+body[len(match[1]+marker):]
            installed=literal
            break
    active=int(installed is not None)
    if source.count(OWNER)!=active or source.count(BASENAME)!=active or source.count(anchor+INCLUDE)!=active:
        raise ValueError('Unknown, duplicate, or moved 3EB10 binding')
    pinned=sha(body)
    literal=pinned==WORLD_PREPARED and not death
    if not literal and pinned not in raw_pins:
        raise ValueError('Complete retained 3EB10 body changed')
    if installed is not None and installed!=literal and not transition:
        raise ValueError('3EB10 callback transport tag changed')
    states=(DEATH_STATE if death else WORLD_STATE).findall(source)
    if len(states)!=1 or sha(states[0]) not in ({DEATH_STATE_PIN} if death else WORLD_STATE_PINS):
        raise ValueError('Correlated 3EB10 runtime state changed')
    _,_,export=source.partition('void '+symbol+'(')
    state_init=('AnimationSelectionState s{m,observe,retained,services};s.load(abi);' if death else
        'constexpr auto image=SourcePhysicsContactImage::TitleUpdate15;\n'
        '    FpScope scope(fp);LiveState s{m,observe,services,image};s.load(abi);')
    if state_init not in export[:800] or export.count('case 0x3eb10u:run_3eb10(s);break;')!=1:
        raise ValueError('3EB10 export image or route changed')
    if not death:
        resolvers=SELECTED.findall(source)
        if len(resolvers)!=1:
            raise ValueError('Missing 3EB10 image resolver')
        constants={'0x3eb10u',*SELECTED_OPERAND.findall(body)}
        pairs=re.findall(r'case (0x[0-9a-f]+u):return (0x[0-9a-f]+u);',resolvers[0])
        for token in constants:
            if [value for key,value in pairs if key==token]!=[token]:
                raise ValueError('3EB10 resolver operand is not exact identity: '+token)
    return match,body,literal,installed,anchor,image


def replace_indexed_datum_gate(source):
    found=authenticate(source)
    if found is None or found[3] is not None:
        return source
    match,body,literal,_,anchor,image=found
    bound=match[1]+prefix(literal,image)+body[len(match[1]):]
    result=(source[:match.start()]+bound+source[match.end():]).replace(anchor,anchor+INCLUDE,1)
    installed_entries(result)
    return result


def remove_indexed_datum_gate(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        return source
    match,body,_,_,_,_=found
    return (source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)


def canonical_body(body):
    for _,_,_,image in CONTEXTS:
        for literal in (False,True):
            marker=prefix(literal,image)
            if marker in body:
                return body.replace(marker,'',1)
    return body


def installed_entries(source):
    found=authenticate(source)
    if found is None or found[3] is None:
        raise ValueError('Complete 3EB10 native binding missing')
    return [ENTRY]


def retag_specialized(source,output):
    before=authenticate(source)
    after=authenticate(output,transition=True)
    if before is None or before[3] is None or after is None or not after[2] or after[3]!=before[3]:
        raise ValueError('Specialization dropped or moved the authenticated 3EB10 owner')
    match,body,literal,_,_,image=after
    replacement=match[1]+prefix(literal,image)+body[len(match[1]):]
    result=output[:match.start()]+replacement+output[match.end():]
    installed_entries(result)
    return result
