"""Exercise the offline census against the actual original-program assembler."""
import hashlib
import itertools
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class NativeCatalogVariants(unittest.TestCase):
    def test_complete_ordinals_and_redundant_assignment_digest(self):
        compiler = shutil.which('c++')
        self.assertIsNotNone(compiler, 'A native C++ compiler is required')
        source = (ROOT/'tools/export_native_world_kernels.cpp').read_text()
        # Compile the real finite-domain implementation against the real
        # 0001BA00 assembler, independently of any maps or bitmap loading.
        implementation = (source[source.index('struct AliasEvidence {'):source.index('struct Registry {')]+
            source[source.index('struct ProgramVariant {'):source.index('void export_all_materials(')])
        prelude = r'''
#include "halo2/cache.hpp"
#include <nlohmann/json.hpp>
#include <openssl/evp.h>
#include <algorithm>
#include <bit>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
using namespace halo2;
using json=nlohmann::json;
std::string hex(std::span<const std::byte> bytes) {
    std::ostringstream stream;stream<<std::hex<<std::setfill('0');
    for(auto byte:bytes)stream<<std::setw(2)<<std::to_integer<unsigned>(byte);
    return stream.str();
}
bool dynamic_color(unsigned i){return (i>=10&&i<=25)||i==43||i==44;}
'''
        probe = r'''
int main() {
    Material material;MaterialPass distinct;
    distinct.fragments.push_back({0,{std::uint16_t(2<<9)}});
    // The remap-filter values deliberately do not equal ordinal selectors.
    distinct.permutations={{{9},0,{std::uint16_t(1<<9)}},{{9},0,{std::uint16_t((1<<9)|1)}}};
    ShaderCombiner first,second;first.words[0]=1;second.words[0]=2;
    distinct.combiners={first,second};
    const auto forms=variants(material,distinct);
    json result;result["distinct_forms"]=forms.size();result["ordinals"]=json::array();
    for(const auto& form:forms)result["ordinals"].push_back(form.switches[0]);
    MaterialPass repeated;repeated.combiners.push_back({});
    for(unsigned selector=0;selector<4;++selector) {
        repeated.fragments.push_back({std::int16_t(selector),{std::uint16_t(8<<9)}});
    }
    for(unsigned i=0;i<8;++i)repeated.permutations.push_back({std::int16_t(i),0,{std::uint16_t(1<<9)}});
    const auto compact=variants(material,repeated);
    result["deduplicated_forms"]=compact.size();result["assignments"]=compact.at(0).valid_assignments;
    result["digest"]=compact.at(0).assignment_sha256;result["words"]=compact.at(0).program.words;
    AliasEvidence evidence;json row;json owners=json::array();json retained=json::array();
    json owner={{"kind","material-pass"},{"map","a.map"},{"map_sha256","a"},{"source","original-disc"},
        {"template_name","ordinary"},{"vertex_shader","world"},{"implementation",0}};
    const auto add=[&]{owners.push_back(owner);retained.push_back(evidence.add(owner));};
    add();owner["map"]="b.map";owner["map_sha256"]="b";add();
    owner["template_name"]="opaque\\emblem_overlay_simple";add();
    owner["vertex_shader"]="shaders\\stencil_texture";add();
    owner["kind"]="hud-material-pass";owner["family"]="hud";add();
    evidence.finish(row);result["source_owners"]=owners;result["retained_roles"]=retained;
    result["source_owner_evidence"]=row.at("source_owner_evidence");
    std::cout<<result.dump()<<'\n';
}
'''
        with tempfile.TemporaryDirectory(dir=os.environ.get('HALO2_TEST_SCRATCH')) as directory:
            work = Path(directory);unit = work/'variants.cpp';binary = work/'variants'
            unit.write_text(prelude+implementation+probe)
            build = subprocess.run([compiler, '-std=c++20', '-O0', '-I'+str(ROOT/'include'),
                str(unit), str(ROOT/'src/rendering/renderer/render_program.cpp'), '-lcrypto', '-o', str(binary)],
                capture_output=True, text=True)
            self.assertEqual(build.returncode, 0, build.stdout+build.stderr)
            run = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(run.returncode, 0, run.stdout+run.stderr)
            result = json.loads(run.stdout)
        self.assertEqual(result['distinct_forms'], 2)
        self.assertEqual(result['ordinals'], [0, 1])
        self.assertEqual(result['deduplicated_forms'], 1)
        self.assertEqual(result['assignments'], 4096)
        digest = hashlib.sha256()
        for switches in itertools.product(range(8), repeat=4):
            digest.update(struct.pack('<8i60I', *switches, 0, 0, 0, 0, *result['words']))
        self.assertEqual(result['digest'], digest.hexdigest())
        self.assertEqual(result['retained_roles'], [True, False, True, True, True])
        self.assertEqual(result['source_owner_evidence']['count'], 5)
        self.assertEqual(result['source_owner_evidence']['retained_routing_signatures'], 4)
        self.assertEqual(sorted(result['source_owner_evidence']['map_counts'].values()), [1, 4])
        digest = hashlib.sha256()
        for owner in result['source_owners']:
            digest.update(json.dumps(owner, sort_keys=True, separators=(',', ':')).encode()+b'\n')
        self.assertEqual(result['source_owner_evidence']['sha256'], digest.hexdigest())


if __name__ == '__main__':
    unittest.main()
