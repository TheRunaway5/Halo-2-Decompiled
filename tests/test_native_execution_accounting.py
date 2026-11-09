#!/usr/bin/env python3
"""Exercise route accounting without replacing/disabling instruction services."""
import json
import hashlib
import re
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest import mock

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
from instrument_native_execution import instrument,instrument_translation
from specialize_native_operands import install_leaves
from native_execution_report import analyze
from native_service_observations import NATIVE_SERVICE_OBSERVATIONS

REPORTS={}

FIXTURE=r'''
#include <functional>
#include <stdexcept>
#include <iostream>
#include <thread>
#include <vector>
#include <xmmintrin.h>
namespace halo2 {enum class SourcePhysicsContactImage {MapPack11,TitleUpdate15};}
struct State {
 halo2::SourcePhysicsContactImage image=halo2::SourcePhysicsContactImage::TitleUpdate15;
 struct {std::function<void(unsigned)> instruction;} services;
 bool native{},child_native{};int depth{};unsigned visits{},gp[2]{};
 void instruction_literal(unsigned pc){if(services.instruction)services.instruction(pc);}
};
static void run_200(State &s) {
 if(s.child_native)return;
 if(s.services.instruction){s.services.instruction(0x200u);}++s.visits;
}
[[gnu::noinline]] static void run_100(State &s) {
 if(s.native){run_200(s);return;}
L100: if(s.services.instruction){s.services.instruction(0x100u);}++s.visits;run_200(s);
 for(unsigned n=0;n<2;++n){if(s.services.instruction)s.services.instruction(0x101u);++s.visits;}
}
static void run_300(State &s) {
 s.instruction_literal(0x300u);++s.visits;if(s.depth-->0)run_300(s);
}
static void run_400(State &s) {
 if(s.services.instruction){s.services.instruction(0x400u);}throw std::runtime_error("after body");
}
static void run_500(State &s) {
 if(s.native)throw std::runtime_error("before body");
 if(s.services.instruction){s.services.instruction(0x500u);}
}
static void run_600(State &s) {
 if(s.services.instruction){s.services.instruction(0x600u);}++s.visits;
}
static void run_700(State &s) {
 if(s.services.instruction){s.services.instruction(0x700u);}++s.visits;
}
static void run_800(State &s) {
 if(s.services.instruction){s.services.instruction(0x800u);}
 s.gp[1]=3;do {++s.visits;--s.gp[1];if(s.gp[1]&&s.services.instruction)s.services.instruction(0x800u);}while(s.gp[1]);
}
static void run_900(State &s) {
 if(s.services.instruction){s.services.instruction(0x900u);}
}
int main(){
 _mm_setcsr(0x5fc1u);State s;unsigned callbacks{};
 // The registry and function-local counter both initialize concurrently.
 {std::vector<std::thread> cold;for(unsigned n=0;n<4;++n)cold.emplace_back([]{State t;run_900(t);});for(auto& t:cold)t.join();}
 // Disabled service callbacks must not hide translated body execution.
 s.native=true;run_100(s);s.native=false;s.child_native=true;run_100(s);
 s.depth=2;run_300(s);try{run_400(s);}catch(const std::runtime_error&){}
 run_800(s);
 s.native=true;try{run_500(s);}catch(const std::runtime_error&){}
 s.image=halo2::SourcePhysicsContactImage::MapPack11;run_100(s);
 // Counts remain independent when a real callback is present too.
 s.services.instruction=[&](unsigned){++callbacks;};run_600(s);
 std::vector<std::thread> threads;
 for(unsigned n=0;n<4;++n)threads.emplace_back([]{State t;for(unsigned k=0;k<1000;++k){if(k&1)run_600(t);else run_700(t);}});
 for(auto& t:threads)t.join();
 std::cout<<s.visits<<' '<<callbacks<<' '<<_mm_getcsr()<<'\n';
}
'''


class Accounting(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH'))
        self.addCleanup(self.temp.cleanup);self.directory=Path(self.temp.name)

    def test_native_parent_nested_fallback_recursion_exceptions_images_and_threads(self):
        transformed,manifest=instrument(FIXTURE,FIXTURE,'src/fixture_generated.cpp')
        self.assertEqual(manifest['function_count'],9)
        self.assertFalse(manifest['service_callbacks_replaced'])
        self.assertFalse(manifest['native_admission_state_modified'])
        outputs=[]
        for name,text in (('original',FIXTURE),('accounted',transformed)):
            source=self.directory/(name+'.cpp');binary=self.directory/name;source.write_text(text)
            command=['c++','-std=c++20','-O2','-pthread','-I'+str(ROOT/'include'),str(source)]
            if name=='accounted':command.append(str(ROOT/'src/runtime/native_execution_accounting.cpp'))
            command+=['-o',str(binary)];subprocess.run(command,check=True,capture_output=True)
            env=dict(os.environ,HALO2_NATIVE_EXECUTION_REPORT=str(self.directory/'counts.json'))
            outputs.append(subprocess.check_output([str(binary)],env=env,text=True))
        self.assertEqual(outputs[0],outputs[1])
        report=json.loads((self.directory/'counts.json').read_text());fields=report['row_fields']
        REPORTS['nested_recursion_exceptions_images_threads']=dict(runtime=report,analysis=analyze(report),output_sha256=hashlib.sha256(outputs[0].encode()).hexdigest())
        rows={(row[1],report['image_names'][row[2]]):dict(zip(fields,row)) for row in report['rows']}
        parent=rows['100','tu15'];self.assertEqual((parent['entries'],parent['body_entries'],parent['literal_statements'],parent['returns_without_body']),(2,1,3,1))
        child=rows['200','tu15'];self.assertEqual((child['entries'],child['body_entries'],child['literal_statements']),(2,1,1))
        recurse=rows['300','tu15'];self.assertEqual((recurse['entries'],recurse['body_entries'],recurse['literal_statements']),(3,3,3))
        self.assertEqual(rows['400','tu15']['unwinds'],1)
        self.assertEqual(rows['400','tu15']['unwinds_without_body'],0)
        self.assertEqual(rows['500','tu15']['unwinds_without_body'],1)
        self.assertEqual(rows['100','map-pack11']['returns_without_body'],1)
        self.assertEqual(rows['600','tu15']['entries'],2000)
        self.assertEqual(rows['600','map-pack11']['entries'],1)
        self.assertEqual(rows['700','tu15']['literal_statements'],2000)
        self.assertEqual(rows['800','tu15']['literal_statements'],3)
        self.assertEqual(rows['900','tu15']['entries'],4)
        self.assertEqual(report['omitted_records'],0);self.assertEqual(report['active_scopes'],0)
        self.assertFalse(report['native_closure_proved'])
        self.assertLess((self.directory/'counts.json').stat().st_size,1_048_576)

    def test_native_service_sites_require_exact_source_hash_without_rewriting_counts(self):
        name='src/native-service.hpp';source='inline void service(State &s){s.instruction_literal(0x33b590u);s.instruction_literal(0x33b595u);s.instruction_literal(0x33b597u);}\n'
        pin=hashlib.sha256(source.encode()).hexdigest()
        with mock.patch.dict(NATIVE_SERVICE_OBSERVATIONS,{(name,'service'):pin}):
            _,manifest=instrument(source,source,name)
            self.assertEqual(manifest['functions'][0]['source_body_semantics'],'native_service_observation_sites')
            fields=['unit','symbol','image','literal_sites','dynamic_forwarders','entries','body_entries','literal_statements','returns','unwinds','returns_without_body','unwinds_without_body','active']
            raw=dict(schema='native-execution-accounting-v1',units=[[name,pin,'1'*64,'tu15']],image_names=['map-pack11','tu15','unknown'],row_fields=fields,
                rows=[[0,'service',1,3,0,2,2,6,2,0,0,0,0]],omitted_records=0,registration_failures=0,active_scopes=0)
            report=analyze(raw);self.assertEqual(report['native_service_observation_rows'],1);self.assertEqual(report['literal_fallback_rows'],0)
            self.assertEqual(report['totals']['literal_statements'],6);self.assertEqual(report['native_service_observation_totals']['literal_statements'],6)
            raw['units'][0][1]='f'*64;report=analyze(raw)
            self.assertEqual(report['native_service_observation_rows'],0);self.assertEqual(report['literal_fallback_rows'],1)
            self.assertEqual(report['totals']['literal_statements'],6)

    def test_rejects_ambiguous_or_reinstrumented_sites(self):
        source='static void run_100(State &s) {\nif(s.services.instruction){s.services.instruction(0x100u);extra();}\n}\n'
        with self.assertRaises(ValueError):instrument(source,source,'src/fixture_generated.cpp')
        source='static void run_100(State &s) {\ns.services.instruction(0x100u);\n}\n'
        with self.assertRaises(ValueError):instrument(source,source,'src/fixture_generated.cpp')
        transformed,_=instrument(FIXTURE,FIXTURE,'src/fixture_generated.cpp')
        with self.assertRaises(ValueError):instrument(transformed,FIXTURE,'src/fixture_generated.cpp')

    def test_descriptive_owners_damage_mapper_and_local_state(self):
        source=r'''
void producer(WorldState &s) {
 if(s.services.instruction)s.services.instruction(0x1482a0u);
}
void run_status(WorldState &s) {
 if(s.services.instruction){s.services.instruction(0x80023653u);}
}
void source_run_tu15_power_dpc(Memory &memory,
 const Services &services) {
 WorldState s{memory,observe,services,SourcePhysicsContactImage::TitleUpdate15};
 if(services.instruction)services.instruction(0x2e009du);
}
static void run_200(ObservedDamageState &s) {
 if(s.services.instruction){s.services.instruction(damage_observation_pc(0x200u,0x300u,s));}
}
'''
        output,manifest=instrument(source,source,'src/fixture_generated.cpp')
        self.assertEqual([row['symbol'] for row in manifest['functions']],['producer','run_status','source_run_tu15_power_dpc','200'])
        self.assertEqual(manifest['literal_sites'],4)
        self.assertIn('halo2_execution_counter_8ef3,1u',output)
        with self.assertRaisesRegex(ValueError,'no recognized generated function owner'):
            instrument('auto unsupported=[] {instruction(0x100u);};','', 'src/fixture_generated.cpp')

    def test_nonvoid_owners_unchanged_lambda_captures_and_orphan_refusal(self):
        fixture=r'''
#include <functional>
#include <iostream>
namespace halo2 {enum class SourcePhysicsContactImage {MapPack11,TitleUpdate15};}
struct State {halo2::SourcePhysicsContactImage image=halo2::SourcePhysicsContactImage::TitleUpdate15;};
unsigned visits{};std::function<void()> deferred;
void instruction(unsigned){++visits;}
static void child(State &s,const std::function<void()>& callback) {
 instruction(0x200u);callback();
}
template<class T> T passthrough(T value){return value;}
[[gnu::noinline]] static unsigned parent(State &s) {
 const auto callback=[] {instruction(0x100u);};
 child(s,callback);deferred=callback;return passthrough(visits);
}
int main(){State s;std::cout<<parent(s)<<' ';
// SNAPSHOT
deferred();std::cout<<visits<<'\n';}
'''
        transformed,manifest=instrument(fixture,fixture,'src/ordinary.cpp')
        self.assertEqual(manifest['function_count'],2)
        self.assertIn('const auto callback=[]',transformed)
        outputs=[]
        for name,text in (('original',fixture),('accounted',transformed)):
            valid=self.directory/'valid.json'
            if name=='accounted':text=text.replace('// SNAPSHOT','::halo2::execution_accounting::write_report("'+str(valid)+'");')
            source=self.directory/(name+'.cpp');source.write_text(text);binary=self.directory/name
            command=['c++','-std=c++20','-O2','-I'+str(ROOT/'include'),str(source)]
            if name=='accounted':command.append(str(ROOT/'src/runtime/native_execution_accounting.cpp'))
            subprocess.run(command+['-o',str(binary)],check=True,capture_output=True)
            outputs.append(subprocess.check_output([str(binary)],env=dict(os.environ,HALO2_NATIVE_EXECUTION_REPORT=str(self.directory/'orphan.json')),text=True))
        self.assertEqual(outputs[0],outputs[1]);self.assertEqual(outputs[0],'2 3\n')
        valid=json.loads((self.directory/'valid.json').read_text());self.assertEqual(analyze(valid)['totals']['body_entries'],2)
        orphan=json.loads((self.directory/'orphan.json').read_text());self.assertEqual(orphan['orphan_literal_events'],1)
        with self.assertRaisesRegex(ValueError,'no live owning activation'):analyze(orphan)
        REPORTS['nonvoid_lambda_and_orphan']=dict(valid=valid,orphan=orphan,captures_preserved=True)

    def test_embedded_header_identity_and_namespace_compile(self):
        (self.directory/'src').mkdir();header=self.directory/'src/body.hpp'
        header.write_text('#pragma once\ninline unsigned helper(State &s){s.instruction(0x100u);return 1;}\n')
        source='''#include <iostream>
struct State {unsigned visits{};void instruction(unsigned){++visits;}};
namespace nested {
#include "body.hpp"
}
static unsigned owner(State &s){s.instruction(0x200u);return nested::helper(s);}
int main(){State s;std::cout<<owner(s)<<' '<<s.visits<<'\\n';}
'''
        generated,receipt,dependencies=instrument_translation(source,source,'src/ordinary.cpp',self.directory)
        self.assertEqual(len(receipt['embedded_units']),1)
        self.assertEqual(receipt['embedded_units'][0]['canonical_source'],'src/body.hpp')
        self.assertIn(header,dependencies);self.assertIn('HALO2_ACCOUNTED_HEADER_BEGIN',generated)
        path=self.directory/'alias.cpp';path.write_text(generated);binary=self.directory/'headers'
        subprocess.run(['c++','-std=c++20','-O2','-I'+str(ROOT/'include'),str(path),str(ROOT/'src/runtime/native_execution_accounting.cpp'),'-o',str(binary)],check=True,capture_output=True)
        report=self.directory/'headers.json'
        self.assertEqual(subprocess.check_output([str(binary)],env=dict(os.environ,HALO2_NATIVE_EXECUTION_REPORT=str(report)),text=True),'1 2\n')
        raw=json.loads(report.read_text());self.assertEqual({unit[0]for unit in raw['units']},{'src/ordinary.cpp','src/body.hpp'})
        self.assertEqual(analyze(raw)['totals']['literal_statements'],2)

    def test_real_numeric_leaf_preserves_live_x87_and_complete_simd_abi(self):
        original=(ROOT/'src/rendering/models/source_model_render_tu15_generated.cpp').read_text()
        from generated_routine_ownership import function_bodies
        body=next(body for match,body in function_bodies(original) if match[1]=='145d50')
        harness=r'''
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include "rendering/models/native_tu15_prt_matrix_vector.hpp"
#include <iostream>
#include <iomanip>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;
struct Bank:SourcePhysicsContactMemory {
 std::array<std::byte,65536> bytes{};
 void read(std::uint32_t at,std::span<std::byte> out)const override{std::copy_n(bytes.begin()+at,out.size(),out.begin());}
 void write(std::uint32_t at,std::span<const std::byte> in)override{std::copy(in.begin(),in.end(),bytes.begin()+at);}
 void put(std::uint32_t at,std::uint32_t word){for(unsigned n=0;n<4;++n)bytes.at(at+n)=std::byte(word>>(n*8));}
};
std::uint32_t selected(std::uint32_t value,SourcePhysicsContactImage){return value;}
// BODY
int main(){
 const long double extended=0x1.000000000000001p+0L;
 for(unsigned depth: {0u,1u,4u})for(unsigned precision: {0u,0x200u,0x300u})for(unsigned rounding: {0u,0x400u,0x800u,0xc00u}) {
  Bank bank;SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observe;
  WorldState s{bank,observe,services,SourcePhysicsContactImage::TitleUpdate15};
  SourcePhysicsWorldRuntimeAbi abi{};abi.eflags=0xad7;abi.gp={0x4000,0xabc,0xdef,5,0xf000,0x111,0x222,0x333};
  for(unsigned n=0;n<8;++n)for(unsigned k=0;k<4;++k)abi.xmm[n][k]=0x3f000001u+n*0x100+k;
  bank.put(0xf004,0x1000);bank.put(0xf008,0x3000);
  for(unsigned n=0;n<25;++n)bank.put(0x1000+n*4,0x3f000001u+n*0x1234);
  for(unsigned n=0;n<5;++n)bank.put(0x3000+n*4,0x3e000003u+n*0x12345);
  s.load(abi);
  const unsigned short cw=static_cast<unsigned short>(0x3fu|precision|rounding);
  asm volatile("fninit\n\tfldcw %0"::"m"(cw):"memory");
  // Seed nonzero condition codes as well as a nondefault control word.
  asm volatile("fld1\n\tfldz\n\tfcompp":::"st");
  for(unsigned n=0;n<depth;++n)asm volatile("fldt %0"::"m"(extended):"st");
  _mm_setcsr(0x1f81u|((rounding>>10u)<<13u));
  alignas(16) std::array<unsigned char,512> before{},after{};
  asm volatile("fxsave %0":"=m"(before)::"memory");
  run_145d50(s);
  asm volatile("fxsave %0":"=m"(after)::"memory");
  // The numeric kernel uses SSE; the live host x87 stack/CW/status must survive.
  for(unsigned n: {0u,1u,2u,3u,4u})if(before[n]!=after[n])return 21;
  for(unsigned lane=0;lane<depth;++lane)for(unsigned n=0;n<10;++n)
   if(before[32+lane*16+n]!=after[32+lane*16+n])return 22;
  asm volatile("fninit":::"memory");
  const auto saved=s.save();
  auto hex=[&](const auto& object){const auto* p=reinterpret_cast<const unsigned char*>(&object);
   for(unsigned n=0;n<sizeof(object);++n)std::cout<<std::hex<<std::setw(2)<<std::setfill('0')<<unsigned(p[n]);};
  hex(saved.gp);hex(saved.eflags);hex(saved.xmm);
  hex(saved.x87_status);hex(bank.bytes);
  for(unsigned n: {0u,1u,2u,3u,4u,24u,25u,26u,27u})hex(after[n]);
  std::cout<<'\n';
 }
}
'''.replace('// BODY',body)
        counted,_=instrument(harness,harness,'src/rendering/models/source_model_render_tu15_generated.cpp')
        outputs=[]
        for name,source_text in (('numeric-original',harness),('numeric-accounted',counted)):
            source=self.directory/(name+'.cpp');source.write_text(source_text);binary=self.directory/name
            command=['c++','-std=c++20','-O2','-fno-fast-math','-ffp-contract=off','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),str(source)]
            if name=='numeric-accounted':command.append(str(ROOT/'src/runtime/native_execution_accounting.cpp'))
            compiled=subprocess.run(command+['-o',str(binary)],capture_output=True,text=True)
            self.assertEqual(compiled.returncode,0,compiled.stderr)
            for reference in (False,True):
                report=self.directory/(name+str(reference)+'.json')
                env=dict(os.environ,HALO2_NATIVE_EXECUTION_REPORT=str(report));env.pop('HALO2_REFERENCE_MODEL_PRT_HELPERS',None)
                if reference:env['HALO2_REFERENCE_MODEL_PRT_HELPERS']='1'
                outputs.append(subprocess.check_output([str(binary)],env=env))
                if name=='numeric-accounted':
                    result=analyze(json.loads(report.read_text()))
                    REPORTS['numeric-reference' if reference else 'numeric-native']=dict(
                        runtime=json.loads(report.read_text()),analysis=result,
                        caller_fp_setups=36,output_sha256=hashlib.sha256(outputs[-1]).hexdigest())
                    self.assertEqual(result['totals']['entries'],36)
                    self.assertEqual(result['totals']['body_entries'],36 if reference else 0)
        self.assertEqual(outputs[0],outputs[2]);self.assertEqual(outputs[1],outputs[3])

    def test_registration_failure_cannot_replace_source_exception(self):
        fixture=r'''
#include <cstdlib>
#include <new>
#include <cstdio>
#include <cerrno>
bool fail_allocation{};
void* operator new(std::size_t size){if(fail_allocation)throw std::bad_alloc();if(auto* p=std::malloc(size))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
struct State {struct {void(*instruction)(unsigned);}services;};
static int original_exception;
static void run_100(State &s) {
 if(s.services.instruction)s.services.instruction(0x100u);
 throw &original_exception;
}
static void run_200(State &s) {
 if(s.services.instruction)s.services.instruction(0x200u);
}
int main(){State s{};errno=EDOM;fail_allocation=true;
 try{run_100(s);return 20;}catch(int* p){if(p!=&original_exception)return 21;}
 catch(...){return 22;}
 fail_allocation=false;if(errno!=EDOM)return 23;run_200(s);return 0;
}
'''
        counted,_=instrument(fixture,fixture,'src/failure_generated.cpp')
        source=self.directory/'failure.cpp';source.write_text(counted);binary=self.directory/'failure'
        subprocess.run(['c++','-std=c++20','-O2','-I'+str(ROOT/'include'),str(source),
            str(ROOT/'src/runtime/native_execution_accounting.cpp'),'-o',str(binary)],check=True,capture_output=True)
        report=self.directory/'registration-failure.json'
        subprocess.run([str(binary)],check=True,env=dict(os.environ,HALO2_NATIVE_EXECUTION_REPORT=str(report)))
        data=json.loads(report.read_text());self.assertEqual(data['registration_failures'],1)
        REPORTS['registration_failure']=dict(runtime=data,reader_rejected=True,source_exception_identity_preserved=True)
        with self.assertRaisesRegex(ValueError,'registration failed'):analyze(data)

    def test_real_post_binding_bsp_admission_with_callbacks_disabled(self):
        original=(ROOT/'src/physics/physics_live_support_tu15_generated.cpp').read_text()
        body=re.search(r'static void run_14cdb0\(WorldState &s\) \{.*?\n\}',original,re.S).group()
        harness=r'''
#include "physics/runtime/physics_world_runtime_internal.hpp"
#include <iostream>
using namespace halo2;using namespace halo2::physics_world_runtime_detail;
struct Bank:SourcePhysicsContactMemory {
 std::vector<std::byte> bytes=std::vector<std::byte>(65536);
 void read(std::uint32_t at,std::span<std::byte> out)const override{std::copy_n(bytes.begin()+at,out.size(),out.begin());}
 void write(std::uint32_t at,std::span<const std::byte> in)override{std::copy(in.begin(),in.end(),bytes.begin()+at);}
 void put(std::uint32_t at,std::uint32_t word,unsigned width=4){for(unsigned n=0;n<width;++n)bytes.at(at+n)=std::byte(word>>(n*8));}
};
std::uint32_t sign_extend(std::uint32_t value,unsigned width){unsigned shift=32-width*8;return std::uint32_t(std::int32_t(value<<shift)>>shift);}
// BODY
int main(){Bank bank;bank.put(0x1004,0x2000);bank.put(0x100c,0x4000);
 bank.put(0x2000,0,2);bank.put(0x2002,0x800005,3);bank.put(0x2005,0x800005,3);
 SourcePhysicsWorldRuntimeServices services;SourcePhysicsContactRecordObserver observer;
 WorldState state{bank,observer,services,SourcePhysicsContactImage::TitleUpdate15};
 SourcePhysicsWorldRuntimeAbi abi{};abi.eflags=2;abi.gp[1]=0x1000;abi.gp[2]=0x3000;abi.gp[4]=0xf000;
 state.load(abi);run_14cdb0(state);std::cout<<state.gp[0]<<' '<<state.gp[4]<<'\n';}
'''
        canonical=harness.replace('// BODY',body)
        admitted,leaves=install_leaves(canonical,canonical)
        self.assertEqual(leaves,['14cdb0'])
        counted,manifest=instrument(admitted,canonical,'src/physics/physics_live_support_tu15_generated.cpp')
        source=self.directory/'bsp.cpp';source.write_text(counted);binary=self.directory/'bsp'
        subprocess.run(['c++','-std=c++20','-O2','-I'+str(ROOT/'include'),'-I'+str(ROOT/'src'),
            str(source),str(ROOT/'src/runtime/native_execution_accounting.cpp'),'-o',str(binary)],check=True,capture_output=True)
        outputs=[]
        for reference in (False,True):
            report=self.directory/('bsp-reference.json' if reference else 'bsp-native.json')
            env=dict(os.environ,HALO2_NATIVE_EXECUTION_REPORT=str(report))
            env.pop('HALO2_REFERENCE_MATRIX_LEAVES',None);env.pop('HALO2_VERIFY_MATRIX_LEAVES',None)
            if reference:env['HALO2_REFERENCE_MATRIX_LEAVES']='1'
            outputs.append(subprocess.check_output([str(binary)],env=env,text=True))
            data=json.loads(report.read_text());row=dict(zip(data['row_fields'],data['rows'][0]))
            REPORTS['bsp-reference' if reference else 'bsp-native']=dict(runtime=data,analysis=analyze(data),output=outputs[-1])
            self.assertEqual(row['entries'],1)
            self.assertEqual(row['body_entries'],int(reference))
            self.assertEqual(row['returns_without_body'],int(not reference))
            self.assertEqual(bool(row['literal_statements']),reference)
        self.assertEqual(outputs[0],outputs[1]);self.assertEqual(outputs[0],'5 61444\n')

    def test_cli_requires_owned_separate_alias(self):
        source=self.directory/'fixture_generated.cpp';source.write_text(FIXTURE)
        command=[sys.executable,str(ROOT/'tools/instrument_native_execution.py'),
                 '--source',str(source),'--canonical',str(source),'--root',str(self.directory),
                 '--output',str(source),'--manifest',str(self.directory/'manifest.json')]
        result=subprocess.run(command,capture_output=True,text=True)
        self.assertEqual(result.returncode,2);self.assertEqual(source.read_text(),FIXTURE)


if __name__=='__main__':
    result=unittest.main(exit=False).result
    requested=os.environ.get('HALO2_EXECUTION_ACCOUNTING_TEST_REPORT')
    if requested:
        output=Path(requested)
        report=dict(schema='native-execution-accounting-regression-v1',passed=result.wasSuccessful(),
            tests_run=result.testsRun,source_hashes={str(path.relative_to(ROOT)):hashlib.sha256(path.read_bytes()).hexdigest()
                for path in (Path(__file__),ROOT/'tools/instrument_native_execution.py',
                    ROOT/'tools/generated_routine_ownership.py',ROOT/'tools/native_execution_report.py',
                    ROOT/'include/halo2/native_execution_accounting.hpp',ROOT/'src/runtime/native_execution_accounting.cpp')},
            cases=REPORTS,large_artifacts_retained=[],engine_execution_proved=False)
        encoded=json.dumps(report,indent=2,sort_keys=True)+'\n'
        if len(encoded.encode())>1_048_576:raise ValueError('Regression report exceeds bounded retention size')
        output.parent.mkdir(parents=True,exist_ok=True);output.write_text(encoded)
    raise SystemExit(not result.wasSuccessful())
