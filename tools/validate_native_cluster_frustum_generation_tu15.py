#!/usr/bin/env python3
"""Whole particle regeneration with strict167EF0 and existing184690 owners."""
import argparse,hashlib,json,os,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--report',type=Path,required=True);parser.add_argument('--compiler',default='clang++');args=parser.parse_args()
    scratch=Path(os.environ['HALO2_TEST_SCRATCH'])
    import validate_native_sphere_frustum_generation_tu15 as classifier
    import native_cluster_frustum_replacements as binding
    from generated_routine_ownership import function_bodies
    public_path=ROOT/'src/rendering/world/source_visibility.cpp';public_before=public_path.read_bytes()
    if binding.replace_world_visibility(public_before.decode())!=public_before.decode():raise ValueError('Production shared cluster visibility facade missing')
    original_argv=sys.argv
    try:
        sys.argv=[__file__,'--report',str(scratch/'sphere-generation.json'),'--compiler',args.compiler]
        status=classifier.main()
    finally:sys.argv=original_argv
    if status:return status
    base=json.loads((scratch/'sphere-generation.json').read_text());raw=(scratch/'particle.cpp').read_text();prepared=(scratch/'particle-specialized.cpp').read_text();canonical=(ROOT/classifier.UNIT).read_text()
    target=lambda source:next(body for signature,body in function_bodies(source)if signature[1]=='167ef0')
    for text in(canonical,raw):
        if binding.replace_cluster_frustum(text)!=text or binding.PREFIX not in target(text):raise ValueError('Native167EF0 raw source route missing')
        if binding.digest(binding.canonical_body(target(text)))!=binding.BODY_PIN:raise ValueError('Original complete167EF0 continuation drift')
    if target(raw)!=target(canonical):raise ValueError('Fresh167EF0 body differs from production')
    if binding.PREFIX not in target(prepared)or's.instruction_literal('not in target(prepared):raise ValueError('Prepared167EF0 native/retained route missing')
    # Output-only witness derived from the strict raw installer and qualified
    # physical source-copy proof. It does not expand raw installer admission.
    if binding.digest(target(prepared))!='34915aa69c65507212624ba463e3a3ccf527d91e2836022c4065741a8b5a5dce':raise ValueError('Prepared complete167EF0 target drift')
    if public_before!=public_path.read_bytes():raise ValueError('Public cluster facade changed during generation')
    base.update(cluster_parent_body_identical=True,cluster_native_prefix_preserved=True,original_cluster_body_sha256=binding.BODY_PIN,prepared_cluster_body_sha256=binding.digest(target(prepared)),shared_public_source_sha256=hashlib.sha256(public_before).hexdigest(),scope='Complete raw/prepared particle regeneration and syntax plus strict167EF0/184690 identities; execution and public DATA proofs are separately registered')
    args.report.parent.mkdir(parents=True,exist_ok=True);args.report.write_text(json.dumps(base,indent=2)+'\n');return 0
if __name__=='__main__':raise SystemExit(main())
