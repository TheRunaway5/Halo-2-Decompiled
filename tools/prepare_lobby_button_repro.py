#!/usr/bin/env python3
"""Add the focused lobby-button tests and stage exact pre-fix/fixed UI hunks.

Only an announced enclosing run_test.py owner's private source is writable.
The red version restores the actual former silent Start predicate/dispatch
and direct219 Setup choice, retaining the same client/server/library cohort.
No live checkout, foreign build or installed runtime is changed.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil

ROOT = Path(__file__).resolve().parents[1]
TARGET = '''    add_executable(frontend_online_lobby_buttons_tests tests/frontend_online_lobby_buttons_tests.cpp)
    target_include_directories(frontend_online_lobby_buttons_tests PRIVATE include src)
    target_link_libraries(frontend_online_lobby_buttons_tests PRIVATE halo2_renderer CURL::libcurl nlohmann_json::nlohmann_json)
'''
TESTS = '''    if(HALO2_MCC_PARTY_UI_TEST_CACHE)
        foreach(button_case buttons legacy_countdown)
            add_test(NAME frontend_online_lobby_${button_case} COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_matchmaking_http_test.py $<TARGET_FILE:frontend_online_lobby_buttons_tests>
                --mcc-root "${HALO2_MCC_PARTY_UI_TEST_CACHE}")
            set_tests_properties(frontend_online_lobby_${button_case} PROPERTIES
                SKIP_RETURN_CODE 77 TIMEOUT 70 RUN_SERIAL TRUE ENVIRONMENT "H2_TEST_LOBBY_BUTTONS=1")
        endforeach()
        set_property(TEST frontend_online_lobby_legacy_countdown APPEND PROPERTY ENVIRONMENT "H2_TEST_LEGACY_PARTY_COUNTDOWN=1")
    endif()
'''
GPU_TEST = '''        add_test(NAME frontend_online_lobby_buttons_gpu COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/frontend_matchmaking_http_test.py $<TARGET_FILE:frontend_online_lobby_buttons_tests>
            --mcc-root "${HALO2_MCC_PARTY_UI_TEST_CACHE}")
        set_tests_properties(frontend_online_lobby_buttons_gpu PROPERTIES
            TIMEOUT 240 RUN_SERIAL TRUE ENVIRONMENT "H2_TEST_LOBBY_BUTTONS=1;H2_TEST_LOBBY_BUTTONS_GPU=1")
'''
OLD_START = '''  if(f.screen==14 && (online_lobby_countdown_>=0 || entry.destination==-17)) {
    // 0x25106C ->0x19A78E(controller,10,3).
    if(account_.party && account_.party->leader && !account_.busy && online_lobby_matchmaking_ &&
        !selected_playlist_.empty() && account_.party->countdown_supported && online_account_) {
      const auto playlist=std::find_if(matchmaking_.playlists.begin(),matchmaking_.playlists.end(),
        [&](const auto &p){return p.id==selected_playlist_;});
      if(playlist!=matchmaking_.playlists.end())
        online_account_->party_command({"countdown_start",{},playlist->id,playlist->revision});
    }
    return true;
  }
'''


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def replace_once(path, before, after):
    text = path.read_text()
    if before == after:
        return
    if before not in text and after in text:
        return
    assert text.count(before) == 1, f'Private source does not contain exactly one expected hunk: {path.name}'
    path.write_text(text.replace(before, after, 1))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, required=True)
    parser.add_argument('--phase', choices=('configure', 'red', 'green'), default='configure')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    scratch = Path(os.environ['HALO2_TEST_SCRATCH']).resolve(strict=True)
    source = args.source.resolve(strict=True)
    assert source != ROOT and source.is_relative_to(scratch), 'Only this enclosing test owner may be changed'
    owned = scratch / 'lobby-button-repro'
    owned.mkdir(exist_ok=True)
    copied = {}
    for relative in ('tests/frontend_online_lobby_buttons_tests.cpp', 'tests/frontend_matchmaking_http_test.py'):
        target = source / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        if not target.exists() or target.read_bytes() != (ROOT / relative).read_bytes():
            shutil.copyfile(ROOT / relative, target)
        copied[relative] = sha(target)
    cmake = source / 'CMakeLists.txt'
    text = cmake.read_text()
    if 'add_executable(frontend_online_lobby_buttons_tests ' not in text:
        anchor = '    add_executable(frontend_online_tests tests/frontend_online_tests.cpp)\n'
        assert text.count(anchor) == 1
        text = text.replace(anchor, anchor + TARGET, 1)
    if 'NAME frontend_online_lobby_${button_case}' not in text:
        anchor = '    set_tests_properties(frontend_online_identity PROPERTIES SKIP_RETURN_CODE 77)\n'
        assert text.count(anchor) == 1
        text = text.replace(anchor, anchor + TESTS, 1)
    text=text.replace('target_include_directories(frontend_online_lobby_buttons_tests PRIVATE include)',
                      'target_include_directories(frontend_online_lobby_buttons_tests PRIVATE include src)')
    if 'target_sources(frontend_online_lobby_buttons_tests PRIVATE src/ui/frontend.cpp)' not in text:
        anchor='    add_executable(frontend_online_lobby_buttons_tests tests/frontend_online_lobby_buttons_tests.cpp)\n'
        assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'    target_sources(frontend_online_lobby_buttons_tests PRIVATE src/ui/frontend.cpp)\n    target_compile_definitions(frontend_online_lobby_buttons_tests PRIVATE HALO2_SHADER_DIR="${HALO2_RUNTIME_SHADER_DIR}")\n',1)
    if 'halo2_bundle_discord_social(frontend_online_lobby_buttons_tests)' not in text:
        anchor='    target_link_libraries(frontend_online_lobby_buttons_tests PRIVATE halo2_renderer CURL::libcurl nlohmann_json::nlohmann_json)\n'
        assert text.count(anchor)==1
        text=text.replace(anchor,anchor+'    halo2_bundle_discord_social(frontend_online_lobby_buttons_tests)\n    add_dependencies(frontend_online_lobby_buttons_tests halo2_shaders)\n',1)
    if 'NAME frontend_online_lobby_buttons_gpu ' not in text:
        anchor='        set_property(TEST frontend_online_lobby_legacy_countdown APPEND PROPERTY ENVIRONMENT "H2_TEST_LEGACY_PARTY_COUNTDOWN=1")\n'
        assert text.count(anchor)==1
        text=text.replace(anchor,anchor+GPU_TEST,1)
    assert text.count('add_executable(frontend_online_lobby_buttons_tests ') == 1
    assert text.count('NAME frontend_online_lobby_${button_case}') == 1
    if text != cmake.read_text():
        cmake.write_text(text)
    manifest = owned / 'fixed-hunks.json'
    if not manifest.exists():
        party = (source / 'src/ui/frontend_party.cpp').read_text()
        start = party.index('  if(f.screen==14 && (online_lobby_countdown_>=0 || entry.destination==-17)) {')
        end = party.index('  if(f.screen==14 && entry.destination>0)', start)
        fixed_start = party[start:end]
        assert 'online_lobby_start_error()' in fixed_start, 'Freeze the fixed task implementation before staging red'
        proactive = '''    if(i==0 && online_lobby_matchmaking_ && account_.party && !account_.party->countdown_supported &&
        !selected_playlist_.empty())return online_lobby_start_error();
'''
        assert proactive in party
        hunks = [dict(file='src/ui/frontend_party.cpp', fixed=fixed_start, red=OLD_START),
                 dict(file='src/ui/frontend_party.cpp', fixed=proactive, red='    // Pre-fix reproduction: missing capability had no lobby explanation.\n'),
                 dict(file='src/ui/frontend_party.cpp', fixed='{content_.localized(screen,pane.buttons[1].text.string),23,0}',
                      red='{content_.localized(screen,pane.buttons[1].text.string),219,0}'),
                 dict(file='src/ui/frontend_state.cpp', fixed='view.input.start_permission=online_lobby_start_error().empty();',
                      red='view.input.start_permission=view.input.host && online_lobby_matchmaking_ && !selected_playlist_.empty();')]
        manifest.write_text(json.dumps(hunks, indent=2) + '\n')
    hunks = json.loads(manifest.read_text())
    if args.phase != 'configure':
        for hunk in hunks:
            replace_once(source / hunk['file'], hunk['fixed' if args.phase == 'red' else 'red'],
                         hunk['red' if args.phase == 'red' else 'fixed'])
    inputs = ('include/halo2/frontend.hpp', 'src/ui/frontend_party.cpp', 'src/ui/frontend_state.cpp',
              'CMakeLists.txt', *copied)
    report = dict(phase=args.phase, private_source=str(source), scratch=str(scratch),
                  source_sha256={p: sha(source / p) for p in inputs},
                  private_fixed_hunks_sha256=sha(manifest), helper_sha256=sha(__file__),
                  test_names=['frontend_online_lobby_buttons', 'frontend_online_lobby_legacy_countdown', 'frontend_online_lobby_buttons_gpu'],
                  changed_live_checkout=False, changed_foreign_build=False, changed_installed_runtime=False)
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report))


if __name__ == '__main__':
    main()
