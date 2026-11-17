"""Private exact Arena source suffixes; no production installation API."""
import hashlib
import native_arena_initialize_replacements as original

SITES=(0x125281,0x125286,0x12528f,0x125290,0x1252a4,0x1252a9,0x1252b3,
       0x1252bf,0x1252c4,0x1252ca,0x1252d7,0x1252dc,0x1252ed,0x1252f2,
       0x1252f7,0x1252fd,0x125303,0x12530d,0x125312,0x12531d,0x125321,
       0x125326,0x12532e,0x125336,0x125337,0x125338,0x125339)
LABELS={pc:'native_arena_source_resume_'+format(pc,'x')+': 'for pc in SITES}
REPEAT='if(s.services.instruction)s.services.instruction(0x1252b8u);'
REP='while(s.gp[1]){w(s.memory,s.observe,0x1252b8u,s.gp[7],4,s.gp[0]);const auto delta=(s.other_flags&1024u)?4294967292u:4u;s.gp[7]+=delta;--s.gp[1];'+REPEAT+'}'
REP_LABEL='native_arena_source_repeat_callback: '
REP_BOUND=REP.replace('){w(','){{w(',1).replace(REPEAT,'}'+REP_LABEL+REPEAT,1)
PREFIX='\n    switch(native_arena_continuation_entry(s)){case 0u:return;'+''.join('case 0x'+format(pc,'x')+'u:goto '+label[:-2]+';'for pc,label in LABELS.items())+'case 0x1252b8u:goto '+REP_LABEL[:-2]+';default:throw std::logic_error("Foreign Arena source continuation");}'
BOUND_BODY_PIN='09994f3fb93f5c1c0f6006c32e26f37a917d35acae24a4c8deb9a872f1d780ba'

def digest(text):return hashlib.sha256(text.encode()).hexdigest()
def canonical_body(body):
    at=body.index('{')+1
    if not body[at:].startswith(PREFIX):return body
    if digest(body)!=BOUND_BODY_PIN:raise ValueError('Complete private Arena source suffix body drift')
    result=body[:at]+body[at+len(PREFIX):]
    if result.count(REP_BOUND)!=1:raise ValueError('Exact Arena repeat callback retirement seam drift')
    result=result.replace(REP_BOUND,REP,1)
    for pc,label in LABELS.items():
        callback='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        if result.count(label)!=1 or result.count(label+callback)!=1:raise ValueError('Relocated or missing Arena source suffix '+hex(pc))
        result=result.replace(label,'',1)
    if digest(result)!=original.PIN:raise ValueError('Complete original Arena source body differs after exact suffix reversal')
    return result
def body(source):
    # Authenticate the installed callback-free owner first. This private proof
    # neither changes nor relaxes its producer/capability or baseline pins.
    source=original.remove_arena_initialize(source);match,raw=original.authenticate(source)
    if raw.count(REP)!=1:raise ValueError('Exact original Arena repeat statement differs')
    bound=raw.replace(REP,REP_BOUND,1)
    for pc,label in LABELS.items():
        callback='if(s.services.instruction){s.services.instruction(0x'+format(pc,'x')+'u);}'
        if bound.count(callback)!=1:raise ValueError('Exact Arena source callback absent '+hex(pc))
        bound=bound.replace(callback,label+callback,1)
    at=bound.index('{')+1;bound=bound[:at]+PREFIX+bound[at:]
    if canonical_body(bound)!=raw:raise ValueError('Arena source continuation reconstruction differs')
    return raw,bound
