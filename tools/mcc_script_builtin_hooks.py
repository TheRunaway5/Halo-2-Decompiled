"""The two explicit native MCC getter seams in authenticated HS bodies."""
INVOKE = '''// Native MCC clock ownership precedes the Xbox opcode table. The
// existing caller frame and original211DA0 completion remain authoritative.
if(const auto native=native_mcc_script_builtin_value(s.memory,s.gp[0])) {
    const auto argument=r(s.memory,selected(0xcu,s.image)+s.gp[4],4);
    const auto opcode=sign_extend(r(s.memory,selected(0x2u,s.image)+s.gp[0],2),2);
    const auto thread=s.gp[7];
    if(s.services.instruction)s.services.instruction(0x211c18u);s.push(0x211c18u,argument);
    if(s.services.instruction)s.services.instruction(0x211c19u);s.push(0x211c19u,thread);
    if(s.services.instruction)s.services.instruction(0x211c1au);s.push(0x211c1au,opcode);
    if(s.services.instruction)s.services.instruction(0x211c1bu);s.push(0x211c1bu,selected(0x211c1eu,s.image));
    s.set(0,*native,4,0);s.set(1,thread,4,0);run_211da0(s);
    s.gp[4]+=12u; // Actual no-argument builtin wrapper RET12.
    goto L211c30;
}
'''
RETURN_TYPE = '''// Resolve the same full syntax receipt during original result coercion.
if(const auto native=native_mcc_script_builtin_return_type(s.memory,s.gp[1])) {
    s.set(2,*native,4,0);goto L211e03;
}
'''
INVOKE_ANCHOR = 'if(s.services.instruction){s.services.instruction(0x211c06u);}'
RETURN_ANCHOR = 'if(s.services.instruction){s.services.instruction(0x211de1u);}'
INCLUDE = '#include "halo2/native_mcc_script_builtin.hpp"\n'


def add_hooks(source):
    if INCLUDE in source:
        raise ValueError('MCC HS hooks already present')
    if INVOKE_ANCHOR not in source and RETURN_ANCHOR not in source:
        return source
    if source.count(INVOKE_ANCHOR) != 1 or source.count(RETURN_ANCHOR) != 1:
        raise ValueError('Authenticated HS hook sites differ')
    at = source.index('\nnamespace halo2 {')
    source = source[:at] + '\n' + INCLUDE.rstrip() + source[at:]
    return source.replace(INVOKE_ANCHOR, INVOKE + INVOKE_ANCHOR).replace(
        RETURN_ANCHOR, RETURN_TYPE + RETURN_ANCHOR)


def remove_hooks(source):
    if source.count(INVOKE) != 1 or source.count(RETURN_TYPE) != 1 or source.count(INCLUDE) != 1:
        raise ValueError('Generated MCC HS hooks differ')
    return source.replace(INVOKE, '').replace(RETURN_TYPE, '').replace(INCLUDE, '')
