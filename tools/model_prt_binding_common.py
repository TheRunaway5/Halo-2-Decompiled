"""Shared strict operand checks for authenticated TU1.5 model bodies."""
import re
import hashlib

SELECTED_SHA256='57359239fb680e03adea12fc0ce9e509fad6bef6ce05f9c03a9a37e7012d16db'


def require_identity_operands(source, body):
    operands={int(value,16) for value in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',body)}
    if not operands:return
    selected=re.search(r'std::uint32_t selected\(std::uint32_t p,SourcePhysicsContactImage image\)\{.*?\}\}',source,re.S)
    if not selected:raise ValueError('Model body has no authenticated TU1.5 selected operand map')
    if hashlib.sha256(selected.group().encode()).hexdigest()!=SELECTED_SHA256:
        raise ValueError('Authenticated model selected operand function changed')
    mapping={int(a,16):int(b,16) for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',selected.group())}
    if any(mapping.get(value)!=value for value in operands):
        raise ValueError('Native model selected operands are not TU1.5 identity mappings')
