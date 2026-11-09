#!/usr/bin/env python3
"""Relocate the frozen V3 accepted listener to actual moving-owned DATA."""
from pathlib import Path
import argparse,hashlib,inspect,json,os,shutil,subprocess,sys
ROOT=Path(__file__).resolve().parents[1]
MATRIX=ROOT/'analysis/physics/native-accepted-body-contact-scalar-controls1481-v3-20261103-first'
MATRIX_PIN='b43bf04d47dad0c8c0badff82c0c32cfd29555ecefeae504355ba391f7f1b924'
ARCHIVE_PIN='e192c756d6d60fdaffe34189dabe4c58b86856b428060cdf3285048162ed09f7'
LAYOUT=ROOT/'analysis/physics/native-accepted-body-contact-world-planning-20261103/RESOURCE_LAYOUT.json'
def sha(p):
    with Path(p).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def save(p,d):Path(p).write_text(json.dumps(d,indent=2,sort_keys=True)+'\n')

def relocated_driver(text,layout):
    def replace(old,new):
        nonlocal text
        assert text.count(old)==1,(old[:90],text.count(old));text=text.replace(old,new,1)
    offsets={r['name']:r['offset'] for r in layout['fields']}
    addr=lambda n:f'p+0x{offsets[n]:x}u'
    weights=','.join(addr('weights.'+n)for n in ('first_weight','second_weight','second_mass','first_mass'))
    material=','.join(addr('accepted.'+n)for n in ('first_material','second_material','first_coefficient','second_coefficient','material_first0','material_first1','material_second0','material_second1'))
    request=addr('accepted.constraint_request.identity');velocity=addr('accepted.body_update.point_velocity');impulse=addr('accepted.body_update.impulse')
    replace('constexpr std::uint32_t p=0x73000000u;const std::array<NativeTu15ContactPrivateRange,1>owned{{{p,0x1000u}}};NativeTu15ContactCandidatePrivateData view(memory,owned,{});',
        '''constexpr std::uint32_t p=0x600e0000u;const auto moving_before=native.read(p,0x8000u);
   const std::array<NativeTu15ContactPrivateRange,1>owned{{{p,0x8000u}}};NativeTu15ContactCandidatePrivateData view(memory,owned,{});
   for(unsigned i=0;i<0x8000u;++i)check(!view.data_is_defined(p+i,1u),"Moving private DATA began provided");''')
    replace('view.write(p+0x800u,id);view.write(p+0x804u,fields);',
        '''view.write(p+0x5030u,id);view.write(p+0x5034u,fields);
   for(unsigned i=0;i<0x8000u;++i){const bool event=(i>=0x5030u&&i<0x5032u)||(i>=0x5034u&&i<0x5054u);
    check(view.data_is_defined(p+i,1u)==event,"A non-event private byte was supplied");}''')
    old='   NativeTu15AcceptedContactScratch scratch{p+0x100u,p+0x104u,p+0x108u,p+0x10cu,p+0x110u,p+0x114u,p+0x118u,p+0x11cu,{p+0x200u},{p+0x300u,p+0x310u}};'
    # The second occurrence belongs to an independent generic model, not the actual branch.
    replace(old,'   NativeTu15AcceptedContactScratch scratch{'+material+',{'+request+'},{'+velocity+','+impulse+'}};')
    replace('view,typed.at("listener"),p+0x800u,{{p,p+4u,p+8u,p+12u},scratch},fp',
        'view,typed.at("listener"),p+0x5030u,{{'+weights+'},scratch},fp')
    replace('   if(mutation.empty()&&!failed){unsigned guards{};', '   check(native.read(p,0x8000u)==moving_before,"Underlying moving32KiB changed");\n   if(mutation.empty()&&!failed){unsigned guards{};')
    a=text.index('    for(auto [offset,n]:{std::pair{0x802u,2u}');b=text.index('    report["definition_guard_controls"]=guards;',a)
    text=text[:a]+'''    for(auto [offset,n]:{std::pair{0x5032u,2u},std::pair{0x612cu,4u},std::pair{0x6190u,1u},
      std::pair{0x7000u,4u},std::pair{0x6131u,3u},std::pair{0x613eu,2u},std::pair{0x614au,2u},
      std::pair{0x6169u,1u},std::pair{0x616eu,2u}})
      refuses([&]{std::array<std::byte,4>b{};view.read(p+offset,std::span(b).first(n));});
    refuses([&]{view.require_retained_extent(p+0x6110u,2u);});
    refuses([&]{view.require_retained_extent(p+0x7fffu,2u);});
    const auto first_body=capture.at("event_fields").at("current_body").get<std::uint32_t>();const auto before=native.read(first_body,4u);
    refuses([&]{view.copy_defined_data(0x1d76b6u,first_body,p+0x612cu,4u);});check(native.read(first_body,4u)==before,"Undefined escape changed retained storage");
    const std::array<NativeTu15ContactPrivateRange,0>none{};NativeTu15ContactCandidatePrivateData nested(view,none,{});
    refuses([&]{std::array<std::byte,4>b{};nested.read(p+0x612cu,b);});
    check(view.data_is_defined(p+0x6100u,4u)&&view.withhold_defined_data(p+0x6100u,1u),"Private lifetime invalidation declined");
    refuses([&]{std::array<std::byte,4>b{};view.read(p+0x6100u,b);});
    check(native.read(p,0x8000u)==moving_before,"Resource guard controls changed underlying moving bank");
'''+text[b:]
    replace('{"captured_bytes_excluded",0},{"native_fp",J::array',
        '{"captured_bytes_excluded",0},{"underlying_moving_bytes_compared",0x8000u},{"underlying_moving_unchanged",true},{"native_fp",J::array')
    return text

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--archive',type=Path,required=True);parser.add_argument('--output',type=Path,required=True);a=parser.parse_args();out=a.output.resolve();out.mkdir(parents=True,exist_ok=False);owner=Path(os.environ['HALO2_TEST_SCRATCH']).resolve();r=dict(passed=False,owned_scratch=str(owner),retained_large_artifacts=[],supplied_original_outcomes=False,captured_gp_inputs=False,scope='Exact V3 component resource relocation only. No World admission or arbitrary-role alias claim.')
    try:
        matrix_path=MATRIX/'report.json';assert sha(matrix_path)==MATRIX_PIN;matrix=json.loads(matrix_path.read_text());assert matrix['passed'] and not Path(matrix['owned_scratch']).exists();frozen=MATRIX/'source'
        pins=matrix['compiled_source_pins'];tools=matrix['tool_pins'];assert len(pins)==53 and len(tools)==9
        for n,h in {**pins,**tools}.items():assert sha(frozen/n)==h,n
        old_driver=MATRIX/'compiled-driver.cpp';assert sha(old_driver)==matrix['compiled_driver_sha256'];metadata=MATRIX/'original-metadata.json';assert sha(metadata)==matrix['original_metadata_sha256']
        assert sha(a.archive)==ARCHIVE_PIN;layout=json.loads(LAYOUT.read_text());assert len(layout['fields'])==15 and layout['defined_field_bytes']==140
        driver=relocated_driver(old_driver.read_text(),layout)
        sys.path[:0]=[str(frozen/'tools'),str(ROOT/'tools')]
        from native_created_body_contact_before_input_tu15 import NativeCreatedBodyContactBeforeInput
        from build_native_tu15_constraint_before_reference import build_constraint_reference
        from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
        assert sha(NEW_DEFAULT)==NEW_PIN=='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd'
        before=NativeCreatedBodyContactBeforeInput(a.archive,ARCHIVE_PIN,owner/'before')
        # The reference loader is exactly the matrix's declared FS setup, not a kernel patch.
        code=inspect.getsource(build_constraint_reference);needle=' mov %ax,%fs\n';assert code.count(needle)==1
        code=code.replace(needle,needle+' movl $0xffffffff,%fs:0\n');env=dict(build_constraint_reference.__globals__);exec(code,env);authority=env['build_constraint_reference'](before,owner/'reference');elf=Path(authority['elf'])
        original_meta=json.loads(metadata.read_text());original_meta['declared_scalar_controls']=[];save(owner/'metadata.json',original_meta)
        project=owner/'source';project.mkdir()
        for n,h in pins.items():
            p=project/n;p.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(frozen/n,p);assert sha(p)==h
        (project/'driver.cpp').write_text(driver);(out/'compiled-driver.cpp').write_text(driver)
        units=sorted(n for n in pins if n.endswith('.cpp'));assert len(units)==15
        r.update(matrix_sha256=MATRIX_PIN,compiled_source_pins=pins,reference_tool_pins=tools,resource_layout_sha256=sha(LAYOUT),driver_sha256=sha(project/'driver.cpp'),original_operand_metadata_sha256=sha(metadata),actual_only_metadata_sha256=sha(owner/'metadata.json'),before_input=before.receipt,reference_authority=authority,source_event=0x72008000,native_moving_base=0x600e0000,native_event=0x600e5030,initially_provided_native_event_bytes=34,other_native_moving_bytes_initially_unavailable=True,source_and_native_bodies_unchanged=True)
        r['results']=[]
        for cc in ['g++','clang++']:
            exe=owner/('moving-resource-'+cc);cmd=[cc,'-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-frounding-math','-ffunction-sections','-fdata-sections','-I'+str(project/'include'),'-I'+str(project/'src/physics'),str(project/'driver.cpp'),*[str(project/n)for n in units],'-Wl,--gc-sections','-o',str(exe)]
            build=subprocess.run(cmd,capture_output=True,text=True);(out/(cc+'-build.log')).write_text(build.stdout+build.stderr);assert build.returncode==0,build.stderr[-4000:]
            result=out/(cc+'-actual.json');run=subprocess.run([str(exe),str(elf),str(before.path/'capture-native-before.json'),str(owner/'metadata.json'),str(result),'0,1,2,3'],env={k:v for k,v in os.environ.items()if k!='HALO2_ACCEPTED_PARENT_FAULTS'});assert run.returncode==0,'Moving resource actual proof differs'
            d=json.loads(result.read_text());assert d['passed'] and len(d['profiles'])==4 and d['definition_guard_controls']==14 and d['production_guard_models']==14
            for p in d['profiles']:
                assert p['passed'] and p['ordered_reads']==244 and p['ordered_stores']==30 and len(p['passes'])==3 and p['declared_scalar_control'] is None
                for x in p['passes']:
                    assert x['compared_bytes']==89795481 and x['captured_bytes_excluded']==0 and x['underlying_moving_bytes_compared']==32768 and x['underlying_moving_unchanged']
                    if x['observed']:assert x['source_pc_events']==30
            r['results'].append(dict(compiler=cc,exit_code=0,report_sha256=sha(result),profiles=4,all_retained_ram_fp=True,ordered_reads=244,exact_source_pc_events=30,underlying_moving_bytes_unchanged=32768,definition_guard_controls=14,generic_binding_models=14))
            print(cc,'four actual relocation profiles GREEN',flush=True)
        for n,h in pins.items():assert sha(project/n)==h and sha(frozen/n)==h,n
        for n,h in tools.items():assert sha(frozen/n)==h,n
        r.update(passed=True,source_inputs_unchanged=True,proves_genuine_input_resource_disjointness_only=True,dynamic_retained_role_revision_still_required=True,no_world_or_live_admission=True)
    except BaseException as e:r['error']=str(e)or repr(e);raise
    finally:r['allocated_bytes_before_cleanup']=sum(p.stat().st_blocks*512 for p in owner.rglob('*')if p.is_file());save(out/'report.json',r)
if __name__=='__main__':main()
