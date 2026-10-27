"""Actual compiled metadata lookup rejects every changed complete identity."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'tools'))
from compile_menu_world_vertex_metadata import header, rows


class NativeRawMetadata(unittest.TestCase):
    def test_compiled_lookup_and_complete_identity_rejections(self):
        compiler = shutil.which('c++')
        self.assertIsNotNone(compiler, 'A native compiler is required')
        catalogs = [ROOT/path for path in ('shaders/native-programs/menu/sources.json',
            'shaders/pstencil/native-programs/sources.json', 'shaders/native-programs/renderer-raw/sources.json')]
        programs = json.loads((ROOT/'shaders/native-programs/world/programs.json').read_bytes())
        records = rows(catalogs, programs)
        code = {hashlib.sha256(bytes.fromhex(row['vertex_program_hex'])).hexdigest(): row['vertex_program_hex']
            for row in programs['cpu_vertex_programs']}
        work = Path(os.environ['HALO2_TEST_SCRATCH'])/'raw-native-metadata';work.mkdir()
        (work/'menu_world_vertex_metadata.generated.hpp').write_text(header(records))
        (work/'cases.json').write_text(json.dumps([{**row, 'program_hex': code[row['program_sha256']]} for row in records]))
        source = r'''
#include "halo2/menu_world_vertex_catalog.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <iostream>
int main(int argc,char** argv) {
    using namespace halo2;std::ifstream input(argv[1]);const auto cases=nlohmann::json::parse(input);
    unsigned passed=0,rejected=0;
    for(const auto& row:cases) {
        const auto text=row.at("program_hex").get<std::string>();std::vector<std::byte> bytes;
        for(unsigned i=0;i<text.size();i+=2)bytes.push_back(std::byte(std::stoul(text.substr(i,2),nullptr,16)));
        SourceVertexDeclaration declaration{row.at("class_index").get<unsigned>(),row.at("semantics").get<std::vector<std::uint16_t>>()};
        const auto& found=menu_world_vertex_metadata_lookup(bytes,declaration);
        if(found.program_sha256!=row.at("program_sha256").get<std::string>() || found.program_bytes!=bytes.size() ||
           !std::ranges::equal(found.consumed_inputs,row.at("consumed_inputs").get<std::vector<unsigned>>()) ||
           !std::ranges::equal(found.consumed_constants,row.at("consumed_constants").get<std::vector<unsigned>>()) ||
           found.consumed_input_components!=row.at("consumed_input_components").get<std::array<std::uint8_t,16>>() ||
           found.default_texture_w_mask!=row.at("default_texture_w_mask").get<unsigned>() ||
           found.relative_constants!=row.at("relative_constants").get<bool>())return 2;
        ++passed;
        const auto reject=[&](const auto& code,const auto& declared) {
            try{(void)menu_world_vertex_metadata_lookup(code,declared);return false;}
            catch(const UnsupportedVertexProgram&){++rejected;return true;}
        };
        auto changed=bytes;changed.back()^=std::byte{128};if(!reject(changed,declaration))return 3;
        changed=bytes;changed.push_back(std::byte{});if(!reject(changed,declaration))return 4;
        auto wrong=declaration;wrong.semantics[0]^=32768u;if(!reject(bytes,wrong))return 5;
    }
    std::cout<<passed<<" complete native metadata identities; "<<rejected<<" identity mutation rejections\n";
}
'''
        unit=work/'probe.cpp';binary=work/'probe';unit.write_text(source)
        result=subprocess.run([compiler, '-std=c++20', '-O1', '-I'+str(ROOT/'include'), '-I'+str(work),
            str(unit), str(ROOT/'src/ui/menu_world_vertex_metadata.cpp'), '-lcrypto', '-o', str(binary)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        result=subprocess.run([str(binary), str(work/'cases.json')], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout+result.stderr)
        print(result.stdout.strip())


if __name__ == '__main__':
    unittest.main()
