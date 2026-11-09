"""Authenticate a complete selected-operand resolver before native binding."""
import hashlib
import re


def require_selected_identity(source,body,pins):
    operands={int(value,16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',body)}
    if not operands:return
    selected=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not selected or hashlib.sha256(selected[0].encode()).hexdigest() not in pins:
        raise ValueError('Complete selected operand resolver changed before native binding')
    mapping={int(a,16):int(b,16) for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',selected[0])}
    if any(mapping.get(value)!=value for value in operands):
        raise ValueError('Native TU1.5 body has a nonidentity selected operand')
