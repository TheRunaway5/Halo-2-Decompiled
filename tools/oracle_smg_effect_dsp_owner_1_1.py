#!/usr/bin/env python3
"""Original audio-device/DSP admission after unchanged configuration ownership.

No audio device, DSP record, section reference, HRESULT or game admission is
supplied. Existing independently verified compatibility lock/clock owners are
retained. This first bounded case measures the natural driver prerequisite.
"""
import ctypes
import hashlib
import json
import re
import subprocess
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX
import oracle_smg_effect_frame_configuration_1_1 as config
from oracle_smg_effect_owner_1_1 import sha, encoded, save
from oracle_mp_spawn_process_heap_world import KernelBoundary
from oracle_mp_spawn_local_map_clock import body

ROOT = config.ROOT
OUT = ROOT / 'analysis/multiplayer/integration/inventory-source/smg-effect-dsp-owner'
PRIOR = {**config.PRIOR,
    'tools/oracle_smg_effect_frame_configuration_1_1.py': '33ca0dd772ddf29a6feb6659a8fc9dc2603789af4dacff4482d7b1d318898fd0',
    'docs/MP_SMG_EFFECT_FRAME_CONFIGURATION_RESEARCH.md': '7fa905daab1e4b8bc6ae8751ad48ba61b3b2736e5bec095b79b0fb65091298fe',
    'analysis/multiplayer/integration/inventory-source/smg-effect-frame-configuration/measured/report.json': '5b6ea02bcb865722f4ace643721ab037a463246e6391d8983abb89a181b76c77',
    'analysis/multiplayer/integration/inventory-source/smg-effect-frame-configuration/manifest.json': 'ed0a2faaf89d1b348d0f46f6ebb9d94208dd69858f505c0f0c60ebcd78b58a32'}
ENTRIES = {0x12000, 0x2dc2d8, 0x33785c, 0x2da22b, 0x2205c0, 0x383dd7, 0x3804de, 0x192d30, 0x2dbda1, 0x2dbcfb, 0x2dbd62, 0x381ead, 0x192e50}
RTL_CODE, STRLEN_HELPER = 0x0c200000, 0x0c210000
RTL = ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/Cxbx-Reloaded-EmuKrnlRtl.cpp'
SDK = ROOT / 'analysis/multiplayer/integration/spawn-source/kernel-sync-world/XboxDev-xboxkrnl.h'


def compile_rtl():
    """Compile complete pinned primary string bodies, without game defaults."""
    primary = RTL.read_text()
    extracted = {name: body(primary, r'xbox::' + name + r'\s*\([^)]*\)') for name in ('RtlUpperChar', 'RtlInitAnsiString', 'RtlEqualString')}
    source = '''// Complete extracted pinned primary compatibility string bodies.
typedef unsigned short USHORT; typedef unsigned char BOOLEAN;
typedef char CHAR; typedef char CCHAR; typedef char* PCHAR;
typedef const char* PCSZ; typedef unsigned int size_t;
struct STRING { USHORT Length, MaximumLength; CHAR* Buffer; };
typedef STRING* PSTRING; typedef STRING* PANSI_STRING;
static_assert(sizeof(STRING)==8 && __builtin_offsetof(STRING,Buffer)==4);
#define TRUE 1
#define FALSE 0
#define NULL 0
#define LOG_FUNC_BEGIN
#define LOG_FUNC_ARG(x)
#define LOG_FUNC_ARG_OUT(x)
#define LOG_FUNC_END
#define LOG_FUNC_ONE_ARG(x)
#define RETURN(x) return (x)
extern "C" size_t strlen(const char*);
typedef char char_xt; typedef unsigned char byte_xt;
extern "C" char __attribute__((stdcall)) RtlUpperChar(char Character);
'''
    signatures = {'RtlUpperChar': 'char RtlUpperChar(char Character)', 'RtlInitAnsiString': 'void RtlInitAnsiString(PANSI_STRING DestinationString, PCSZ SourceString)', 'RtlEqualString': 'BOOLEAN RtlEqualString(PSTRING String1, PSTRING String2, BOOLEAN CaseInSensitive)'}
    for name in extracted:
        signature = signatures[name].replace(name, '__attribute__((stdcall)) ' + name)
        source += 'extern "C" ' + signature + '\n' + extracted[name] + '\n'
    out = OUT / 'provider'
    cpp, obj, elf, binary = [out / ('compiled-rtl.' + ext) for ext in ('cpp', 'o', 'elf', 'bin')]
    save(cpp, source.encode())
    command = ['c++', '-m32', '-mno-sse', '-mno-sse2', '-O2', '-ffreestanding', '-fno-pie', '-fno-exceptions', '-fno-rtti', '-fno-stack-protector', '-fno-asynchronous-unwind-tables', '-c', str(cpp), '-o', str(obj)]
    result = subprocess.run(command, capture_output=True, text=True)
    save(out / 'rtl-compiler.log', (result.stdout + result.stderr).encode())
    assert result.returncode == 0, result.stderr
    subprocess.run(['ld', '-m', 'elf_i386', '-Ttext', hex(RTL_CODE), '-e', 'RtlInitAnsiString', '--defsym', 'strlen=' + hex(STRLEN_HELPER), str(obj), '-o', str(elf)], check=True)
    subprocess.run(['objcopy', '-O', 'binary', '--only-section=.text', str(elf), str(binary)], check=True)
    symbols = subprocess.check_output(['nm', '-n', str(elf)], text=True)
    entries = {name: int(next(line.split()[0] for line in symbols.splitlines() if line.endswith(' ' + name)), 16) for name in extracted}
    libc_path = '/usr/lib/libc.so.6'
    libc = ctypes.CDLL(libc_path)
    libc.strlen.argtypes, libc.strlen.restype = [ctypes.c_char_p], ctypes.c_size_t
    metadata = dict(primary_rtl_sha256=sha(RTL), primary_sdk_sha256=sha(SDK), primary_rtl_file=str(RTL.relative_to(ROOT)), primary_sdk_file=str(SDK.relative_to(ROOT)), extracted_full_bodies=extracted, generated_source_sha256=sha(cpp), compiled_text_sha256=sha(binary), compiled_text_hex=binary.read_bytes().hex(), entries={name: f'{value:08x}' for name, value in entries.items()}, compiler_command=command, actual_host_strlen_library=libc_path, actual_host_strlen_library_sha256=sha(libc_path), scope='Complete primary compatibility RTL string bodies execute as compiled 32-bit guest code. Only their libc strlen dependency reads the actual captured NUL-terminated bytes through the actual host libc. No selected descriptor fields, equality result, XBE section handle/refcount or success is written by the observer; this is not original Xbox kernel code.')
    save(out / 'rtl-contract.json', encoded(metadata))
    return metadata, libc


class DspTrace(config.ConfigurationTrace):
    def __init__(self, *args, **kwargs):
        self.dsp_records, self.dsp_calls = [], []
        self.dsp_root_argument, self.section_root_argument = None, None
        self.rtl_metadata, self.libc = compile_rtl()
        self.rtl_calls, self.rtl_pending, self.strlen_calls = [], [], []
        self.rtl_executed = set()
        super().__init__(*args, **kwargs)
        assert all(RTL_CODE + 0x1000 <= left or RTL_CODE > right for left, right, _ in self.g.mem_regions())
        assert all(STRLEN_HELPER + 0x1000 <= left or STRLEN_HELPER > right for left, right, _ in self.g.mem_regions())
        self.g.mem_map(RTL_CODE, 0x1000)
        self.g.mem_write(RTL_CODE, bytes.fromhex(self.rtl_metadata['compiled_text_hex']))
        self.g.mem_map(STRLEN_HELPER, 0x1000)
        self.starts |= ENTRIES
        for entry in sorted(ENTRIES):
            flow = config.frame.tick.system.life._trace(self.code, entry, self.starts)
            self.life_routines[entry] = flow
            for address in flow.code:
                self.life_owners[address] = entry

    def dsp_state(self):
        driver = self.u(0x526474)
        dsp = self.u(0x518480)
        return dict(driver_pointer=f'{driver:08x}', driver2ad8_hex=self.raw(driver, 0x2ad8) if driver else None,
                    actual_device_pointer=f'{self.u(driver+0x2ab0):08x}' if driver else None,
                    dsp_pointer=f'{dsp:08x}', dsp48_hex=self.raw(dsp, 0x48) if dsp else None,
                    owner_globals52646c_10_hex=self.raw(0x52646c, 0x10),
                    owner_globals518478_10_hex=self.raw(0x518478, 0x10),
                    source_arena517ff0_c_hex=self.raw(0x517ff0, 0xc),
                    directsound_static_critical_section38d158_1c_hex=self.raw(0x38d158, 0x1c),
                    original_header2000_hex=self.raw(0x10000, 0x2000),
                    frame=self.frame_state(), current=self.snapshot())

    def run(self, entry, argv=(), regs=()):
        if entry == 0x2205c0 and self.dsp_root_argument is not None:
            assert not argv
            argv = (self.dsp_root_argument,)
        if entry == 0x2dbda1 and self.section_root_argument is not None:
            assert not argv
            argv = (self.section_root_argument,)
        return super().run(entry, argv, regs)

    def hook(self, guest, address, size, user):
        for returned, row in self.rtl_pending[:]:
            if address == returned:
                row.update(after_cpu=self.cpu(), output_descriptor8_hex=self.raw(int(row['arguments'][0], 16), 8), return_al=f'{guest.reg_read(UC_X86_REG_EAX) & 255:02x}')
                self.rtl_pending.remove((returned, row))
        if RTL_CODE <= address < RTL_CODE + 0x1000:
            self.rtl_executed.add(address)
        if self.life_on and self.native_clock is not None and address in (0x80000121, 0x80000117):
            stack = guest.reg_read(UC_X86_REG_ESP)
            name, count = ('RtlInitAnsiString', 2) if address == 0x80000121 else ('RtlEqualString', 3)
            args = [self.u(stack + 4 + index * 4) for index in range(count)]
            row = dict(order=self.life_order, ordinal=address & 0x7fffffff, name=name, original_return=f'{self.u(stack):08x}', arguments=[f'{value:08x}' for value in args], before_cpu=self.cpu(), input_descriptor8_hex=self.raw(args[0], 8))
            if name == 'RtlEqualString':
                row['second_descriptor8_hex'] = self.raw(args[1], 8)
            self.rtl_calls.append(row)
            self.rtl_pending.append((self.u(stack), row))
            self.life_order += 1
            guest.reg_write(UC_X86_REG_EIP, int(self.rtl_metadata['entries'][name], 16))
            return
        if address == STRLEN_HELPER:
            stack = guest.reg_read(UC_X86_REG_ESP)
            pointer = self.u(stack + 4)
            raw = bytearray()
            for index in range(4096):
                value = bytes(guest.mem_read(pointer + index, 1))
                raw.extend(value)
                if value == b'\0':
                    break
            else:
                raise RuntimeError('Actual RTL source string exceeds bounded observation scope')
            actual = self.libc.strlen(bytes(raw))
            assert actual <= 0xffffffff
            self.strlen_calls.append(dict(order=self.life_order, compiled_caller=f'{self.u(stack):08x}', original_source_pointer=f'{pointer:08x}', actual_input_nul_bytes_hex=raw.hex(), actual_host_libc_result=actual, library_sha256=self.rtl_metadata['actual_host_strlen_library_sha256']))
            self.life_order += 1
            guest.reg_write(UC_X86_REG_EAX, actual)
            guest.reg_write(UC_X86_REG_ESP, stack + 4)
            guest.reg_write(UC_X86_REG_EIP, self.u(stack))
            return
        if self.life_on and self.native_clock is not None and address == 0x80000147:
            stack = guest.reg_read(UC_X86_REG_ESP)
            section = self.u(stack + 4)
            words = [self.u(section + index * 4) for index in range(9)]
            self.new_invalid = dict(boundary='Original XeLoadSection request: full primary FindSection/VM commit and section-reference ownership are not supplied.', instruction=f'{address:08x}', kernel_ordinal=327, original_return=f'{self.u(stack):08x}', section_header_pointer=f'{section:08x}', section_header38_hex=self.raw(section, 0x38), actual_section_virtual_address=f'{words[1]:08x}', actual_section_virtual_size=words[2], actual_section_raw_file_offset=words[3], actual_section_raw_file_size=words[4], actual_section_name_pointer=f'{words[5]:08x}', actual_section_reference_count=words[6], actual_head_reference_pointer=f'{words[7]:08x}', actual_tail_reference_pointer=f'{words[8]:08x}', head_reference4_hex=self.raw(words[7], 4), tail_reference4_hex=self.raw(words[8], 4), already_mapped_actual_section_sha256=hashlib.sha256(bytes(guest.mem_read(words[1], words[2]))).hexdigest(), raw_file_section_sha256=hashlib.sha256(self.image.data[words[3]:words[3] + words[4]]).hexdigest(), cpu=self.cpu(), original_result_supplied=False)
            raise KernelBoundary()
        if self.life_on and self.native_clock is not None and address in ENTRIES:
            stack = guest.reg_read(UC_X86_REG_ESP)
            self.dsp_calls.append(dict(order=self.life_order, entry=f'{address:08x}', caller=f'{self.u(stack):08x}',
                                       cpu=self.cpu(), stack12_words=[f'{self.u(stack+4+index*4):08x}' for index in range(12)],
                                       state=self.dsp_state(), scope='Literal current source-owned audio/device/section inputs; no admission or result supplied.'))
        super().hook(guest, address, size, user)

    def life_stage(self, name, entry):
        if entry != 0x12cbc0:
            return super().life_stage(name, entry)
        assert not self.configuration_inserted
        self.configuration_inserted = True
        before = self.configuration_state()
        first_call, first_store = len(self.life_calls), len(self.life_stores)
        ok = config.frame.AdmissionTrace.life_stage(self, 'original_configuration_default_handler_selected_startup_root', 0x663b0)
        self.configuration_records.append(dict(entry='000663b0', before=before, after=self.configuration_state(), first_call=first_call, final_call=len(self.life_calls), first_store=first_store, final_store=len(self.life_stores), complete_original_return=ok,
                                               scope='Existing original whole configuration owner selected before device prerequisite; complete upstream title constructor order remains explicit.'))
        if not ok:
            return False
        # Exact authored argument at original126C8B immediately before126C90.
        assert self.image.read(0x126c8b, 5).hex() == '6820124500'
        self.dsp_root_argument = 0x451220
        before = self.dsp_state()
        first_call, first_store, first_lock = len(self.life_calls), len(self.life_stores), len(self.lock_calls)
        ok = config.frame.AdmissionTrace.life_stage(self, 'original_audio_driver_selected_prerequisite_root', 0x2205c0)
        self.life_stages[-1]['arguments'] = ['00451220']
        self.dsp_root_argument = None
        self.dsp_records.append(dict(entry='002205c0', authored_argument='00451220', authored_argument_original_instruction='00126c8b',
                                     before=before, after=self.dsp_state(), first_call=first_call, final_call=len(self.life_calls),
                                     first_store=first_store, final_store=len(self.life_stores), first_lock=first_lock, final_lock=len(self.lock_calls),
                                     complete_original_return=ok,
                                     scope='Explicit original driver prerequisite root with unchanged authored policy; natural audio controller/whole title scheduling remain unproved. No successful device or DSP record supplied.'))
        if not ok:
            # Preserve the failed independent driver. Inspect the whole owning
            # title constructor separately, without skipping its disk/OS calls.
            before = self.dsp_state()
            first_call, first_store = len(self.life_calls), len(self.life_stores)
            startup_ok = config.frame.AdmissionTrace.life_stage(self, 'original_full_title_constructor_selected_prerequisite_root', 0x12000)
            self.dsp_records.append(dict(entry='00012000', before=before, after=self.dsp_state(), first_call=first_call, final_call=len(self.life_calls), first_store=first_store, final_store=len(self.life_stores), complete_original_return=startup_ok, scope='Whole original owning constructor is attempted. No omitted disk/VirtualAlloc operation is given a result; no audio arena is assigned. This independent root does not repair the earlier failed driver.'))
            # Exact source192D39 section-name input. This pure section-loader
            # root is independent of the unavailable natural device caller.
            assert self.image.read(0x192d39, 5).hex() == '68a0ae4500'
            self.section_root_argument = 0x45aea0
            before = self.dsp_state()
            first_call, first_store = len(self.life_calls), len(self.life_stores)
            section_ok = config.frame.AdmissionTrace.life_stage(self, 'original_DSPImage_section_loader_selected_source_root', 0x2dbda1)
            self.life_stages[-1]['arguments'] = ['0045aea0']
            self.section_root_argument = None
            self.dsp_records.append(dict(entry='002dbda1', authored_argument='0045aea0', authored_argument_original_instruction='00192d39', before=before, after=self.dsp_state(), first_call=first_call, final_call=len(self.life_calls), first_store=first_store, final_store=len(self.life_stores), complete_original_return=section_ok, scope='Independent exact192D30-owned section-loader request, not whole192D30 or natural DirectSound admission. Original lookup/header order executes; no section/device/HRESULT success supplied.'))
            return False
        # Only an actual complete source result can reach the later frame.
        if not self.g.reg_read(config.frame.UC_X86_REG_EAX) & 0xff:
            return False
        return config.frame.AdmissionTrace.life_stage(self, name, entry)


def main():
    pin = sha(__file__)
    active = []
    original_save = config.save

    class CapturedTrace(DspTrace):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, **kwargs)
            active.append(self)

    def capture(path, contents):
        if path.name == 'report.json':
            report = json.loads(contents)
            trace = active[0]
            report.update(schema='source-smg-effect-dsp-owner-v1', oracle_sha256=pin,
                          underlying_configuration_oracle_sha256=sha(config.__file__),
                          dsp_records=trace.dsp_records, dsp_calls=trace.dsp_calls, final_dsp_state=trace.dsp_state(),
                          verified_compatibility_lock_calls=trace.lock_calls, verified_compatibility_lock_stores=trace.lock_stores,
                          verified_compatibility_lock_metadata=trace.locks, retained_compatibility_context=trace.platform_input,
                          verified_compatibility_rtl=trace.rtl_metadata, rtl_calls=trace.rtl_calls, actual_host_strlen_calls=trace.strlen_calls,
                          compiled_rtl_executed_addresses=[f'{address:08x}' for address in sorted(trace.rtl_executed)])
            report['boundaries'].append('Original driver prerequisite uses actual126C8B authored argument before any natural192D30. No DirectSound device, DSP record, section refcount, HRESULT, gameplay flags or elapsed value is supplied.')
            contents = encoded(report)
        original_save(path, contents)

    config.OUT, config.PRIOR, config.ConfigurationTrace, config.save = OUT, PRIOR, CapturedTrace, capture
    config.main()
    assert sha(__file__) == pin


if __name__ == '__main__':
    main()
