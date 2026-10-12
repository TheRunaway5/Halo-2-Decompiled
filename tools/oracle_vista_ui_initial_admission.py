#!/usr/bin/env python3
"""Execute the original UI constructor and initial offline sign-in poll.

The original DLL listener constructor and XUserGetSigninState execute. Win32
mutex/critical-section services are explicit native boundaries. Game-state
and all category/profile constructors execute unchanged. The first frame is
stopped at XNotifyGetNext: no notification is supplied or needed to produce
the initial offline context. The asset context is the actual null fixture
global, and the manager's initial static bytes are retained from the PE.
"""
from pathlib import Path
import hashlib, json, sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from unicorn.x86_const import *
from oracle_vista_mainmenu_game_state import GameState, GAME
from oracle_native_vista_menu_services import PE, EXE, EXE_SHA, STOP, u32, w32

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'analysis/frontend/vista-session-flow-audit/local-session-inputs-original'))
from audit import DLL, DLL_SHA
OUT = ROOT / 'analysis/frontend/vista-session-flow-audit/mainmenu-network-original/ui-initial-poll'
UI = 0xd712c8

class Admission(GameState):
    def __init__(self, exe, dll, identity_state, event_handle, enabled=True):
        super().__init__(exe)
        self.events = []
        self.all_executed = {}
        self.phase = 'game-construct'
        self.event_handle = event_handle
        u = self.u
        u.mem_map(STOP + 0x2000, 0x1000)
        end = max(s['rva'] + max(s['virtual_size'], s['raw_size']) for s in dll.sections)
        u.mem_map(dll.base, (end + 4095) & ~4095)
        for section in dll.sections:
            u.mem_write(dll.base + section['rva'], dll.data[section['raw']:section['raw'] + section['raw_size']])
        self.dll_api = {}
        for index, imp in enumerate(dll.imports()):
            address = STOP + 0x2000 + index * 4
            w32(u, int(imp['iat_va'], 16), address)
            self.dll_api[address] = imp
        w32(u, 0x10794f60, 0)  # Actual optional logger boundary is absent.
        w32(u, 0x107e8654, identity_state)
        u.mem_write(0x8fad99, bytes([int(enabled)]))  # Explicit listener-enable predicate.

    def args(self, count):
        sp = self.u.reg_read(UC_X86_REG_ESP)
        return [u32(self.u, sp + 4 + 4 * i) for i in range(count)]

    def hook(self, u, pc, size, unused):
        if self.phase != 'game-construct' and pc < STOP:
            self.all_executed[pc] = bytes(u.mem_read(pc, size))
        if pc == 0x6a8775:
            self.events.append(['listener', *self.args(2)])
            u.reg_write(UC_X86_REG_EIP, 0x105a5f10)
            return
        if pc == 0x6a878d:
            self.events.append(['signin_query', *self.args(1)])
            u.reg_write(UC_X86_REG_EIP, 0x105b6c30)
            return
        if pc == 0x6a8787:
            self.events.append(['notification_poll_boundary', *self.args(2)])
            u.emu_stop()
            return
        if pc == 0x61ab26:
            self.events.append(['apply_signin', *self.args(1)])
        if pc == 0x5afd25:
            self.events.append(['manager_signin', u.reg_read(UC_X86_REG_ECX), *self.args(1)])
        if pc == 0x44a619:
            self.ret(GAME)
            return
        if pc == 0x44a7b5:
            self.events.append(['game_context', *self.args(1)])
        if pc == 0x60bb89:
            self.events.append(['resource_context'])
        if pc == 0x60a89f:
            self.events.append(['special_ui', u32(u, UI + 8)])
        if pc == 0x6a857d:
            pointer = self.args(1)[0]
            self.events.append(['cancel_overlapped', bytes(u.mem_read(pointer, 0x1c)).hex()])
            self.ret(0, 4)
            return
        if pc == 0x1020b598:
            self.ret()
            return
        if pc in self.dll_api:
            imp = self.dll_api[pc]
            if imp['name'] == 'CreateMutexW':
                self.events.append(['CreateMutexW', *self.args(3)])
                self.ret(self.event_handle, 12)
                return
            if imp['name'] == 'InitializeCriticalSection':
                self.events.append(['InitializeCriticalSection', *self.args(1)])
                self.ret(0, 4)
                return
            if imp['name'] == 'SetEvent':
                # The supplied DLL actually passes its CreateMutexW handle.
                # SetEvent cannot signal a mutex; its result is ignored here.
                self.events.append(['SetEvent_mutex', *self.args(1)])
                self.ret(0, 4)
                return
            raise RuntimeError(f'Unexpected DLL import {imp}')
        if pc in self.api and self.api[pc]['name'] in ('EnterCriticalSection', 'LeaveCriticalSection'):
            self.ret(0, 4)
            return
        super().hook(u, pc, size, unused)

def main():
    exe, dll = PE(EXE, EXE_SHA), PE(DLL, DLL_SHA)
    rows = []
    bodies = {}
    for enabled, event_handle, seed, pending, operation in (
            (enabled, event_handle, seed, pending, operation)
            for enabled in (False, True) for event_handle in (0, 0x5001)
            for seed in (0, 0xa5) for pending, operation in ((0, 0), (1, 0), (1, 0x3e5))):
        # State1 is the actual NativeVistaSession offline provider result.
        oracle = Admission(exe, dll, 1, event_handle, enabled)
        result = oracle.invoke(0x44a82e, GAME)
        assert result['pc'] == STOP
        oracle.phase = 'ui-construct'
        ui_before = bytes([seed]) * 0x610
        oracle.u.mem_write(UI, ui_before)
        result = oracle.invoke(0x61b266, UI)
        assert result['pc'] == STOP
        constructor = bytes(oracle.u.mem_read(UI, 0x610)).hex()
        profile_pointer = u32(oracle.u, GAME + 0xac)
        profile_before = bytearray((seed + i * 17) & 255 for i in range(0x1c0))
        profile_before[0x30] = pending
        profile_before[0x1a4:0x1a8] = operation.to_bytes(4, 'little')
        oracle.u.mem_write(profile_pointer, bytes(profile_before))
        game_before = bytes((seed + i * 19 + 3) & 255 for i in range(8))
        oracle.u.mem_write(GAME, game_before)
        manager_before = bytes([seed, seed ^ 0x91, 0, 0])
        oracle.u.mem_write(0x918210 + 0x1b40, manager_before)
        oracle.phase = 'initial-poll'
        result = oracle.invoke(0x61b562, UI)
        assert result['pc'] in (STOP, 0x6a8787)
        rows.append(dict(listener_enabled=enabled, event_handle=event_handle, seed=seed,
                         identity_state=1, before=ui_before.hex(), constructor=constructor,
                         after=bytes(oracle.u.mem_read(UI, 0x610)).hex(), events=oracle.events,
                         game_before=game_before.hex(), profile_before=profile_before.hex(),
                         manager_before=manager_before.hex(),
                         game_root=bytes(oracle.u.mem_read(GAME, 8)).hex(),
                         game_profile=bytes(oracle.u.mem_read(profile_pointer, 0x1c0)).hex(),
                         manager=bytes(oracle.u.mem_read(0x918210 + 0x1b40, 4)).hex(),
                         instructions=oracle.visits, stopped=result['pc']))
        bodies.update(oracle.all_executed)
    OUT.mkdir(parents=True, exist_ok=True)
    cs = Cs(CS_ARCH_X86, CS_MODE_32)
    (OUT / 'executed.asm').write_text('\n'.join(
        f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}'
        for pc, raw in sorted(bodies.items()) for i in cs.disasm(raw, pc)) + '\n')
    report = dict(source_sha256=EXE_SHA, dll_sha256=DLL_SHA,
                  script_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  rows=rows, limits=__doc__)
    (OUT / 'execution.json').write_text(json.dumps(report, indent=2) + '\n')
    queries = []
    for state in (0, 1, 2, 3, 0x7fffffff, 0xffffffff):
        oracle = Admission(exe, dll, state, 0x5001)
        result = oracle.invoke(0x105b6c30, 0, [3])
        assert result['pc'] == STOP
        queries.append(dict(state=state, result=result['eax']))
    fixture = ROOT / 'tests/fixtures/vista_ui_initial_admission_original.json'
    fixture.write_text(json.dumps(dict(rows=rows, queries=queries), separators=(',', ':')) + '\n')
    report['fixture_sha256'] = hashlib.sha256(fixture.read_bytes()).hexdigest()
    (OUT / 'execution.json').write_text(json.dumps(report, indent=2) + '\n')
    for row in rows:
        after = bytes.fromhex(row['after'])
        valid = row['event_handle'] and row['listener_enabled']
        assert int.from_bytes(after[8:12], 'little') == (1 if valid else 0)
        assert after[0xc] == (0 if valid else 1)
    print(json.dumps(dict(cases=len(rows), events=[r['events'] for r in rows],
                          instructions=sum(r['instructions'] for r in rows))))

if __name__ == '__main__':
    main()
