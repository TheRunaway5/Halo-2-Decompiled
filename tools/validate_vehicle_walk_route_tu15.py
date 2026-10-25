#!/usr/bin/env python3
"""Check the input observer's collision routes against recorded actual approaches.

Uses only read-only managed headers/libraries. CMake and all generated files
belong to the enclosing run_test.py owner; retain only route reports and logs.
"""
import argparse
import json
import os
from pathlib import Path
import shlex
import subprocess
import sys
from weapon_test_cache import sha

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-owner', type=Path, required=True)
    parser.add_argument('--case', type=Path, action='append', required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    owner = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    borrowed = args.reference_owner.resolve(strict=True)
    if borrowed == owner or borrowed.parent != ROOT / '.local/test-scratch':
        raise ValueError('Read only a distinct managed source/build owner')
    frozen, build = borrowed / 'source', borrowed / 'build'
    headers = {str(path): sha(path) for folder in ('include', 'src')
               for path in (frozen / folder).rglob('*')
               if path.is_file() and path.suffix in ('.hpp', '.h', '.inc', '.inl', '.def')}
    tokens = shlex.split((build / 'CMakeFiles/vehicle_choice_runtime_tu15_tests.dir/link.txt').read_text())
    libraries = []
    pins = dict(headers)
    for token in tokens[tokens.index('-o') + 2:]:
        if token.startswith('-Wl,@'):
            path = Path(token.removeprefix('-Wl,@')).resolve(strict=True)
            pins[str(path)] = sha(path)
        elif not token.startswith('-'):
            path = (build / token).resolve(strict=True)
            pins[str(path)] = sha(path)
            token = str(path)
        libraries.append(token)
    source = owner / 'source'
    source.mkdir()
    entry = ROOT / 'tools/observe_vehicle_game_lan_tu15.cpp'
    prefix = entry.read_text().split('int main(int argc,char** argv)try {')[0]
    (source / 'route.cpp').write_text(prefix + r'''
int main(int argc,char** argv)try {
    check(argc==3,"Supply recorded input cases and an owned report path");
    std::ifstream in(argv[1]);const auto cases=Json::parse(in);Json rows=Json::array();bool passed=true;
    for(const auto& item:cases) {
        Cache cache(item["map"].get<std::string>(),item["maps"].get<std::string>());
        const auto content=cache.load_content(0,true);const CollisionWorld terrain(content.collision);
        auto point=[](const Json& p){return Vec3{p[0].get<float>(),p[1].get<float>(),p[2].get<float>()};};
        const auto start=point(item["start"]),goal=point(item["goal"]);
        const auto before=std::chrono::steady_clock::now();
        const auto route=collision_walk_route(terrain,content.player,start,goal);
        Json row={{"case",item["case"]},{"start",vector(start)},{"goal",vector(goal)},
            {"route",Json::array()},{"seconds",std::chrono::duration<double>(std::chrono::steady_clock::now()-before).count()},
            {"collision_triangles",terrain.size()},{"passed",!route.empty()}};
        for(const auto p:route)row["route"].push_back(vector(p));
        passed &= !route.empty();rows.push_back(std::move(row));
    }
    std::ofstream out(argv[2]);out<<Json{{"passed",passed},{"cases",rows},
        {"input_automation_only",true},{"in_game_route_following_verified",false}}.dump(2)<<'\n';
    return passed?0:1;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
''')
    cases = []
    for case in args.case:
        outer = json.loads((case / 'report.json').read_text())
        game = json.loads((case / 'gameplay/report.json').read_text())
        target = game['target']
        map_path = outer.get('map_input', {}).get('path') or game['observer_command'][1]
        cases.append(dict(case=str(case), map=map_path,
                          maps=str(Path(map_path).parent),
                          start=target['initial_player']['position'], goal=target['entry']['approach']))
    inputs = owner / 'inputs.json'
    inputs.write_text(json.dumps(cases) + '\n')
    q = lambda value: '"' + str(value).replace('\\', '/').replace('"', '\\"') + '"'
    text = '''cmake_minimum_required(VERSION 3.20)
project(VehicleInputWalkRoute LANGUAGES CXX)
enable_testing()
add_executable(route route.cpp)
target_compile_features(route PRIVATE cxx_std_20)
'''
    text += 'target_include_directories(route PRIVATE ' + q(frozen / 'include') + ' ' + q(frozen / 'src') + ' /usr/include/SDL2)\n'
    text += 'target_link_libraries(route PRIVATE "-Wl,--start-group" ' + ' '.join(map(q, libraries)) + ' "-Wl,--end-group")\n'
    text += 'add_test(NAME collision_walk_routes COMMAND $<TARGET_FILE:route> ' + q(inputs) + ' "${CMAKE_CURRENT_BINARY_DIR}/routes.json")\n'
    text += 'set_tests_properties(collision_walk_routes PROPERTIES TIMEOUT 180)\n'
    (source / 'CMakeLists.txt').write_text(text)
    args.output.mkdir(parents=True, exist_ok=False)
    command = [sys.executable, '-B', str(ROOT / 'tools/run_tests.py'), '--build-worker', '--source', str(source),
               '--target', 'route', '--filter', '^collision_walk_routes$', '--jobs', '2', '--test-jobs', '1',
               '--output', str(args.output), '--retain-report', 'routes.json', '--retain-report', 'Testing/Temporary/LastTest.log']
    result = subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.STDOUT).returncode
    unchanged = all(sha(Path(name)) == pin for name, pin in pins.items())
    path = args.output / 'report.json'
    report = json.loads(path.read_text())
    report.update(borrowed_inputs_unchanged=unchanged, observer_source_sha256=sha(entry),
                  no_native_game_execution=True, in_game_route_following_verified=False)
    report['passed'] &= result == 0 and unchanged
    path.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(passed=report['passed'], report=str(path))), flush=True)
    return 0 if report['passed'] else result or 1


if __name__ == '__main__':
    raise SystemExit(main())
