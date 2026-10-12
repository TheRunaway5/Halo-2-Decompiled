#!/usr/bin/env python3
"""Unchanged Vista 638CE9/638EE0/63902E/638D16/638D4C instruction results.

Platform, map, shared UI, sound and clock services are explicit fixture inputs.
This proves owner mutations and service ordering, not audio output or loader
ownership. No original instruction is patched.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys

from unicorn import Uc, UC_ARCH_X86, UC_MODE_32, UC_HOOK_CODE
from unicorn.x86_const import *

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'analysis/frontend/vista-session-flow-audit/local-session-inputs-original'))
from audit import PE, EXE, EXE_SHA

BASE, STOP, DATA, STACK, SP = 0x400000, 0x1800000, 0x1810000, 0x1900000, 0x1908000
OWNER, SCENARIO, SHARED, REQUEST = DATA, DATA + 0x100, DATA + 0x200, DATA + 0x500
CONVERT, FLOAT_STOP = STOP + 0x100, STOP + 0x200
PATH = b'levels\\declared\\declared\0'


class Original:
    def __init__(self, e):
        self.u = u = Uc(UC_ARCH_X86, UC_MODE_32)
        u.mem_map(BASE, (len(e.data) + 4095) & ~4095)
        u.mem_write(BASE, e.data)
        u.mem_map(STOP, 0x1000)
        u.mem_map(DATA, 0x10000)
        u.mem_map(STACK, 0x10000)
        # Explicit caller spills the actual x87 result to a binary32 slot.
        u.mem_write(FLOAT_STOP, b'\xd9\x1d' + struct.pack('<I', DATA + 0xF000) + b'\x90')
        self.w(0x86D7CC, CONVERT)
        u.hook_add(UC_HOOK_CODE, self.hook)

    def w(self, a, v):
        self.u.mem_write(a, struct.pack('<I', v & 0xFFFFFFFF))

    def r(self, a):
        return struct.unpack('<I', self.u.mem_read(a, 4))[0]

    def args(self, n):
        return [self.r(self.u.reg_read(UC_X86_REG_ESP) + 4 + 4 * i) for i in range(n)]

    def ret(self, value=0, pop=0):
        sp = self.u.reg_read(UC_X86_REG_ESP)
        self.u.reg_write(UC_X86_REG_EAX, value & 0xFFFFFFFF)
        self.u.reg_write(UC_X86_REG_ESP, sp + 4 + pop)
        self.u.reg_write(UC_X86_REG_EIP, self.r(sp))

    def event(self, service, *args):
        self.events.append([service, *args])

    def hook(self, u, pc, size, unused):
        self.visits += 1
        if pc in (STOP, FLOAT_STOP + 6):
            u.emu_stop()
        elif pc == 0x6096B4:
            self.event('screen', self.input['screen'])
            self.ret(self.input['screen'])
        elif pc == 0x621B68:
            self.event('permit_audio', self.input['permit_audio'])
            self.ret(self.input['permit_audio'])
        elif pc == 0x6096AE:
            self.event('ui_clock', self.input['ui_clock'])
            self.ret(self.input['ui_clock'])
        elif pc == 0x437E51:
            self.event('network_clock', self.input['network_clock'])
            self.ret(self.input['network_clock'])
        elif pc == 0x60BB89:
            self.event('shared_ui', self.input['shared'], self.input['music'], self.input['duration'])
            self.ret(SHARED if self.input['shared'] else 0)
        elif pc == 0x485DA1:
            datum, none, gain = self.args(3)
            assert (datum, none, gain) == (self.input['music'], 0xFFFFFFFF, 0x3F800000)
            self.event('play', datum, none, gain, self.input['new_handle'])
            self.ret(self.input['new_handle'])
        elif pc == 0x485DEB:
            self.event('stop', self.args(1)[0])
            self.ret()
        elif pc == 0x609E82:
            self.event('request', self.input['request'])
            self.ret(REQUEST if self.input['request'] else 0)
        elif pc == CONVERT:
            args = self.args(6)
            assert args[:4] == [65001, 0, REQUEST + 8, 0xFFFFFFFF] and args[5] == 0x104
            self.event('path_conversion')
            u.mem_write(args[4], PATH)
            self.ret(len(PATH), 24)
        elif pc == 0x44B633:
            assert bytes(u.mem_read(self.args(1)[0], len(PATH))) == PATH
            self.event('map_state', self.input['map_state'])
            self.ret(self.input['map_state'])
        elif pc == 0x4317A9:
            assert self.args(2) == [0, REQUEST + 8]
            self.event('estimate', self.input['estimate'])
            self.ret(self.input['estimate'])

    def run(self, kind, before, supplied):
        self.input = supplied
        self.events, self.visits = [], 0
        u = self.u
        u.reg_write(UC_X86_REG_FPCW, 0x027F)
        u.reg_write(UC_X86_REG_FPSW, 0)
        u.reg_write(UC_X86_REG_FPTAG, 0xFFFF)
        u.reg_write(UC_X86_REG_MXCSR, 0x1F80)
        u.mem_write(OWNER, struct.pack('<6I', *before))
        self.w(0x879E74, SCENARIO if supplied['scenario'] else 0)
        u.mem_write(SCENARIO + 0x10, struct.pack('<H', supplied['scenario_kind']))
        self.w(SHARED + 0x1BC, supplied['music'])
        self.w(SHARED + 0x1C0, supplied['duration'])
        u.mem_write(REQUEST + 8, PATH.decode().encode('utf-16le'))
        u.mem_write(SP - 0x3000, bytes(0x4000))
        self.w(SP, FLOAT_STOP if kind == 'secondary' else STOP)
        u.reg_write(UC_X86_REG_ESP, SP)
        u.reg_write(UC_X86_REG_ECX, OWNER)
        address = {'initialize': 0x638CE9, 'update': 0x63902E, 'desired': 0x638EE0,
                   'ready': 0x638D16, 'secondary': 0x638D4C}[kind]
        u.emu_start(address, STOP + 0x1000, count=10000)
        assert u.reg_read(UC_X86_REG_EIP) in (STOP, FLOAT_STOP + 6)
        return dict(kind=kind, before=before, supplied=supplied,
            after=list(struct.unpack('<6I', u.mem_read(OWNER, 24))),
            result=self.r(DATA + 0xF000) if kind == 'secondary' else u.reg_read(UC_X86_REG_EAX) & 255 if kind == 'ready' else None,
            events=self.events, visits=self.visits)


def main():
    e = PE(EXE, EXE_SHA)
    original = Original(e)
    default = dict(scenario=True, scenario_kind=2, screen=6, permit_audio=1,
        ui_clock=7001, network_clock=10000, shared=True, music=0xE87C51A1,
        duration=5500, new_handle=0xAABB0001, request=True, map_state=1, estimate=5500)
    cases = []
    def case(kind, before, **overrides):
        cases.append(original.run(kind, before, {**default, **overrides}))
    case('initialize', [0x11111111 * (i + 1) for i in range(6)])
    for scenario, scenario_kind in [(False, 2), (True, 0), (True, 1), (True, 2)]:
        for screen in range(260):
            for desired in (0, 1, 0x12345678):
                case('desired', [0, 0, desired, 0, 0xFFFFFFFF, 777],
                    scenario=scenario, scenario_kind=scenario_kind, screen=screen)
    for map_state in (0, 1, 2, 3, 4, 0xFFFFFFFF):
        for request in (False, True):
            for estimate in (0, 5499, 5500, 5501, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF):
                for desired in (0, 1):
                    case('desired', [0, 0, desired, 0, 0xFFFFFFFF, 777],
                         screen=209, map_state=map_state, request=request, estimate=estimate)
    states = [[0, 0, 0, 0, 0xFFFFFFFF, 0], [1, 1, 1, 1000, 0xAABB0001, 7],
              [0, 0, 0, 1000, 0xFFFFFFFF, 7], [1, 1, 1, 0xFFFFFF00, 0xFFFFFFFF, 7],
              [7, 8, 7, 0, 0xAABB0001, 7]]
    for before in states:
        for screen in (6, 13, 19, 247, 259):
            for permit_audio in (0, 1):
                for ui_clock in (0, 1, 6999, 7000, 7001, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF):
                    for shared, music in [(False, 0xFFFFFFFF), (True, 0xFFFFFFFF), (True, 0xE87C51A1)]:
                        case('update', before, screen=screen, permit_audio=permit_audio,
                            ui_clock=ui_clock, shared=shared, music=music)
    for desired in (0, 1, 7):
        for handle in (0xFFFFFFFF, 0, 0xAABB0001):
            for duration in (0, 1, 2, 5500, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF):
                for elapsed in (0, 1, 5499, 5500, 5501, 0x7FFFFFFF, 0x80000000, 0xFFFFFFFF):
                    for shared in (False, True):
                        for kind in ('ready', 'secondary'):
                            case(kind, [1, 0, desired, 9, handle, 0xFFFFFF00],
                                duration=duration, network_clock=(elapsed + 0xFFFFFF00) & 0xFFFFFFFF, shared=shared)
    report = dict(schema='H2V_LOADING_MUSIC_ORIGINAL_V1', source_sha256=EXE_SHA,
        tool_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        instruction_visits=sum(c['visits'] for c in cases), boundary=__doc__, cases=cases)
    out = ROOT / 'tests/fixtures/vista_loading_music_original.json'
    out.write_text(json.dumps(report, separators=(',', ':')) + '\n')
    evidence = ROOT / 'analysis/frontend/vista-session-flow-audit/native-loading-presentation-original/music-original.json'
    evidence.write_text(json.dumps({k: v for k, v in report.items() if k != 'cases'}, indent=2) + '\n')
    print(json.dumps(dict(cases=len(cases), instructions=report['instruction_visits'],
        fixture_sha256=hashlib.sha256(out.read_bytes()).hexdigest())))


if __name__ == '__main__':
    main()
