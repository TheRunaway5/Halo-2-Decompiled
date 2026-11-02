#!/usr/bin/env python3
"""Pin literal modern party callers and the actual current native IO adapter."""
import argparse
import hashlib
import json
from pathlib import Path


def block(text, token):
    start = text.index(token)
    brace = text.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return start, end


def definition(text, token):
    start, end = block(text, token)
    return text[text.rfind('\n', 0, start) + 1:end]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--reference', type=Path, required=True)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--before', type=Path, required=True)
    p.add_argument('--after', type=Path, required=True)
    args = p.parse_args()
    provenance = json.loads((args.reference / 'provenance.json').read_text())
    inputs = {}
    for relative in ('src/ui/frontend_party.cpp', 'src/ui/frontend_state.cpp', 'src/ui/frontend_clans.cpp'):
        path = args.reference / (Path(relative).name + '.reference')
        if hashlib.sha256(path.read_bytes()).hexdigest() != provenance['source_pins'][relative]:
            raise ValueError('Captured party caller differs from its pinned input: ' + relative)
        inputs[relative] = path.read_text()
    methods = ['invite_notice()', 'act_invite_notice(', 'online_party_screen(', 'party_invite_picker()',
               'switch_invite_tab(', 'update_online_privacy()', 'accept_online_party()']
    old = ['constexpr unsigned party_context=42,invite_context=45;']
    old += [definition(inputs['src/ui/frontend_party.cpp'], 'MenuState::' + token) for token in methods]
    old.append(definition(inputs['src/ui/frontend_party.cpp'], 'MenuState::party_command('))
    state = inputs['src/ui/frontend_state.cpp']
    start, end = block(state, 'if(screen==25 && mode_==3 && account_.party && account_.party->privacy)')
    old.append('void MenuState::opened_privacy(){const auto screen=frame().screen;\n' + state[start:end] + '\n}')
    start, end = block(state, 'if(f.screen==25 && mode_==3) {')
    old.append('void MenuState::select_privacy(int value){const auto f=frame();struct{int value;}entry{value};\n' + state[start:end] + '\n}')
    clans = inputs['src/ui/frontend_clans.cpp']
    line = next(line for line in clans.splitlines() if 'std::erase_if(dismissed_party_invites_' in line)
    old.append('void MenuState::prune_dismissed(){' + line + '\n}')
    old.append('void MenuState::clear_dismissed(){dismissed_party_invites_.clear();}')
    current = (args.root / 'src/ui/frontend_party.cpp').read_text()
    adapter = (args.root / 'src/ui/frontend_matchmaking_party_flow.cpp').read_text()
    new = [definition(adapter, 'flow_bytes('), definition(adapter, 'flow_text(')]
    new.append(definition(adapter, 'struct FrontendPartyFlowNativeIo ') + ';')
    new += [definition(adapter, 'MenuState::' + name) for name in ('party_flow_drive(', 'party_command(')]
    new += [definition(current, 'MenuState::' + token) for token in methods]
    new.append('void MenuState::opened_privacy(){party_flow_drive(FrontendPartyFlowAction::OpenedPrivacy);}')
    new.append('void MenuState::select_privacy(int value){party_flow_drive(FrontendPartyFlowAction::SelectPrivacy,value);}')
    new.append('void MenuState::prune_dismissed(){std::vector<std::string> ids;for(const auto&i:account_.party_invitations)ids.push_back(i.id);party_flow_.prune(ids);}')
    new.append('void MenuState::clear_dismissed(){party_flow_.clear_dismissed();}')
    for target, lines in ((args.before, old), (args.after, new)):
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text('// Literal modern callers and declared synthetic native IO.\n' + '\n'.join(lines) + '\n')


if __name__ == '__main__':
    main()
