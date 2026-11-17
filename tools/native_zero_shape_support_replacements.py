"""Full actual2E8D20 zero-support context pins; local only in Primary."""
import hashlib
import re
OWNER_MARKER='native_zero_shape_support_owner_detail'
BODY=re.compile(r'static void run_2e8d20\(WorldState &s\) \{.*?\n\}',re.S)
PREFIX='\n    if(halo2::native_zero_shape_support_owner_detail::bridge(s))return;'
INCLUDE='#include "physics/runtime/native_tu15_zero_shape_support_source_owner.hpp"\n'
BODY_PIN='9c6c29faa6b324f6bad5839cd47aa4bc7c15e8f7f552e4f80612b401f998ad32'
RESOLVER_PIN='1d97a2827ec4426bdd7077b006ea21a2adf19d9020e35fe91ade4be3f357bdb4'
DISPATCHERS=('5901d2c9c67421f4dabe988f833de38ce4a22dd4cb0778fbf19eb0fd0cb55ce0','aba69140bdfbfe48111ef3727b7bb22a4891a8daa06770aabcb75183386d5849')
PREPARED_CONTEXTS=(
    ('d2791af58c70ed8214c51b31479df9f06b61f2bc60cb02cf75809de84b6e9023',RESOLVER_PIN,'37477844625e78f7f747e917b6bc039f03390bfaa5419a9843557f0af9e17052'),
    ('d2791af58c70ed8214c51b31479df9f06b61f2bc60cb02cf75809de84b6e9023',RESOLVER_PIN,'8aee54bced297db8c7b162c84479071889c2bfc03472376f8a6cba0b7f68b851'),
)
def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    at=body.index('{')+1
    return body[:at]+body[at+len(PREFIX):]if body[at:].startswith(PREFIX)else body
def context(source):
    bodies=list(BODY.finditer(source))
    if len(bodies)!=1:raise ValueError('Missing or ambiguous actual zero-shape support owner')
    resolver=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{[^\n]*\}',source)
    dispatcher=re.search(r'static void dispatch\(WorldState &s,std::uint32_t target,std::uint32_t site,bool tail\)\{[^\n]*\}',source)
    if not resolver or not dispatcher:raise ValueError('Missing complete zero-shape support context')
    return bodies[0],resolver[0],dispatcher[0]
def require_owned_markers(source,installed):
    if source.count(OWNER_MARKER)!=int(installed):raise ValueError('Foreign zero-shape support owner or continuation marker')
def authenticate(source):
    if not BODY.search(source):
        if INCLUDE in source or OWNER_MARKER in source:raise ValueError('Missing actual zero-shape support owner')
        return None
    match,resolver,dispatcher=context(source);body=canonical_body(match[0])
    if digest(body)!=BODY_PIN or digest(resolver)!=RESOLVER_PIN or digest(dispatcher)not in DISPATCHERS:
        raise ValueError('Complete zero-shape support body/resolver/dispatcher drift')
    if source.count(INCLUDE)>1 or(PREFIX in match[0])!=(INCLUDE in source):raise ValueError('Incomplete zero-shape support overlay')
    require_owned_markers(source,PREFIX in match[0])
    if 'selected('in body:raise ValueError('Actual zero-shape support literal identity drift')
    return match,body
def replace_zero_shape_support(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;at=body.index('{')+1;value=source[:match.start()]+body[:at]+PREFIX+body[at:]+source[match.end():]
    return value if INCLUDE in value else INCLUDE+value
def remove_zero_shape_support(source):
    found=authenticate(source)
    if not found:return source
    match,body=found;return(source[:match.start()]+body+source[match.end():]).replace(INCLUDE,'',1)
def installed_entries(source):
    match,resolver,dispatcher=context(source)
    if source.count(INCLUDE)!=1 or match[0].count(PREFIX)!=1:raise ValueError('Missing actual zero-shape support overlay')
    require_owned_markers(source,True)
    if tuple(digest(value)for value in(match[0],resolver,dispatcher))not in PREPARED_CONTEXTS:authenticate(source)
    return['2e8d20']
