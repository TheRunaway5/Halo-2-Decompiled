"""Deliver an actual captured UTC input at the original kernel import.

The imported operating-system clock is external input. Original game/CRT/RNG
instructions execute; no gameplay return, seed, damage or physical state is
provided by this bridge. Unsupported external clocks remain explicit failures.
"""
import hashlib
import json
from capstone import Cs, CS_ARCH_X86, CS_MODE_32
from capstone.x86 import X86_OP_MEM


class OriginalRecordedPlatformTimeTu15:
    def __init__(self, capture, image):
        path = capture/'external-inputs.json'
        transcript = json.loads(path.read_text())
        assert transcript['supplied_gameplay_results'] is False
        inputs = transcript['actual_external_inputs']
        assert all(row['kind'] in (2, 3) for row in inputs), 'Unsupported clock domain in full-call replay'
        self.values = [row['value'] for row in inputs if row['kind'] == 3]
        assert self.values and all(row['operand'] == 0 for row in inputs if row['kind'] == 3)
        decoder = Cs(CS_ARCH_X86, CS_MODE_32)
        decoder.detail = True
        instruction = next(decoder.disasm(image.read(0x33a4d3, 6), 0x33a4d3))
        assert instruction.mnemonic == 'jmp' and instruction.operands[0].type == X86_OP_MEM
        operand = instruction.operands[0].mem
        assert not operand.base and not operand.index
        self.address = operand.disp
        assert int.from_bytes(image.read(self.address, 4), 'little') == 0x80000080
        self.linked = None
        self.receipt = dict(input_transcript_sha256=hashlib.file_digest(path.open('rb'), 'sha256').hexdigest(),
                            original_import_instruction=instruction.bytes.hex(),
                            import_address=self.address, original_ordinal=0x80000080,
                            recorded_utc_inputs=self.values, supplied_gameplay_or_rng_results=[])

    def assembly(self):
        values = '\n'.join(f' .quad {value}' for value in self.values)
        return f'''
.balign 16
.globl reference_query_system_time
reference_query_system_time:
 mov %eax,reference_saved_eax
 mov %ecx,reference_saved_ecx
 mov %edx,reference_saved_edx
 mov 4(%esp),%edx
 mov %esp,reference_saved_esp
 mov $reference_kernel_stack_end,%esp
 pushfl
 mov reference_time_index,%ecx
 cmp ${len(self.values)},%ecx
 jae reference_time_exhausted
 mov reference_times(,%ecx,8),%eax
 mov %eax,(%edx)
 mov reference_times+4(,%ecx,8),%eax
 mov %eax,4(%edx)
 lea 1(%ecx),%ecx
 mov %ecx,reference_time_index
 popfl
 mov reference_saved_esp,%esp
 mov reference_saved_eax,%eax
 mov reference_saved_ecx,%ecx
 mov reference_saved_edx,%edx
 ret $4
reference_time_exhausted:
 mov $1,%eax
 mov $126,%ebx
 int $0x80
reference_saved_eax: .long 0
reference_saved_ecx: .long 0
reference_saved_edx: .long 0
reference_saved_esp: .long 0
reference_time_index: .long 0
.balign 8
reference_times:
{values}
.balign 16
reference_kernel_stack: .space 4096
reference_kernel_stack_end:
'''

    def bind(self, symbols):
        self.linked = symbols['reference_query_system_time']
        self.receipt['linked_import'] = self.linked
        return dict(writes=[dict(address=self.address, before=0x80000080, linked=self.linked)])

    def link(self, address, raw):
        if not (address <= self.address and self.address+4 <= address+len(raw)):
            return raw
        offset = self.address-address
        assert int.from_bytes(raw[offset:offset+4], 'little') == 0x80000080
        return raw[:offset]+self.linked.to_bytes(4, 'little')+raw[offset+4:]
