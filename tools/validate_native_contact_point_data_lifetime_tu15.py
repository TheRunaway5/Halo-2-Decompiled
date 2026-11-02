#!/usr/bin/env python3
"""Separate initialized-DATA lifetime proof; fullparent raw-before remains RED.

This does not change the strict all-events validator. Every raw preinitialization
delta is retained in the test report. The separate comparison proves complete
written DATA, persistent preimages, current callback payloads, and final banks,
under independent original incoming-GP and native private-DATA poison controls.
"""
import inspect
import sys
import validate_native_contact_point_removal_tu15 as parent


def lifetime_cases():
    rows=[case for case in parent.cases()if case['entry']==0x322f70 and not case['name'].startswith('fault_') and not case.get('required_owner',False)]
    # Ordinary original ABI controls, never read by the native kernel. Event
    # padding is held constant within each group, independently of GP poison.
    for count in (2,3):
        source=next(case for case in rows if case['name']==('parent_listeners_2'if count==3 else 'parent_'+str(count)+'_0'))
        import copy
        for poison in (0,0xa5a50000,0x5a5a0000):
            for private in (0,0xa5,0x5a):
                case=copy.deepcopy(source)
                case.update(name='lifetime_count'+str(count)+'_gp'+hex(poison)+'_data'+hex(private),
                            original_gp_poison=poison,native_private_seed=private,
                            padding_seed=0x3c,before_fp_input=[0x27f,0x4020,0x1fa0],
                            invariance_group='count'+str(count))
                rows.append(case)
    return rows


def summarize_invariance(rows,fixture):
    groups={}
    for case in rows:
        if 'invariance_group'not in case:continue
        signature=([parent.sha(fixture/bank['after_file'])for bank in case['banks']],case['allocator_calls'],case['after_fp'])
        group=groups.setdefault(case['invariance_group'],dict(signatures=[],initialization_preimages=[],cases=0))
        group['cases']+=1
        if signature not in group['signatures']:group['signatures'].append(signature)
        preimages=[event['before']for event in case['writes']if event['instruction']in(0x325db7,0x325dbb,0x325dbf,0x325dc3)]
        if preimages not in group['initialization_preimages']:group['initialization_preimages'].append(preimages)
    for group in groups.values():
        assert len(group.pop('signatures'))==1,'Original typed DATA/FP depends on incoming GP controls'
        group['all_original_after_data_callback_payloads_and_fp_equal']=True
        group['source_preinitialization_preimage_variants']=len(group['initialization_preimages'])
    return groups


def transform_original(source):
    source=source.replace("stack=bytearray(0x10000);struct.pack_into", "stack=bytearray(0x10000);stack[SP-STACK-18:SP-STACK-16]=bytes([case.get('padding_seed',all_cases.index(case)%256),0xa5]);struct.pack_into")
    source=source.replace("case['before_fp']=fp", "fp=case.get('before_fp_input',fp);case['before_fp']=fp")
    needle="        assembly.append(f'''.code32"
    assert source.count(needle)==1
    prepare=("        poison=case.get('original_gp_poison',0)\n"
             "        incoming='\\n'.join(f'mov ${((poison+n*0x12345)&0xffffffff)},%{register}'for n,register in enumerate(('eax','edx','ebx','ebp','esi','edi')))\n")
    source=source.replace(needle,prepare+needle)
    source=source.replace(" {prepare}\n mov ${SP},%esp", " {prepare}\n {incoming}\n mov ${SP},%esp")
    return source


def finalize_driver(driver):
    driver=driver.replace('std::cout<<J{{"passed",true}', 'const auto result=J{{"passed",true}')
    return driver.replace(".dump()<<'\\n';return 0;", ";std::ofstream(\"point-lifetime.json\")<<result.dump(2)<<'\\n';std::cout<<result.dump()<<'\\n';return 0;")


def main():
    source=inspect.getsource(parent.main)
    anchor="        source=source.replace(\"stack=bytearray(0x10000);struct.pack_into\", \"stack=bytearray(0x10000);stack[SP-STACK-18:SP-STACK-16]=bytes([all_cases.index(case)%256,0xa5]);struct.pack_into\")"
    assert source.count(anchor)==1
    source=source.replace(anchor,'        source=transform_original(source)')
    source=source.replace('all_cases=[case for case in cases()if not args.children_only or case[\'entry\']!=0x322f70]',
                          'all_cases=lifetime_cases()')
    anchor="        fixture,authority=environment['original'](image,{},all_cases,scratch)"
    assert source.count(anchor)==1
    source=source.replace(anchor,anchor+'\n        invariance=summarize_invariance(all_cases,fixture)')
    anchor="        driver='#include \"halo2/native_tu15_contact_point_removal.hpp\"\\n'+driver[:begin]+replacement+driver[end:]"
    assert source.count(anchor)==1
    extra='''
        driver=driver.replace('std::size_t cases{},stores{},read_count{},faults{},mutations{};', 'std::size_t cases{},stores{},read_count{},faults{},mutations{},raw_private_before_differences{};J raw_private_examples=J::array();')
        driver=driver.replace('memory.fault=c.value("fault",J{});memory.log_reads=read_pass;', 'memory.fault=c.value("fault",J{});memory.log_reads=read_pass;if(c.contains("native_private_seed")){auto it=memory.at(c.at("info"),16);std::fill_n(memory.banks.at(it->first).begin()+c.at("info").get<std::uint32_t>()-it->first,16,std::byte(c.at("native_private_seed").get<unsigned>()));}')
        first=driver.index('if(actual_stores!=c.at("writes"))')
        last=driver.index('stores+=actual_stores.size();',first)
        driver=driver[:first]+r\'''check(actual_stores.size()==c.at("writes").size(),name+" initialized-DATA store count differs");
        for(std::size_t n=0;n<actual_stores.size();++n){const auto&got=actual_stores[n];const auto&expected=c.at("writes")[n];
          check(got.at("instruction")==expected.at("instruction")&&got.at("address")==expected.at("address")&&got.at("written")==expected.at("written"),name+" source initialized-DATA store differs at "+std::to_string(n));
          if(got.at("before")!=expected.at("before")){
            const auto pc=got.at("instruction").get<std::uint32_t>(),address=got.at("address").get<std::uint32_t>(),info=c.at("info").get<std::uint32_t>();
            check(pc>=0x325db7u&&pc<=0x325dc3u&&(pc-0x325db7u)%4u==0&&address==info+(pc-0x325db7u)&&got.at("written")=="00000000",name+" persistent/public preimage differs");
            ++raw_private_before_differences;raw_private_examples.push_back(J{{"case",name},{"instruction",pc},{"address",address},{"native_before",got.at("before")},{"original_before",expected.at("before")}});
          }
        }\'''+driver[last:]
        driver=driver.replace('J{{"passed",true},{"cases",cases}', 'J{{"passed",true},{"initialized_data_lifetime_passed",true},{"strict_all_events_passed",raw_private_before_differences==0},{"raw_private_before_differences",raw_private_before_differences},{"raw_private_examples",raw_private_examples},{"cases",cases}')
        driver=finalize_driver(driver)
'''
    source=source.replace(anchor,anchor+extra)
    source=source.replace("'--jobs','1','--test-jobs','1']", "'--jobs','1','--test-jobs','1','--retain-report','point-lifetime.json']")
    source=source.replace("'tools/validate_native_contact_point_removal_tu15.py','tools/validate_native_array_growth_tu15.py',",
                          "'tools/validate_native_contact_point_removal_tu15.py','tools/validate_native_contact_point_data_lifetime_tu15.py','tools/validate_native_array_growth_tu15.py',")
    source=source.replace("scope='Declared valid point/map/listener DATA; all DATA bytes including preserved event padding and all ordered operands compared. Actual RET4/RET8 stubs are declared component services only. Remaining316AA0 stops at genuine entry; no full positive live point-removal claim.'",
                          "strict_all_events_passed=False,initialized_data_lifetime_passed=True,original_gp_and_native_private_data_invariance=invariance,last_point_world_update_return_qualified=False,scope='Separate complete initialized-DATA and persistent preimage comparison for normal nonlast-point returns. Fullparent strict all-events BEFORE-byte ledger remains RED and its raw differences are retained. Independent incoming original GP/native private-DATA controls do not supply source postimages. Declared RET4/RET8 component stubs only; actual callback lifetime, last-point316AA0 continuation and full live frame remain separate gates.'")
    environment=dict(parent.__dict__,lifetime_cases=lifetime_cases,transform_original=transform_original,summarize_invariance=summarize_invariance,finalize_driver=finalize_driver)
    exec(source,environment)
    return environment['main']()


if __name__=='__main__':
    raise SystemExit(main())
