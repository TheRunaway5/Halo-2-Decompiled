"""Authenticate actual16E770 requested allocation and its real initializer."""
import hashlib,re
from native_selected_operand_binding import require_selected_identity
from native_datum_initialize_replacements import replace_datum_initialize,RESOLVER_PINS,INCLUDE

BODY_PINS=(
    'd138fd85fdb3100db554c9a3c4e04734d922179c59d6ccebff7ab7445617e9ec',
    '0abfec4ddaf3e90b2dd08096a6648ecc1bc940b797871c552ee22ba29fb1c1b0',
)
PREFIX='\n    if(halo2::native_datum_specific_detail::bridge(s,[](auto&state,std::uint32_t target,std::uint32_t site,bool tail){dispatch(state,target,site,tail);}))return;'
def canonical_body(body):
    start=body.index('{')+1
    return body[:start]+body[start+len(PREFIX):]if body[start:].startswith(PREFIX)else body

def replace_datum_specific(source):
    found=list(re.finditer(r'static void run_16e770\(WorldState &s\) \{.*?\n\}',source,re.S))
    if not found:return source
    if len(found)!=1:raise ValueError('Expected one complete16E770 requested allocation body')
    current=found[0][0];original=canonical_body(current)
    if hashlib.sha256(original.encode()).hexdigest()not in BODY_PINS:raise ValueError('Complete16E770 requested allocation body drift')
    require_selected_identity(source,original,RESOLVER_PINS)
    if replace_datum_initialize(source)!=source:raise ValueError('Native16E770 requires its authenticated native16E860 initializer')
    if current==original:
        start=current.index('{')+1;bound=current[:start]+PREFIX+current[start:]
        source=source[:found[0].start()]+bound+source[found[0].end():]
    if INCLUDE not in source:source=INCLUDE+source
    return source
