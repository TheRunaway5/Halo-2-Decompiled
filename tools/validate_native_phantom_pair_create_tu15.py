#!/usr/bin/env python3
"""Qualify actual31FFC0 notification dispatch with unchanged original RET8.

Reuses the strict physical phantom-dispatch ledger, including current reads,
callback input mutations, complete persistent DATA, FP and source RET4. The
declared callback is authentic8FDB0 RET8, not a substituted original result.
This does not qualify live phantom overlap insertion or listener callbacks.
"""
import inspect
import validate_native_phantom_pair_delete_tu15 as parent


def creation_cases():
    rows=parent.cases()
    for row in rows:
        row['entry']=0x31ffc0
        row['user_words']=[(offset-4 if offset in(0x520,0x560,0x5a0)else offset,
            0x8fdb0 if value==0x2e4d60 else value)for offset,value in row['user_words']]
        if 'fault'in row and row['fault']['address']in(parent.base.UI+0x520,parent.base.UI+0x560):
            row['fault']['address']-=4
        for mutation in row.get('mutations',[]):mutation['instruction']=0x8fdb0
        if 'stop_boundary'in row:row['stop_boundary']=0x8fdb0
    return rows


def replace_once(text,before,after):
    assert text.count(before)==1,(before,text.count(before))
    return text.replace(before,after)


def main():
    source=inspect.getsource(parent.main)
    source=source.replace('0x320040','0x31ffc0').replace('0x2e4d60','0x8fdb0')
    source=source.replace('NativeTu15PhantomPairDeleteServices','NativeTu15PhantomPairCreateServices')
    source=source.replace('native_tu15_delete_phantom_pair','native_tu15_create_phantom_pair')
    source=replace_once(source,'all_cases=cases();','all_cases=creation_cases();')
    source=replace_once(source,'const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u,4);',
        'const auto raw=memory(child.pid,std::uint32_t(regs.rsp)+4u,8);')
    source=replace_once(source,'allocator_calls.push_back(J{{"owner",pc},{"receiver",std::uint32_t(regs.rcx)},{"other",value}});',
        'std::uint32_t flag{};for(unsigned n=0;n<4;++n)flag|=std::to_integer<unsigned>(raw[4+n])<<(8u*n);allocator_calls.push_back(J{{"owner",pc},{"receiver",std::uint32_t(regs.rcx)},{"other",value},{"flag_word",flag}});')
    source=replace_once(source,'services.remove_overlap=[&](auto self,auto owner,auto other,auto&){',
        'services.add_overlap=[&](auto self,auto owner,auto other,auto flag_word,auto&){')
    source=replace_once(source,'callbacks.push_back(J{{"owner",owner.identity},{"receiver",self},{"other",other.identity}});',
        'callbacks.push_back(J{{"owner",owner.identity},{"receiver",self},{"other",other.identity},{"flag_word",flag_word}});')
    source=replace_once(source,'native_tu15_ignore_removed_phantom_overlap();','/* Authenticated8FDB0 RET8 consumes no DATA or FP. */')
    source=replace_once(source,"'src/physics/native_tu15_phantom_pair_delete.cpp','tools/validate_native_phantom_pair_delete_tu15.py'",
        "'src/physics/native_tu15_phantom_pair_delete.cpp','tools/validate_native_phantom_pair_delete_tu15.py','tools/validate_native_phantom_pair_create_tu15.py'")
    source=replace_once(source,"('wrong_other','{other},fp);','{other+4u},fp);')",
        "('wrong_other','{other},phantom&0xffffff00u,fp);','{other+4u},phantom&0xffffff00u,fp);'),('flag_word','phantom&0xffffff00u','0u')")
    source=replace_once(source,
        "scope='Declared valid model pairs/type2 phantom dispatch with authentic original RET4 no-op; mutable current DATA controls after first genuine callback; exact local liveness/ordinaryRET4, not actual live nestedowner admission'",
        "scope='Actual54-site31FFC0 parent over declared valid pair/type2 DATA; authentic8FDB0 RET8 callbacks, full notification argument word, current rereads after input mutations, complete persistent fields/FP and RET4. No actual live overlap-addition or listener admission.'")
    environment=dict(parent.__dict__,creation_cases=creation_cases,__doc__=__doc__)
    exec(source,environment)
    return environment['main']()


if __name__=='__main__':raise SystemExit(main())
