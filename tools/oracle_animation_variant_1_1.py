#!/usr/bin/env python3
"""Observe genuine animation variant/resource/RNG owners without new seed writes.

Reproduce the frozen constructor/selection schedule, then invoke explicit pure
variant roots with actual authored indices. No selected output, graph bytes,
seed, successful resource return, readiness or service is supplied.
"""
import argparse
import json
import os
import struct
import subprocess
from pathlib import Path
import unicorn
from unicorn.x86_const import UC_X86_REG_ESP, UC_X86_REG_EIP, UC_X86_REG_EAX, UC_X86_REG_EDX, UC_X86_REG_ESI, UC_X86_REG_EDI
import oracle_mp_animation_selection_1_1 as owning
from functions import _trace

ROOT = owning.ROOT
OUT = ROOT/'analysis/multiplayer/integration/animation-variant'
ENTRIES = (0x1ceda0, 0x1e1020, 0x1e0fc0, 0x1de890, 0x1de9b0, 0x1e13c0, 0x124e40)
PINNED = {
    'tools/oracle_mp_animation_selection_1_1.py': owning.sha(owning.__file__),
    'analysis/multiplayer/integration/spawn-source/animation-selection/manifest.json': owning.sha(ROOT/'analysis/multiplayer/integration/spawn-source/animation-selection/manifest.json'),
    'analysis/multiplayer/integration/spawn-source/animation-selection/measured/report.json': owning.sha(ROOT/'analysis/multiplayer/integration/spawn-source/animation-selection/measured/report.json'),
}


class VariantObserver:
    def __init__(self, *args, **kwargs):
        self.variant_calls = []
        self.variant_pending = []
        self.variant_stores = []
        self.variant_reads = []
        self.variant_order = 0
        self.variant_executed = set()
        self.variant_graphs = {}
        self.variant_kind = 'unchanged_owning_selection_schedule'
        super().__init__(*args, **kwargs)
        self.variant_routines = {entry: _trace(self.code, entry, self.starts|set(ENTRIES)) for entry in ENTRIES}
        self.variant_owners = {address: entry for entry, routine in self.variant_routines.items() for address in routine.code}
        self.g.hook_add(unicorn.UC_HOOK_MEM_READ, self.variant_read)

    def raw(self, pointer, size):
        return bytes(self.g.mem_read(pointer, size)).hex()

    def rng(self):
        pointer = self.u(0x4ee9d8)
        return dict(pointer=f'{pointer:08x}', raw8_hex=self.raw(pointer, 8) if pointer else None)

    def observe_variant_graph(self, address):
        if not address or address in self.variant_graphs:
            return
        tag = next((tag for tag in self.cache.tags.values() if tag['group']=='jmad' and tag['offset']+self.cache.magic==address), None)
        assert tag is not None, ('Original graph address is not owned by the cache', hex(address))
        datum = next(datum for datum, row in self.cache.tags.items() if row is tag)
        regions = [dict(address=f'{address:08x}', bytes=0x54, raw_hex=self.raw(address, 0x54))]
        for at, stride in ((0x2c, 0x6c), (0x4c, 0x20)):
            count = struct.unpack('<i', bytes(self.g.mem_read(address+at, 4)))[0]
            pointer = self.u(address+at+4)
            assert 0 <= count < 10000
            if count:
                regions.append(dict(address=f'{pointer:08x}', bytes=count*stride, raw_hex=self.raw(pointer, count*stride)))
        for region in regions:
            pointer = int(region['address'], 16)
            assert bytes(self.cache.data[pointer-self.cache.magic:pointer-self.cache.magic+region['bytes']]).hex() == region['raw_hex']
        self.variant_graphs[address] = dict(datum=f'{datum:08x}', address=f'{address:08x}', name=tag['name'], regions=regions)

    def variant_finish(self, address):
        sp = self.g.reg_read(UC_X86_REG_ESP)
        for pending in reversed(self.variant_pending[:]):
            ret, before_sp, row = pending
            if ret != address or sp <= before_sp:
                continue
            row.update(return_eax=f'{self.g.reg_read(UC_X86_REG_EAX):08x}',
                       return_esp_delta=sp-before_sp, return_order=self.variant_order,
                       rng_after=self.rng())
            if row.get('output_pointer') is not None:
                row['output_after_hex'] = self.raw(row['output_pointer'], 4)
            if row.get('manager') is not None:
                row['manager_after_hex'] = self.raw(row['manager'], 0x90)
            if row['entry'] == '001de890':
                pointer = self.g.reg_read(UC_X86_REG_EAX)
                row['definition_return'] = dict(pointer=f'{pointer:08x}', raw6c_hex=self.raw(pointer, 0x6c) if pointer else None)
            self.variant_order += 1
            self.variant_pending.remove(pending)

    def hook(self, g, address, size, user):
        if (getattr(self, 'selection_enabled', False) or getattr(self, 'clock_enabled', False)) and hasattr(self, 'variant_owners'):
            self.variant_finish(address)
            if address in self.variant_owners:
                self.variant_executed.add(address)
            if address in ENTRIES and (address in (0x1ceda0, 0x1e1020) or self.variant_pending):
                sp = g.reg_read(UC_X86_REG_ESP)
                registers = {key: int(value, 16) for key, value in self.registers().items()}
                stack = [self.u(sp+4+index*4) for index in range(8)]
                row = dict(id=len(self.variant_calls), entry=f'{address:08x}', caller=f'{self.u(sp):08x}',
                    phase=self.active, kind=self.variant_kind, order=self.variant_order, registers=registers,
                    stack=[f'{value:08x}' for value in stack], rng_before=self.rng(),
                    ancestors=[pending[2]['id'] for pending in self.variant_pending])
                if address == 0x1ceda0:
                    row.update(manager=registers['edi'], manager_before_hex=self.raw(registers['edi'], 0x90),
                               output_pointer=registers['esi'])
                    graph_datum=self.u(registers['edi']+0x68)
                    graph_address=self.u(self.u(0x4eb114)+(graph_datum&65535)*16+8)
                    row['graph_datum']=f'{graph_datum:08x}'
                    self.observe_variant_graph(graph_address)
                elif address == 0x1e1020:
                    row.update(graph_address=f'{stack[0]:08x}', output_pointer=stack[1])
                    self.observe_variant_graph(stack[0])
                elif address in (0x1de890, 0x1de9b0, 0x1e13c0):
                    row['graph_address']=f'{registers["edx"]:08x}'
                    self.observe_variant_graph(registers['edx'])
                elif address == 0x1e0fc0:
                    row.update(graph_address=f'{registers["ebx"]:08x}', output_pointer=registers['edi'])
                    self.observe_variant_graph(registers['ebx'])
                if row.get('output_pointer') is not None:
                    row['output_before_hex']=self.raw(row['output_pointer'], 4)
                self.variant_calls.append(row)
                self.variant_pending.append((self.u(sp), sp, row))
                self.variant_order += 1
        super().hook(g, address, size, user)

    def mem_write(self, g, access, pointer, size, value, user):
        if self.variant_pending:
            address = g.reg_read(UC_X86_REG_EIP)
            if address in self.variant_owners:
                self.variant_stores.append(dict(order=self.variant_order, instruction=f'{address:08x}',
                    address=f'{pointer:08x}', size=size, before_hex=self.raw(pointer, size),
                    written_hex=(value&((1<<(size*8))-1)).to_bytes(size, 'little').hex(),
                    calls=[pending[2]['id'] for pending in self.variant_pending]))
                self.variant_order += 1
        super().mem_write(g, access, pointer, size, value, user)

    def variant_read(self, g, access, pointer, size, value, user):
        address = g.reg_read(UC_X86_REG_EIP)
        if self.variant_pending and address in self.variant_owners:
            self.variant_reads.append(dict(order=self.variant_order, instruction=f'{address:08x}',
                address=f'{pointer:08x}', size=size, raw_hex=self.raw(pointer, size),
                calls=[pending[2]['id'] for pending in self.variant_pending]))
            self.variant_order += 1


class VariantTrace(VariantObserver, owning.SelectionTrace):
    pass


def execute(context):
    owning.SelectionTrace = VariantTrace  # New observer only, inherited guest preparation unchanged.
    trace = owning.execute(context)
    trace.variant_finish(owning.ctor.STOP)
    assert not trace.variant_pending
    frozen = json.loads((ROOT/'analysis/multiplayer/integration/spawn-source/animation-selection/measured/report.json').read_text())
    assert json.loads(owning.encoded(trace.selection_calls)) == frozen['calls']
    assert json.loads(owning.encoded(trace.selection_stores)) == frozen['stores']
    owning_counts = dict(calls=len(trace.variant_calls), stores=len(trace.variant_stores), reads=len(trace.variant_reads))
    # Explicit pure caller roots choose authored input indices, never selected
    # outputs or new RNG state. Both stream selectors are declared ABI inputs.
    roots=[]
    for datum in (0xe9180793, 0xe5aa0435, 0xe2d4015f):
        tag=trace.cache.tags[datum];graph=tag['offset']+trace.cache.magic
        trace.observe_variant_graph(graph)
        count=trace.u(graph+0x2c);bank=trace.u(graph+0x30)
        for index in range(count):
            first=struct.unpack('<h',bytes(trace.g.mem_read(bank+index*0x6c+0x30,2)))[0]
            next_=struct.unpack('<h',bytes(trace.g.mem_read(bank+index*0x6c+0x32,2)))[0]
            if first == -1 and next_ == -1:
                continue
            for stream in (0,1):
                packed=(index<<16)|65535
                trace.variant_kind='declared_pure_authored_variant_input'
                trace.active=f'pure_variant_{datum:08x}_{index}_{stream}'
                root=dict(graph=f'{datum:08x}', graph_address=f'{graph:08x}', authored_input_index=index,
                          packed=f'{packed:08x}', stream_selector=stream, call_index=len(trace.variant_calls), rng_before=trace.rng())
                try:
                    trace.run(0x1e1020,(graph,0x2000100,packed,stream))
                    trace.variant_finish(owning.ctor.STOP)
                    assert not trace.variant_pending
                    root['completion']='complete_original_return'
                except unicorn.UcError as error:
                    root.update(completion='blocked_original_instruction', error=str(error),
                                instruction=f'{trace.g.reg_read(UC_X86_REG_EIP):08x}', invalid=trace.new_invalid)
                root['rng_after']=trace.rng();roots.append(root)
                if root['completion']!='complete_original_return':
                    return trace, owning_counts, roots
    return trace, owning_counts, roots


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();args.output=args.output.resolve();assert not (args.output/'report.json').exists()
    for name,pin in PINNED.items():assert owning.sha(ROOT/name)==pin,name
    args.output.mkdir(parents=True,exist_ok=True)
    with (args.output/'host-provider.stderr.log').open('w') as errors:
        process=subprocess.Popen(['wine',str(owning.ctor.owning.CONTEXT_OUT/'generator.exe')],
            stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=errors,text=True,
            env=dict(os.environ,WINEDEBUG='-all',WINEPREFIX=str(owning.ctor.owning.CONTEXT_OUT/'wine-prefix')))
        try:
            context=json.loads(process.stdout.readline());trace,counts,roots=execute(context)
            listings=[]
            for entry,routine in trace.variant_routines.items():
                instructions=[trace.code.at(address) for address in sorted(routine.code)]
                for instruction in instructions:
                    assert trace.image.read(instruction.address,len(instruction.bytes))==instruction.bytes
                    assert bytes(trace.g.mem_read(instruction.address,len(instruction.bytes)))==instruction.bytes
                raw=''.join(f'{i.address:08x} {i.bytes.hex()} {i.mnemonic} {i.op_str}\n' for i in instructions).encode()
                path=args.output/'instructions'/f'{entry:08x}.asm';owning.save(path,raw)
                listings.append(dict(entry=f'{entry:08x}',file=str(path.relative_to(args.output)),sha256=owning.sha(path),
                    rows=len(instructions),executed_rows=sum(i.address in trace.variant_executed for i in instructions)))
            report=dict(schema='original-animation-variant-v1',tool_sha256=owning.sha(__file__),source_pins=PINNED,
                xbe_sha256=owning.ctor.PIN,cache_sha256=owning.ctor.CACHE_PIN,
                exact_owning_selection_correlation=True,owning_ranges=counts,declared_pure_roots=roots,
                graphs=list(trace.variant_graphs.values()),calls=trace.variant_calls,stores=trace.variant_stores,reads=trace.variant_reads,
                constants={f'{address:08x}':f'{trace.u(address):08x}' for address in (0x465030,0x4650c0,0x465028)},
                listings=listings,inherited_services=trace.events,new_guest_seed_or_graph_fields=[],
                new_selected_variant_or_resource_results=[],instruction_patches=[],
                boundaries=['Exact frozen original join/loadout/constructor/selection schedule first; existing bounded RNG and resource delivery inputs remain explicit and are not initialized or changed by this observer.',
                    'Pure inputs use original authored linked animation indices and declared stream selector0/1; selected outputs and ordered RNG stores are unchanged original instructions, never prechosen.',
                    'Original1DE890/1E0FC0/1E13C0 and124E40 execute whenever called. No successful resource result, readiness, callback or source seed is supplied. Full ordinary game RNG initialization and live resource integration remain separate owners.'])
            owning.save(args.output/'report.json',owning.encoded(report))
            print(json.dumps(dict(report_sha256=owning.sha(args.output/'report.json'),calls=len(trace.variant_calls),
                stores=len(trace.variant_stores),pure_roots=len(roots),completions=sorted(set(r['completion'] for r in roots)))),flush=True)
            for name,pin in PINNED.items():assert owning.sha(ROOT/name)==pin,name
        finally:
            if process.poll() is None:
                process.stdin.write('x\n');process.stdin.flush()
                try:process.wait(timeout=15)
                except subprocess.TimeoutExpired:process.kill();process.wait(timeout=5)
            process.stdin.close();process.stdout.close()


if __name__=='__main__':main()
