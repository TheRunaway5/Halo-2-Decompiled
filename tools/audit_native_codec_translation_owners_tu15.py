#!/usr/bin/env python3
"""Exact private2961F0 original/current/fresh/prepared identity audit."""
import argparse,hashlib,json,os,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
import native_packed_rotation_replacements as packed
import native_codec_translation_replacements as binding
from native_packed_rotation_source_inputs import UNITS,authenticate,function,digest
from validate_native_packed_rotation_generation_tu15 import parts,dispatcher_map
from research_mp_sandbox_update_functions import NEW_DEFAULT,NEW_PIN
from oracle_smg_firing_1_1 import Xbe,_Code,_trace
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--output',type=Path,required=True);args=parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):parser.error('Use tools/run_test.py')
    import platform
    if platform.system()!='Linux' or platform.machine().lower()not in('x86_64','amd64'):return 77
    scratch=Path(os.environ['HALO2_TEST_SCRATCH']);args.output.mkdir(parents=True,exist_ok=False)
    report=dict(passed=False,private_unbound=True,owned_scratch=str(scratch),retained_large_artifacts=[],numeric_source_route_execution_qualified=False,variants=[])
    try:
        command=[sys.executable,'-B',str(ROOT/'tools/validate_native_packed_rotation_generation_tu15.py'),'--report',str(args.output/'full-generation.json'),'--candidate-preparation','--require-binding'];done=subprocess.run(command,capture_output=True,text=True)
        if done.returncode:raise ValueError('Genuine codec full generation failed '+(done.stdout+done.stderr)[-6000:])
        image=Xbe(NEW_DEFAULT)
        if hashlib.sha256(Path(NEW_DEFAULT).read_bytes()).hexdigest()!=NEW_PIN:raise ValueError('Original code image drift')
        code=_Code(image,'.text');original=_trace(code,0x2961f0,{0x2961f0});sites=sorted(original.code)
        if len(sites)!=15:raise ValueError('Complete original codec instruction set changed')
        report['original_sites']=[dict(instruction=hex(pc),bytes=bytes(code.at(pc).bytes).hex(),mnemonic=code.at(pc).mnemonic,operands=code.at(pc).op_str)for pc in sites]
        report['original_code_sha256']=hashlib.sha256(b''.join(bytes(code.at(pc).bytes)for pc in sites)).hexdigest()
        for owner,name in zip(('primary','ai','model'),UNITS):
            current=(ROOT/name).read_text();index,context=authenticate(current);binding.authenticate(current);baseline=binding.canonical_body(function(current,'run_2961f0'));projection=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',baseline)
            for stage in('current','fresh'):
                for form in('bound','prepared'):
                    text=(scratch/(owner+'-'+stage+'-'+form+'.cpp')).read_text();binding.authenticate(text);subject=binding.canonical_body(function(text,'run_2961f0'));_,resolver,dispatcher,state=parts(text)
                    calls=re.findall(r'(?:s\.services\.instruction|s\.instruction_literal)\((?:selected\()?0x([0-9a-f]+)u',subject)
                    if [int(value,16)for value in calls]!=sites:raise ValueError('Original codec site order changed')
                    normalized=re.sub(r'selected\((0x[0-9a-f]+u),s\.image\)',r'\1',subject);normalized=re.sub(r's\.instruction_literal\((0x[0-9a-f]+u)\);',r'if(s.services.instruction){s.services.instruction(\1);}',normalized)
                    if normalized!=projection:raise ValueError('Codec body changed beyond exact selected/literal projection')
                    mapping={int(a,16):int(b,16)for a,b in re.findall(r'case 0x([0-9a-f]+)u:return 0x([0-9a-f]+)u;',resolver)}
                    for value in re.findall(r'selected\(0x([0-9a-f]+)u,s.image\)',subject):
                        if mapping.get(int(value,16))!=int(value,16):raise ValueError('Codec selected data/site is not exact identity')
                    if form=='bound':
                        if dispatcher_map(resolver,dispatcher).get(0x2961f0)!='2961f0':raise ValueError('Codec complete raw dispatch target changed')
                    else:
                        # Prepared Primary includes its previously qualified
                        # native shim. The entire existing tuple remains pinned;
                        # authenticate the exact direct clause without parsing
                        # that prepared function as raw-dispatch grammar.
                        if packed.installed_entries(text)!=['296180']:raise ValueError('Exact existing prepared context changed')
                        clause=('if(target==0x2961f0u){run_2961f0(s);return;}'if owner!='primary'and stage=='current'else'case 0x2961f0u:run_2961f0(s);return;')
                        if dispatcher.count(clause)!=1:raise ValueError('Prepared exact codec direct clause missing/ambiguous')
                    report['variants'].append(dict(owner=owner,stage=stage,form=form,complete_body_sha256=digest(subject),complete_resolver_sha256=digest(resolver),complete_dispatcher_sha256=digest(dispatcher),complete_state_sha256=digest(state),original_site_order_equal=True,complete_identity_projection_equal=True,prepared_context_uses_exact_existing_tuple=form=='prepared',numeric_source_route_execution_qualified=False))
        report.update(passed=True,source_owners=list(UNITS),whole_cpp_syntax_variants=12,genuine_generator_command=command,prior_dispatch_shape_diagnostics=['native-codec-translation-owner-audit-20261110/report.json','native-codec-translation-owner-audit-prepared-fixed-20261110/report.json'],audit_tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest())
    except BaseException as error:report['error']=repr(error);raise
    finally:(args.output/'report.json').write_text(json.dumps(report,indent=2)+'\n')
    return 0 if report['passed']else 1
if __name__=='__main__':raise SystemExit(main())
