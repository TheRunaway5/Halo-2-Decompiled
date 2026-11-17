#!/usr/bin/env python3
"""Prove installed lookup format tags against their complete retained owners."""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import platform
import re
import subprocess

from specialize_native_operands import specialize
from validate_native_object_body_frame_tu15 import corpus
from validate_native_object_body_source_frame_tu15 import prepare as prepare_reference

ROOT = Path(__file__).resolve().parents[1]
FAMILIES = (
    ('native_object_body_replacements', 'replace_object_body', 'remove_object_body', 'src/gameplay/units/source_unit_view_reset_tu15_generated.cpp'),
    ('native_object_material_lookup_replacements', 'replace_object_material_lookup', 'remove_object_material_lookup', 'src/gameplay/objects/source_object_material_tu15_generated.cpp'),
    ('native_object_lookup_copies_replacements', 'replace_object_lookup_copies', 'remove_object_lookup_copies', 'src/rendering/renderer/source_scene_device_constructor_tu15_generated.cpp'),
    ('native_object_lookup_copies_replacements', 'replace_object_lookup_copies', 'remove_object_lookup_copies', 'src/gameplay/objects/source_sandbox_special_tu15_generated.cpp'),
    ('native_object_lookup_copies_replacements', 'replace_object_lookup_copies', 'remove_object_lookup_copies', 'src/rendering/effects/source_effect_particle_draw_tu15_generated.cpp'),
    ('native_object_lookup_copies_replacements', 'replace_object_lookup_copies', 'remove_object_lookup_copies', 'src/campaign/ai/source_ai_frame_tu15_generated.cpp'),
    ('native_object_lookup_canonical_replacements', 'replace_canonical_object_lookup', 'remove_canonical_object_lookup', 'src/gameplay/units/source_direct_damage_runtime_generated.cpp'),
    ('native_object_lookup_canonical_replacements', 'replace_canonical_object_lookup', 'remove_canonical_object_lookup', 'src/gameplay/units/source_vitality_runtime_generated.cpp'),
)
HEADERS = (
    'native_tu15_object_body_source_owner.hpp',
    'native_tu15_object_material_lookup_source_owner.hpp',
    'native_tu15_object_lookup_copies_source_owner.hpp',
    'native_tu15_object_body_canonical_source_owner.hpp',
    'native_tu15_object_body_source_frame.hpp',
    'native_tu15_object_body_canonical_sites.hpp',
    'native_tu15_object_body_frame.hpp',
)
BODY = re.compile(r'^static void run_(?:bcdf0|bbcb0)\(WorldState &s\) \{.*?^\}', re.M | re.S)
RAW = re.compile(r'if\(s\.services\.instruction\)\{s\.services\.instruction\((0x[0-9a-f]+u)\);\}')
LITERAL = re.compile(r's\.instruction_literal\((0x[0-9a-f]+u)\);')
HARNESS = 'tests/native_tu15_object_body_source_frame_tests.cpp'
HARNESS_PIN = 'eb85f7413eed1810fe78ee25da39447d29c98ba93054dca6e32193eb5ad78687'


def digest(data):
    return hashlib.sha256(data).hexdigest()


def load_helper(name):
    spec = importlib.util.spec_from_file_location('installed_format_' + name, ROOT / 'tools' / (name + '.py'))
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    return helper


def refused(api, source, label):
    try:
        api(source)
    except ValueError:
        return
    raise ValueError('Installed lookup API accepted ' + label + ' through ' + api.__name__)


def contexts():
    rows, blocks = [], []
    for family, install, remove, unit in FAMILIES:
        helper = load_helper(family)
        current = (ROOT / unit).read_text()
        helper.installed_entries(current)
        original = getattr(helper, remove)(current)
        if getattr(helper, install)(current) != current:
            raise ValueError('Installed format binding is not idempotent ' + unit)
        for form, source in (('raw', current), ('prepared', specialize(current)[0])):
            helper.installed_entries(source)
            body = BODY.findall(source)
            if len(body) != 1:
                raise ValueError('Missing exact installed lookup body ' + unit)
            body = body[0]
            expected = helper.prefix(form == 'prepared')
            if helper.entry_prefix(body) != expected:
                raise ValueError('Installed format tag disagrees with actual callback format ' + unit)
            raw_count, literal_count = len(RAW.findall(body)), len(LITERAL.findall(body))
            if (raw_count, literal_count) != ((35, 0) if form == 'raw' else (0, 35)):
                raise ValueError('Lookup callback format is not uniformly35 source sites')
            retained_source = getattr(helper, remove)(source)
            retained = BODY.findall(retained_source)[0]
            legacy = source.replace(expected, helper.PREFIX, 1)
            helper.authenticate(legacy)
            refused(helper.installed_entries, legacy, 'legacy prefix counted installed')
            if getattr(helper, remove)(legacy) != retained_source or getattr(helper, install)(legacy) != source:
                raise ValueError('Exact legacy migration changed retained source')
            late, receipt = specialize(source)
            twice, second = specialize(late)
            if twice != late or receipt['specialized_sha256'] != digest(late.encode()) or second['specialized_sha256'] != digest(twice.encode()):
                raise ValueError('Late retagging/idempotence/returned-byte hash mismatch')
            helper.installed_entries(late)
            unbound, _ = specialize(retained_source)
            if helper.INCLUDE in unbound:
                raise ValueError('Specializer installed an unbound lookup owner')
            clean = body.replace(expected, '', 1)
            at = clean.rfind('\n}')
            end = clean[:at] + expected + clean[at:]
            nested = clean.replace('{', '{\n    auto misplaced_callback = [&]() {' + expected + '\n    };\n', 1)
            mutants = {
                'whole-function-end': source.replace(body, end, 1),
                'nested-callback': source.replace(body, nested, 1),
                'before-owner': source.replace(body, expected + clean, 1),
                'earlier-helper': 'static void prior_callback() {' + expected + '\n}\n' + source.replace(expected, '', 1),
                'wrong-tag': source.replace(expected, helper.prefix(form != 'prepared'), 1),
                'missing-include': source.replace(helper.INCLUDE, '', 1),
            }
            mixed_body = (LITERAL.sub(r'if(s.services.instruction){s.services.instruction(\1);}', body, count=1)
                          if form == 'prepared' else RAW.sub(r's.instruction_literal(\1);', body, count=1))
            if mixed_body == body:
                raise ValueError('Mixed-format control did not alter the actual owning body')
            mutants['mixed-callbacks'] = source.replace(body, mixed_body, 1)
            for tag in (None, False, True):
                misplaced = helper.PREFIX if tag is None else helper.prefix(tag)
                prior = 'static void prior_callback(WorldState &s) {' + misplaced + '\n}\n'
                # This uses the truly unbound body (labels removed), so a
                # combined missing include cannot hide behind partial labels.
                mutants['prior-helper-no-include-' + str(tag)] = retained_source.replace(retained, prior + retained, 1)
                mutants['duplicate-outside-' + str(tag)] = source.replace(body, prior + body, 1)
            namespace = re.findall(r'halo2::([A-Za-z_]+)::bridge', expected)[0]
            foreign = 'native_object_material_owner_detail' if namespace == 'native_object_body_owner_detail' else 'native_object_body_owner_detail'
            wrong_owner = expected.replace('halo2::' + namespace + '::bridge', 'halo2::' + foreign + '::bridge', 1)
            prior = 'static void prior_callback(WorldState &s) {' + wrong_owner + '\n}\n'
            mutants['wrong-owner-no-include'] = retained_source.replace(retained, prior + retained, 1)
            mutants['wrong-owner-at-entry'] = source.replace(expected, wrong_owner, 1)
            prior = 'static void prior_callback(WorldState &s) {(void)halo2::' + namespace + '::bridge<false>(s);}\n'
            mutants['partial-outside-call-no-include'] = retained_source.replace(retained, prior + retained, 1)
            for label, mutant in mutants.items():
                for api in (helper.authenticate, helper.installed_entries, getattr(helper, install), specialize):
                    refused(api, mutant, unit + '/' + form + '/' + label)
            # Resolver/sign-extension and complete retained body remain actual
            # owner bytes. Only names change inside disposable C++ namespaces.
            resolver = re.findall(r'^std::uint32_t selected\([^\n]+', source, re.M)
            sign = re.findall(r'^std::uint32_t sign_extend\([^\n]+\{.*?^\}', source, re.M | re.S)
            if len(resolver) != 1 or len(sign) != 1:
                raise ValueError('Missing complete lookup source helpers')
            index = len(rows)
            entry = 'run_bbcb0' if 'run_bbcb0' in body else 'run_bcdf0'
            blocks.append('namespace input' + str(index) + ' {\n' + resolver[0] + '\n' + sign[0] + '\n' +
                          body.replace(entry + '(', 'candidate(', 1) + '\n' + retained.replace(entry + '(', 'reference(', 1) + '\n}\n')
            rows.append(dict(unit=unit, form=form, complete_body_sha256=digest(body.encode()),
                             retained_body_sha256=digest(retained.encode()), resolver_sha256=digest(resolver[0].encode()),
                             callback_sites_sha256=digest(json.dumps(RAW.findall(body) + LITERAL.findall(body)).encode()),
                             raw_callbacks=raw_count, literal_callbacks=literal_count, strict_api_mutations=len(mutants) * 4,
                             legacy_migration_exact=True, retagging_idempotent=True, returned_byte_hash_exact=True))
    return rows, blocks


def harness(scratch, rows, blocks):
    prepare_reference(scratch)
    support = (ROOT / HARNESS).read_text()
    if digest(support.encode()) != HARNESS_PIN:
        raise ValueError('Complete retained source test harness drift')
    seam = '''            out.resume=native_object_body_detail::run_source_frame(state,continuation);
            if(out.resume)source_reference::retained(state,out.resume);'''
    construction = '    WorldState state{memory,observer,services,SourcePhysicsContactImage::TitleUpdate15};'
    if support.count(seam) != 1 or support.count(construction) != 1 or support.count('}else source_reference::retained(state,0u);') != 1:
        raise ValueError('Exact retained source harness seam drift')
    support = support.replace(seam, '            installed_candidate(state);', 1)
    support = support.replace('}else source_reference::retained(state,0u);', '}else installed_reference(state);', 1)
    support = support.replace(construction, '    if(callback_free){services.instruction={};cursor={};}\n' + construction, 1)
    declarations = '''using WorldState=halo2::physics_world_runtime_detail::WorldState;
static void(*installed_candidate)(WorldState&);
static void(*installed_reference)(WorldState&);
static bool callback_free{};
'''
    at = support.index('namespace {')
    support = support[:at] + declarations + support[at:]
    support = support.replace('int main(int argc,char**argv)', 'int existing_source_harness_main(int argc,char**argv)', 1)
    source = ''.join('#include "gameplay/objects/' + header + '"\n' for header in HEADERS[:4]) + support
    source += '\nnamespace {using namespace halo2::physics_world_runtime_detail;\n' + '\n'.join(blocks) + '\n}\n'
    source += '''int main(int argc,char**argv)try{
check(argc==2,"Supply installed lookup format corpus");std::ifstream f(argv[1]);const auto rows=J::parse(f);unsigned comparisons{};
'''
    for index, row in enumerate(rows):
        source += 'installed_candidate=&input' + str(index) + '::candidate;installed_reference=&input' + str(index) + '::reference;callback_free=false;\n'
        source += '''for(const auto&row:rows)for(bool observed:{false,true})for(unsigned action=0;action<=9u;++action){
auto old=seed(row),now=seed(row);const auto before=halo2::native_object_body_owner_detail::activity();
const auto expected=run(old,row,false,observed,action?1u:0u,action);const auto got=run(now,row,true,observed,action?1u:0u,action);
equal(old,now,expected,got,"Installed lookup callback stream");
check(halo2::native_object_body_owner_detail::activity()-before==EXPECTED,"Installed lookup entry format guard differs");++comparisons;}
'''.replace('EXPECTED', '0u' if row['form'] == 'raw' else '1u')
        source += '''callback_free=true;for(const auto&row:rows)for(unsigned action:{0u,7u}){
auto old=seed(row),now=seed(row);const auto before=halo2::native_object_body_owner_detail::activity();
const auto expected=run(old,row,false,false,action?1u:0u,action);const auto got=run(now,row,true,false,action?1u:0u,action);
equal(old,now,expected,got,"Installed callback-free lookup and late callback installation");
check(halo2::native_object_body_owner_detail::activity()-before==1u,"Installed callback-free lookup did not execute native guard");++comparisons;}
'''
    source += r'''std::cout<<J{{"passed",true},{"source_stream_comparisons",comparisons},{"installed_headers",true}}.dump()<<'\n';return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
'''
    path = scratch / 'lookup_format.cpp'
    path.write_text(source)
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--compiler', action='append')
    parser.add_argument('--sanitizers', action='store_true')
    parser.add_argument('--semantic-mutants', action='store_true')
    args = parser.parse_args()
    if not os.environ.get('HALO2_TEST_SCRATCH'):
        parser.error('Run through tools/run_test.py')
    if platform.system() != 'Linux' or platform.machine().lower() not in ('x86_64', 'amd64', 'i386', 'i686'):
        return 77
    scratch = Path(os.environ['HALO2_TEST_SCRATCH'])
    names = [unit for *_, unit in FAMILIES] + ['tools/' + family + '.py' for family, *_ in FAMILIES]
    names += ['src/gameplay/objects/' + header for header in HEADERS]
    names += [HARNESS, 'tools/validate_native_object_lookup_format_tu15.py', 'tools/specialize_native_operands.py',
              'tools/validate_native_object_body_source_frame_tu15.py', 'tools/validate_native_object_body_frame_tu15.py',
              'src/physics/runtime/physics_contact_runtime_internal.hpp', 'include/halo2/native_tu15_object_body_kernel.hpp']
    pins = {name: digest((ROOT / name).read_bytes()) for name in names}
    report = dict(passed=False, scope='Installed source-format transport; original hardware/core qualification remains separate.',
                  source_before=pins, owned_scratch=str(scratch), retained_large_artifacts=[])
    try:
        rows, blocks = contexts()
        report['contexts'] = rows
        source = harness(scratch, rows, blocks)
        fixture = scratch / 'inputs.json'
        fixture.write_text(json.dumps(corpus()[:32], separators=(',', ':')))
        environment = os.environ.copy()
        environment.pop('HALO2_REFERENCE_OBJECT_BODY', None)
        results = []
        for compiler in args.compiler or ('g++', 'clang++'):
            binary = scratch / Path(compiler).name
            command = [compiler, '-std=c++20', '-O1', '-fno-fast-math', '-ffp-contract=off', '-frounding-math',
                       '-I' + str(ROOT / 'include'), '-I' + str(ROOT / 'src'), '-I' + str(ROOT / 'tests'), '-I' + str(scratch),
                       str(source), '-o', str(binary)]
            if args.sanitizers:
                command += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
            built = subprocess.run(command, capture_output=True, text=True)
            if built.returncode:
                raise ValueError('Installed lookup format compile failed: ' + built.stderr[-7000:])
            ran = subprocess.run([str(binary), str(fixture)], capture_output=True, text=True, timeout=120, env=environment)
            if ran.returncode:
                raise ValueError('Installed lookup format comparison failed: ' + (ran.stdout + ran.stderr)[-7000:])
            mutations = []
            if args.semantic_mutants:
                relative = 'src/gameplay/objects/native_tu15_object_body_source_owner.hpp'
                header = (ROOT / relative).read_text()
                definitions = (
                    ('ignore-raw-entry-policy', 'if constexpr(!Literal)if(s.services.instruction)return false;', 'if constexpr(false)if(s.services.instruction)return false;'),
                    ('ignore-returned-format-policy', 'return admitted<Literal>(current);', 'return true;'),
                )
                for label, old, new in definitions:
                    if header.count(old) != 1:
                        raise ValueError('Installed runtime guard defect seam drift ' + label)
                    overlay = scratch / (Path(compiler).name + '-' + label)
                    target = overlay / relative
                    target.parent.mkdir(parents=True)
                    target.write_text(header.replace(old, new, 1))
                    changed = list(command)
                    changed.insert(1, '-I' + str(overlay / 'src'))
                    changed[changed.index('-o') + 1] = str(overlay / 'bad')
                    compiled = subprocess.run(changed, capture_output=True, text=True)
                    if compiled.returncode:
                        raise ValueError('Runtime format defect failed to compile ' + label + ': ' + compiled.stderr[-5000:])
                    bad = subprocess.run([str(overlay / 'bad'), str(fixture)], capture_output=True, text=True, timeout=120, env=environment)
                    diagnostic = (bad.stdout + bad.stderr)[-5000:]
                    if not bad.returncode or 'Object-body source continuation differs' not in diagnostic:
                        raise ValueError('Runtime format defect survived/unrecognized ' + label + ': ' + diagnostic)
                    mutations.append(dict(name=label, rejected=True, diagnostic=diagnostic))
            results.append(dict(compiler=compiler, command=command, result=json.loads(ran.stdout), compiled_mutations=mutations))
        report.update(passed=True, compilers=results)
    except BaseException as error:
        report['error'] = repr(error)
        raise
    finally:
        report['source_after'] = {name: digest((ROOT / name).read_bytes()) for name in pins}
        if report['source_after'] != pins:
            report.update(passed=False, source_drift=True)
        args.output.mkdir(parents=True, exist_ok=True)
        (args.output / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
