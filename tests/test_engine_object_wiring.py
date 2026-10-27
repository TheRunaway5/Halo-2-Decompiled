"""Real object-library archive delivery and build-only dependency accounting."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import textwrap
import unittest

ROOT = Path(__file__).resolve().parents[1]


@unittest.skipUnless(shutil.which('cmake') and shutil.which('ninja') and shutil.which('c++'),
                     'CMake, Ninja and C++ compiler required')
class EngineObjectWiring(unittest.TestCase):
    def test_literal_object_source_reaches_archive_but_build_dependency_stays_separate(self):
        with tempfile.TemporaryDirectory(prefix='engine-object-wiring-',
                                        dir=os.environ.get('HALO2_TEST_SCRATCH')) as owner:
            work = Path(owner)
            source = work / 'source'
            (source / 'src').mkdir(parents=True)
            (source / 'cmake').mkdir()
            (source / 'src/context.cpp').write_text('extern "C" int object_context() { return 17; }\n')
            (source / 'src/loading.cpp').write_text('extern "C" int loading_owner() { return 23; }\n')
            (source / 'src/build_only.cpp').write_text('extern "C" int build_only() { return 99; }\n')
            (source / 'src/main.cpp').write_text(
                'extern "C" int object_context();\nextern "C" int loading_owner();\n'
                'int main() { return object_context() == 17 && loading_owner() == 23 ? 0 : 1; }\n')
            (source / 'verify_wiring.py').write_text(textwrap.dedent(f'''\
                import json
                from pathlib import Path
                import sys
                from unittest.mock import patch
                sys.path.insert(0, {str(ROOT / 'tools')!r})
                import build_launcher_engine as engine
                source, build = map(Path, sys.argv[1:])
                graph = build / 'engine-targets.txt'
                rows = {{line.split('|')[0]: line.split('|') for line in graph.read_text().splitlines()}}
                assert '$<TARGET_OBJECTS:context_objects>' in rows['halo2_systems'][3]
                assert '/cmake/../src/loading.cpp' in rows['halo2_systems'][3]
                assert 'build_only_objects' in rows['halo2_native'][2].split(';')
                unadmitted = engine.wiring(graph, source)
                assert unadmitted['unexpected_unwired_sources'] == ['src/build_only.cpp'], unadmitted
                with patch.dict(engine.SEPARATE_COMPONENTS, {{'src/build_only.cpp': 'Fixture build-only provider'}}):
                    report = engine.wiring(graph, source)
                assert report['passed'], report
                assert set(report['engine_targets']) == {{'halo2_native', 'halo2_systems', 'context_objects'}}, report
                assert set(report['engine_sources']) == {{'src/context.cpp', 'src/loading.cpp', 'src/main.cpp'}}, report
                assert report['separate_components'] == {{'src/build_only.cpp': ['build_only_objects']}}, report
                assert report['component_boundaries'] == {{'src/build_only.cpp': 'Fixture build-only provider'}}, report
                (build / 'object-wiring-report.json').write_text(json.dumps(report, indent=2) + '\\n')
                print('Verified actual archived object source and excluded build-only dependency')
                '''))
            (source / 'CMakeLists.txt').write_text(textwrap.dedent(f'''\
                cmake_minimum_required(VERSION 3.24)
                project(EngineObjectWiringFixture LANGUAGES CXX)
                find_package(Python3 REQUIRED COMPONENTS Interpreter)
                enable_testing()
                add_library(context_objects OBJECT src/context.cpp)
                add_library(halo2_systems STATIC $<TARGET_OBJECTS:context_objects>)
                target_sources(halo2_systems PRIVATE "${{CMAKE_CURRENT_SOURCE_DIR}}/cmake/../src/loading.cpp")
                add_library(build_only_objects OBJECT EXCLUDE_FROM_ALL src/build_only.cpp)
                add_executable(halo2_native src/main.cpp)
                target_link_libraries(halo2_native PRIVATE halo2_systems)
                add_dependencies(halo2_native build_only_objects)
                include("{ROOT / 'cmake/EngineWiring.cmake'}")
                add_test(NAME fixture_object_archive_execution COMMAND halo2_native)
                add_test(NAME fixture_object_graph COMMAND "${{Python3_EXECUTABLE}}" -B
                    "${{CMAKE_CURRENT_SOURCE_DIR}}/verify_wiring.py"
                    "${{CMAKE_CURRENT_SOURCE_DIR}}" "${{CMAKE_CURRENT_BINARY_DIR}}")
                '''))
            output = work / 'reports'
            subprocess.run([sys.executable, '-B', str(ROOT / 'tools/run_tests.py'),
                '--source', str(source), '--output', str(output), '--scratch-root', str(work / 'scratch'),
                '--target', 'halo2_native', '--filter', '^fixture_object_', '--cmake-arg=-GNinja'], check=True)
            report = json.loads((output / 'report.json').read_bytes())
            self.assertTrue(report['passed'])
            self.assertFalse(Path(report['build']).exists())
        self.assertFalse(work.exists())


if __name__ == '__main__':
    unittest.main()
