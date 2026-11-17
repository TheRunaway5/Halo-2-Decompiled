"""Full actual313290 context pins; the World313290/actual316E10 is not this owner."""
import hashlib
import re
OWNER_MARKER='native_solver_hull_append_owner_detail'
BODY=re.compile(r'static void run_313290\(WorldState &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_solver_hull_append_owner_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_solver_hull_append_source_owner.hpp"\n'
BODY_PIN='46e4d78ccc0231ec938c131be02b5184f74acbd46a2926b6259f1d0d57fe10c9'
RESOLVER_PIN='1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'
DISPATCHERS=('5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0','aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849')
PREPARED_CONTEXTS=(
    ('b741acd0eadfd3cf8708eba1c51f4acb5a29eb72ac46bd55ca208fb5f315491b',RESOLVER_PIN,'37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'),
    ('b741acd0eadfd3cf8708eba1c51f4acb5a29eb72ac46bd55ca208fb5f315491b',RESOLVER_PIN,'8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'),
)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    at=body.index('{')+1
    return body[:at]+body[at+len(PREFIX):]if body[at:].startswith(PREFIX)else body
def context(source):
    bodies=list(BODY.finditer(source))
    if len(bodies)!=1:raise ValueError('Missing or ambiguous actual solver-hull append owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete solver-hull append context')
    return bodies[0],resolver[0],dispatcher[0]
def require_owned_markers(source,installed):
    if source.count(OWNER_MARKER)!=int(installed):raise ValueError('Foreign solver-hull append owner or continuation marker')
def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or OWNER_MARKER in source:raise ValueError('Missing actual solver-hull append owner')
        return None
    match,resolver,dispatcher=context(source);body=canonical_body(match[0])
    if digest(body)!=BODY_PIN or digest(resolver)!=RESOLVER_PIN or digest(dispatcher)not in DISPATCHERS:
        raise ValueError('Complete solver-hull append body/resolver/dispatcher drift')
    if source.count(INCLUDE)>1 or(PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete solver-hull append overlay')
    require_owned_markers(source,PREFIX in match[0])
    if 'selected('in body:raise ValueError('Actual solver-hull append literal identity drift')
    return match,body
def replace_solver_hull_append(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;at=body.index('{')+1;value=source[:match.start()]+body[:at]+PREFIX+body[at:]+source[match.end():]
    return value if INCLUDE in value else INCLUDE+value
def remove_solver_hull_append(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    match,resolver,dispatcher=context(source)
    if source.count(INCLUDE)!=1 or match[0].count(PREFIX)!=1:raise ValueError('Missing actual solver-hull append overlay')
    require_owned_markers(source,True)
    if tuple(digest(value)for value in(match[0],resolver,dispatcher))not in PREPARED_CONTEXTS:authenticate(source)
    return['313290']
