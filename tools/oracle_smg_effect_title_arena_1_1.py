#!/usr/bin/env python3
"""Original whole12000 after its actual primary/native error mapper.

No title arena, NT allocation success, DirectSound or DSP state is supplied.
The earlier three negative/lookup roots remain immutable. Only complete primary
ordinal301's native NtDll delegate returns its actual measured error code.
"""
import ctypes
import json
import os
import subprocess
from pathlib import Path
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX
import oracle_smg_effect_dsp_owner_1_1 as dsp
from oracle_smg_effect_owner_1_1 import sha, save, encoded
from oracle_mp_spawn_local_map_clock import body
from oracle_mp_spawn_process_heap_world import KernelBoundary

ROOT = dsp.ROOT
OLD_OUT = dsp.OUT
OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/smg-effect-title-arena'
PRIOR = {**dsp.PRIOR,
    'tools/oracle_smg_effect_dsp_owner_1_1.py': '749ec42cc7e379899fd2efdc467c818a69373479724895ebe668510457fd59d6',
    'tools/research_smg_effect_dsp_owner_1_1.py': 'f60c47f2b9705d11cf03cf3de57d4b9bf24f6b00c7f33241a33f246f5ac680a0',
    'tools/audit_smg_effect_dsp_owner.py': '179a33333b92ee5087027f7d93957463e4e77df511f6443b7438627433c03745',
    'docs/MP_SMG_EFFECT_DSP_OWNER_RESEARCH.md': 'bbeb6eab2bb51b020746740f543b51dcce38a3bf9c2aacb81a4534ea9271a1ad',
    'analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/measured/report.json': '01e36f2d46232289678b90637b47f4d2381781c1bb555794258a8390ed96f679',
    'analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner/manifest.json': '51d914f8f892ceb538e831e11c6fec3fe68a9bf7998c6a40ec3860b2a9c8c80d'}


def load_frozen_rtl():
    contract = json.loads((OLD_OUT / 'provider/rtl-contract.json').read_text())
    assert sha(dsp.RTL) == contract['primary_rtl_sha256']
    assert sha(dsp.SDK) == contract['primary_sdk_sha256']
    assert sha(OLD_OUT / 'provider/compiled-rtl.cpp') == contract['generated_source_sha256']
    assert sha(OLD_OUT / 'provider/compiled-rtl.bin') == contract['compiled_text_sha256']
    assert sha(contract['actual_host_strlen_library']) == contract['actual_host_strlen_library_sha256']
    library = ctypes.CDLL(contract['actual_host_strlen_library'])
    library.strlen.argtypes, library.strlen.restype = [ctypes.c_char_p], ctypes.c_size_t
    return contract, library


def mapper_provider():
    extracted = body(dsp.RTL.read_text(), r'xbox::RtlNtStatusToDosError\s*\([^)]*\)')
    assert 'ULONG ret = NtDll::RtlNtStatusToDosError(Status);' in extracted
    assert 'XBAPI ULONG NTAPI RtlNtStatusToDosError' in dsp.SDK.read_text()
    source = '''#include <windows.h>
#include <cstdio>
#include <cassert>
#define LOG_FUNC_ONE_ARG(x)
#define RETURN(x) return (x)
namespace xbox { typedef ULONG ulong_xt; typedef LONG ntstatus_xt;
ulong_xt NTAPI RtlNtStatusToDosError(ntstatus_xt Status); }
namespace NtDll { typedef ULONG (NTAPI* Mapper)(LONG); Mapper RtlNtStatusToDosError; }
xbox::ulong_xt NTAPI xbox::RtlNtStatusToDosError(xbox::ntstatus_xt Status)
''' + extracted + '''
int main() {
 auto module=GetModuleHandleA("ntdll.dll"); assert(module);
 auto address=GetProcAddress(module,"RtlNtStatusToDosError"); assert(address);
 NtDll::RtlNtStatusToDosError=reinterpret_cast<NtDll::Mapper>(address);
 char path[MAX_PATH]{};assert(GetModuleFileNameA(module,path,MAX_PATH));
 std::printf("{\\"actual_native_module\\":\\"%s\\",\\"actual_native_export16_hex\\":\\"",path);
 for(unsigned i=0;i<16;++i)std::printf("%02x",reinterpret_cast<unsigned char*>(address)[i]);
 std::printf("\\"}\\n");std::fflush(stdout);
 unsigned status;while(std::scanf("%x",&status)==1) {
  auto result=xbox::RtlNtStatusToDosError(static_cast<LONG>(status));
  std::printf("{\\"actual_status32\\":%u,\\"actual_dos_error32\\":%u}\\n",status,result);
  std::fflush(stdout);
 }
}
'''
    # Windows paths are emitted with slash separators solely for JSON encoding.
    source = source.replace('std::printf("{\\"actual_native_module', 'for(auto& c:path)if(c==char(92))c=char(47);\n std::printf("{\\"actual_native_module')
    out = OUT / 'provider'
    cpp, exe = out / 'compiled-error-mapper.cpp', out / 'compiled-error-mapper.exe'
    save(cpp, source.encode())
    command = ['i686-w64-mingw32-g++', '-std=c++20', '-O0', '-static', '-Wl,--no-insert-timestamp', str(cpp), '-o', str(exe)]
    result = subprocess.run(command, capture_output=True, text=True)
    save(out / 'mapper-compiler.log', (result.stdout + result.stderr).encode())
    assert result.returncode == 0, result.stderr
    native_dll = Path('/usr/lib/wine/i386-windows/ntdll.dll')
    metadata = dict(primary_rtl_sha256=sha(dsp.RTL), primary_sdk_sha256=sha(dsp.SDK), extracted_complete_primary_body=extracted,
                    generated_source_sha256=sha(cpp), compiled_executable_sha256=sha(exe), compiler_command=command,
                    native_ntdll_file=str(native_dll), native_ntdll_sha256=sha(native_dll),
                    abi='Ordinal301 ULONG NTAPI RtlNtStatusToDosError(NTSTATUS); stdcall one32-bit argument, actual32-bit return. Only EAX/return routing is transported; volatile-register/flags and native TEB side effects are outside this bounded service.',
                    scope='Complete unchanged pinned primary compatibility301 body calls actual resolved native Wine32 NtDll RtlNtStatusToDosError. Actual error mapping, not an invented startup success or status. No original allocation/device/DSP/game/particle field written by this provider.')
    save(out / 'mapper-contract.json', encoded(metadata))
    return exe, metadata


class Mapper:
    def __init__(self):
        executable, self.metadata = mapper_provider()
        source = dsp.config.frame.tick.system.life.birth.local.base.owner.player_source
        self.process = subprocess.Popen(['wine', str(executable)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                                        env=dict(os.environ, WINEDEBUG='-all', WINEPREFIX=str(source.CONTEXT_OUT / 'wine-prefix')))
        line = self.process.stdout.readline()
        assert line, self.process.stderr.read()
        self.actual_module = json.loads(line)
        assert self.actual_module['actual_native_module'].lower().endswith('/windows/system32/ntdll.dll')
        self.rows = []

    def query(self, status):
        self.process.stdin.write(f'{status:08x}\n')
        self.process.stdin.flush()
        row = json.loads(self.process.stdout.readline())
        assert row['actual_status32'] == status
        self.rows.append(row)
        return row['actual_dos_error32']

    def close(self):
        _, stderr = self.process.communicate(timeout=15)
        assert self.process.returncode == 0, stderr
        return stderr


class ArenaTrace(dsp.DspTrace):
    def __init__(self, *args, **kwargs):
        self.mapper = Mapper()
        self.mapper_calls = []
        super().__init__(*args, **kwargs)

    def hook(self, guest, address, size, user):
        if self.life_on and self.native_clock is not None and address == 0x8000012d:
            stack = guest.reg_read(UC_X86_REG_ESP)
            status = self.u(stack + 4)
            before = self.cpu()
            actual = self.mapper.query(status)
            returned = self.u(stack)
            guest.reg_write(UC_X86_REG_EAX, actual)
            guest.reg_write(UC_X86_REG_ESP, stack + 8)
            guest.reg_write(UC_X86_REG_EIP, returned)
            self.mapper_calls.append(dict(order=self.life_order, phase=self.active, kernel_ordinal=301, original_return=f'{returned:08x}', actual_status_word=f'{status:08x}', actual_native_return_word=f'{actual:08x}', before_cpu=before, after_cpu=self.cpu(), native_observation_index=len(self.mapper.rows) - 1, original_game_or_particle_or_success_supplied=False))
            self.life_order += 1
            return
        if self.life_on and self.native_clock is not None and address == 0x800000b8:
            stack = guest.reg_read(UC_X86_REG_ESP)
            args = [self.u(stack + 4 + index * 4) for index in range(5)]
            self.new_invalid = dict(boundary='Original title VirtualAlloc reaches NtAllocateVirtualMemory. Actual full VM/VMA/PTE/PFN arena allocation ownership is unavailable; no pointer/size/status output supplied.', instruction=f'{address:08x}', kernel_ordinal=184, original_return=f'{self.u(stack):08x}', arguments=[f'{value:08x}' for value in args], actual_input_base_word=f'{self.u(args[0]):08x}', actual_input_size_word=f'{self.u(args[2]):08x}', original_base_output_pointer=f'{args[0]:08x}', original_size_output_pointer=f'{args[2]:08x}', cpu=self.cpu(), source_arena517ff0_c_hex=self.raw(0x517ff0, 0xc), original_result_supplied=False)
            raise KernelBoundary()
        super().hook(guest, address, size, user)


def main():
    pin = sha(__file__)
    active = []
    original_save = dsp.config.save

    class CapturedTrace(ArenaTrace):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            active.append(self)

    def capture(path, contents):
        if path.name == 'report.json':
            report = json.loads(contents)
            trace = active[0]
            report.update(schema='source-smg-effect-title-arena-v1', oracle_sha256=pin,
                          underlying_dsp_oracle_sha256=sha(dsp.__file__), actual_error_mapper_calls=trace.mapper_calls,
                          actual_error_mapper_observations=trace.mapper.rows, actual_error_mapper_module=trace.mapper.actual_module,
                          verified_error_mapper_contract=trace.mapper.metadata)
            report['boundaries'].append('Actual ordinal301 primary/native error delegate only. Whole12000 owns original VirtualAlloc request, but its actual arena/kernel184 output and full title/driver/DSP/gameplay admission remain unavailable.')
            contents = encoded(report)
        original_save(path, contents)

    dsp.compile_rtl, dsp.OUT, dsp.PRIOR, dsp.DspTrace = load_frozen_rtl, OUT, PRIOR, CapturedTrace
    dsp.config.save = capture
    try:
        dsp.main()
    finally:
        for trace in active:
            save(OUT / 'provider/mapper-stderr.log', trace.mapper.close().encode())
    assert sha(__file__) == pin


if __name__ == '__main__':
    main()
