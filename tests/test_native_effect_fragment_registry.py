"""The shipped fragment controls must equal the actual TU1.5 state producers."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
REGISTRY=ROOT/'shaders/native-programs/fragments/effects.json'
sys.path.insert(0,str(ROOT/'tools'))
from compile_native_fragment_kernels import entries,write_header


class NativeEffectFragmentRegistryTests(unittest.TestCase):
    def test_procedural_startup_native_lookup_owns_each_source_program(self):
        scratch=Path(os.environ['HALO2_TEST_SCRATCH'])/'projected-decal-native-lookup'
        scratch.mkdir()
        write_header(scratch/'native_fragment_kernels.generated.hpp',entries(
            sorted((ROOT/'shaders/native-programs/fragments').glob('*.json'))))
        source=scratch/'projected_decal.cpp'
        source.write_text('''#include "halo2/source_decal_pass.hpp"
#include "halo2/native_fragment_kernel.hpp"
#include "halo2/motion_sensor.hpp"
#include <iostream>
int main(){try{
    for(unsigned layer=2;layer<=5;++layer){
        const auto authored=halo2::source_decal_pass(layer);
        if(halo2::native_fragment_kernel_path("decal",authored.program.words).empty() ||
           halo2::native_fragment_kernel_path("world-fixed",authored.program.words).empty())return 2;
        auto unknown=authored.program.words;unknown[55]^=1;
        try{(void)halo2::native_fragment_kernel_path("world-fixed",unknown);return 3;}
        catch(const halo2::FormatError&){}
    }
    const auto sensor=halo2::source_motion_sensor_final_program_tu15();
    if(halo2::native_fragment_kernel_path("hud",sensor.words).empty())return 4;
    std::cout<<"Original projected-decal and final radar programs resolve; unknown controls reject\\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\\n';return 1;}}
''')
        binary=scratch/'projected_decal'
        result=subprocess.run(['c++','-std=c++20','-O1','-ffunction-sections','-fdata-sections',
            '-I'+str(ROOT/'include'),
            '-I'+str(scratch),str(source),str(ROOT/'src/rendering/native_fragment_kernel.cpp'),
            str(ROOT/'src/ui/hud/motion_sensor.cpp'),'-Wl,--gc-sections','-o',str(binary)],
            capture_output=True,text=True,cwd=ROOT)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        result=subprocess.run([str(binary)],capture_output=True,text=True)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)

    def test_complete_producer_registry_reexecutes_without_control_or_branch_loss(self):
        scratch=Path(os.environ['HALO2_TEST_SCRATCH'])/'effect-fragment-registry-check'
        scratch.mkdir()
        output,report=scratch/'effects.json',scratch/'receipt.json'
        result=subprocess.run([sys.executable,'-B',str(ROOT/'tools/export_native_effect_fragment_programs.py'),
                               '--output',str(output),'--report',str(report)],
                              capture_output=True,text=True,cwd=ROOT)
        self.assertEqual(result.returncode,0,result.stdout+result.stderr)
        self.assertEqual(json.loads(output.read_text()),json.loads(REGISTRY.read_text()))
        receipt=json.loads(report.read_text())
        self.assertEqual(receipt['producer_cases'],50)
        self.assertEqual(receipt['counts'],{'scene-copy':1,'scope-copy':1,'lens':1,'decal':5,
                                          'world-fixed':4,'hud':1,'contrail':14})

    def test_only_dynamic_color_words_are_normalized_and_provenance_is_current(self):
        registry=json.loads(REGISTRY.read_text())
        self.assertEqual(registry['schema'],'HALO2_NATIVE_FRAGMENT_PROGRAMS_V1')
        identities=set()
        for row in registry['entries']:
            self.assertEqual(len(row['controls']),60)
            self.assertTrue(all(type(word)==int and 0<=word<=0xffffffff for word in row['controls']))
            self.assertTrue(all(row['controls'][i]==0 for i in (*range(10,26),43,44)))
            identity=(row['family'],tuple(row['controls']))
            self.assertNotIn(identity,identities);identities.add(identity)
            self.assertTrue(row['provenance']['producer_arguments'])
            for name,pin in row['provenance']['source_files'].items():
                self.assertEqual(hashlib.sha256((ROOT/name).read_bytes()).hexdigest(),pin,name)
            for route in row['provenance'].get('runtime_routes',[]):
                source=(ROOT/route['source']).read_text()
                first=source.index(route['begin']);last=source.index(route['end_exclusive'],first)
                self.assertEqual(hashlib.sha256(source[first:last].encode()).hexdigest(),route['sha256'])


if __name__=='__main__':unittest.main()
