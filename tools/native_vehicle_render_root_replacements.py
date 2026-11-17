"""Private strict VehicleUpdate BBE70/actual BCFB0 source binding candidate."""
import hashlib
import re

INCLUDE = '#include "rendering/models/native_tu15_vehicle_render_root_source_owner.hpp"\n'
LEGACY_INCLUDE = '#include "rendering/models/native_tu15_render_object_ownership_bridge.hpp"\n'
LEGACY_PREFIX = ('\n    if(s.image==SourcePhysicsContactImage::TitleUpdate15) {\n'
                 '        native_tu15_render_object_ownership_bridge(s,0xbcfb0u);return;\n'
                 '    }')
NAMESPACE = 'native_vehicle_render_root_detail'
INCLUDE_FILE = 'native_tu15_vehicle_render_root_source_owner.hpp'
MARKER = 'halo2::' + NAMESPACE + '::bridge'
BODY = re.compile(r'static void run_bbe70\(VehicleState &s\) \{.*?\n\}',re.S)
RESOLVER = re.compile(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}')
DISPATCHER = re.compile(r'static void dispatch\(VehicleState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}')
CONTEXTS = {
    ('d994bffa4b3bdfb01348123a547485a987341a1178043c81810e7cb716734aac',
     '1ad94227d3a1fefcce67499652d13611b97df348eef749f2bb19c323da59196a',
     'd52a1a4589a2d4a0179531ddc498125f06e394e36cb8df7f4a671c6a49692bdf'): False,
    ('f7d7fd3cb627cc7cc26f3df14dda3c919ffe4b02862e4eb2c2c730d5c2709052',
     '1ad94227d3a1fefcce67499652d13611b97df348eef749f2bb19c323da59196a',
     'a8a9f8851187b20c7e0ce414b04fcf6a586f28848eebe8d645c7f2472776bb13'): True,
}


def digest(text):
    return hashlib.sha256(text.encode()).hexdigest()


def prefix(literal):
    tag = 'true' if literal else 'false'
    return f'\n    if({MARKER}<{tag}>(s))return;'


def canonical_body(body):
    opening = body.index('{') + 1
    for literal in (False, True):
        bound = prefix(literal)
        if body.startswith(bound,opening):
            return body[:opening] + LEGACY_PREFIX + body[opening+len(bound):]
    return body


def authenticate(source, *, intermediate=False):
    found = list(BODY.finditer(source))
    if not found:
        if INCLUDE_FILE in source or NAMESPACE in source:
            raise ValueError('Vehicle render-root overlay has no VehicleUpdate owner')
        return None
    resolver, dispatcher = list(RESOLVER.finditer(source)), list(DISPATCHER.finditer(source))
    if len(found)!=1 or len(resolver)!=1 or len(dispatcher)!=1 or source.count(LEGACY_INCLUDE)!=1:
        raise ValueError('Ambiguous Vehicle render-root body, resolver, dispatcher or legacy include')
    match=found[0]; body=match[0]; opening=body.index('{')+1
    tags=[literal for literal in (False,True) if body.startswith(prefix(literal),opening)]
    if source.count(NAMESPACE)!=len(tags) or source.count(INCLUDE_FILE)!=len(tags) or \
       (tags and source.count(INCLUDE+LEGACY_INCLUDE)!=1):
        raise ValueError('Unowned or misplaced Vehicle render-root prefix/include')
    canonical=canonical_body(body)
    context=(digest(canonical),digest(resolver[0][0]),digest(dispatcher[0][0]))
    if context not in CONTEXTS:
        raise ValueError('Complete Vehicle render-root body/resolver/dispatcher drift')
    literal=CONTEXTS[context]
    if tags and tags[0]!=literal and not (intermediate and literal and tags==[False]):
        raise ValueError('Vehicle render-root callback format tag does not match the whole owner')
    return match,canonical,literal,bool(tags)


def replace_vehicle_render_root(source):
    found=authenticate(source)
    if not found:return source
    match,body,literal,bound=found
    if bound:return source
    opening=body.index('{')+1
    replacement=body[:opening]+prefix(literal)+body[opening+len(LEGACY_PREFIX):]
    result=(source[:match.start()]+replacement+source[match.end():]).replace(LEGACY_INCLUDE,INCLUDE+LEGACY_INCLUDE,1)
    authenticate(result)
    return result


def installed_entries(source):
    found=authenticate(source)
    return ['bbe70'] if found and found[3] else []


def rebind_prepared(source):
    found=authenticate(source,intermediate=True)
    if not found or not found[3]:return source
    match,_,literal,_=found
    if not literal:
        raise ValueError('Cannot retag unprepared Vehicle render-root owner')
    body=match[0]; opening=body.index('{')+1
    if body.startswith(prefix(False),opening):
        body=body[:opening]+prefix(True)+body[opening+len(prefix(False)):]
        source=source[:match.start()]+body+source[match.end():]
    authenticate(source)
    return source


def remove_vehicle_render_root(source):
    found=authenticate(source)
    if not found or not found[3]:return source
    match,canonical,_,_=found
    return (source[:match.start()]+canonical+source[match.end():]).replace(INCLUDE,'',1)
