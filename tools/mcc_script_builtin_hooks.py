"""Explicit native MCC getter and command seams in authenticated HS bodies."""
INVOKE = '''// Argumentful MCC commands use actual212010 scheduling. The signature is
// private to this invocation; original212010 retains progress in its thread
// frame, never this pointer (unchanged original lifetime oracle).
if(const auto command=native_mcc_script_builtin_command(s.memory,s.gp[0])) {
    const auto initial=r(s.memory,selected(0xcu,s.image)+s.gp[4],4);
    const auto opcode=sign_extend(r(s.memory,selected(0x2u,s.image)+s.gp[0],2),2);
    const auto thread=s.gp[7];
    if(s.services.instruction)s.services.instruction(0x211c18u);s.push(0x211c18u,initial);
    if(s.services.instruction)s.services.instruction(0x211c19u);s.push(0x211c19u,thread);
    if(s.services.instruction)s.services.instruction(0x211c1au);s.push(0x211c1au,opcode);
    if(s.services.instruction)s.services.instruction(0x211c1bu);s.push(0x211c1bu,selected(0x211c1eu,s.image));
    s.gp[4]-=4u;
    const auto signature=s.gp[4];
    w(s.memory,s.observe,0x211c1bu,signature,2,7u);
    s.push(0x211c1bu,initial);s.push(0x211c1bu,signature);
    s.push(0x211c1bu,1u);s.push(0x211c1bu,thread);
    s.push(0x211c1bu,selected(0x211c1eu,s.image));
    dispatch(s,selected(0x212010u,s.image),0x211c1bu,false);
    if(s.gp[0]) {
        const auto amount=std::bit_cast<std::int16_t>(std::uint16_t(r(s.memory,s.gp[0],2)));
        if(*command!=NativeMccScriptCommand::RasterizerBlur)
            throw std::runtime_error("Unowned MCC command receipt");
        native_mcc_legacy_rasterizer_blur(amount);
        s.set(0,0u,4,0);s.set(1,thread,4,0);
        s.push(0x211c1bu,selected(0x211c1eu,s.image));run_211da0(s);
    }
    s.gp[4]+=4u; // Expire this invocation's signature.
    s.pop();s.gp[4]+=12u; // Command wrapper RET12, including deferred calls.
    goto L211c30;
}
// Native MCC clock ownership precedes the Xbox opcode table. The
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
