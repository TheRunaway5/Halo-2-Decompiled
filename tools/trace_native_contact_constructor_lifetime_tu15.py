#!/usr/bin/env python3
"""Observe constructor-word reads/writes and heap copying in one original run.

Offline diagnostics only. Hardware breakpoints do not change original code,
operands, memory or outcomes. Use run_test.py for the inherited owned lifecycle.
"""
import argparse
import json
from pathlib import Path
import sys
import trace_native_physics_heap_writer_tu15 as parent


def dual_observer(source, constructor_word):
    source = parent.replace_once(source,
        ' J watched_writes=J::array(),watched_copies=J::array();std::size_t watched_write_count=0,copy_entry_count=0;',
        ''' const unsigned source_index=copy_index+1;require(source_index<4,"No hardware constructor-word slot");
 const unsigned long constructor_word=CONSTRUCTOR_WORD;
 const auto constructor_initial_word=static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(constructor_word)))&0xfffffffful;
 debug(source_index,constructor_word);
 enabled|=(1ul<<(2*source_index))|(3ul<<(16+4*source_index))|(3ul<<(18+4*source_index));
 debug(6,0);debug(7,enabled);
 J constructor_events=J::array();std::size_t observation_ordinal=0,constructor_event_count=0;
 J watched_writes=J::array(),watched_copies=J::array();std::size_t watched_write_count=0,copy_entry_count=0;'''.replace('CONSTRUCTOR_WORD', hex(constructor_word)+'ul'))
    source = parent.replace_once(source,
        '   if(dr6&(1ul<<copy_index)) {',
        '''   ++observation_ordinal;
   if(dr6&(1ul<<source_index)) {
    ++constructor_event_count;
    if(constructor_events.size()<256)constructor_events.push_back(J{
     {"ordinal",observation_ordinal},{"after_instruction",regs.rip},{"address",constructor_word},
     {"word_after",static_cast<unsigned long>(request(PTRACE_PEEKDATA,child,reinterpret_cast<void *>(constructor_word)))&0xfffffffful},
     {"gp",J::array({regs.rax,regs.rcx,regs.rdx,regs.rbx,regs.rsp,regs.rbp,regs.rsi,regs.rdi})}});
    debug(6,0);if(!returns.contains(regs.rip))continue;
   }
   if(dr6&(1ul<<copy_index)) {''')
    source = parent.replace_once(source,
        'watched_copies.push_back(J{{"entry",regs.rip},{"caller_return",arguments[0]},',
        'watched_copies.push_back(J{{"ordinal",observation_ordinal},{"entry",regs.rip},{"caller_return",arguments[0]},')
    source = parent.replace_once(source,
        '     {"after_instruction",regs.rip},{"address",watch_address},',
        '     {"ordinal",observation_ordinal},{"after_instruction",regs.rip},{"address",watch_address},')
    source = parent.replace_once(source,
        '{"watched_address",watch_address},{"watched_write_count",watched_write_count}',
        '{"constructor_word_address",constructor_word},{"constructor_initial_word",constructor_initial_word},'
        '{"constructor_event_count",constructor_event_count},{"constructor_events",constructor_events},'
        '{"watched_address",watch_address},{"watched_write_count",watched_write_count}')
    return source


def main():
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument('--constructor-word', type=lambda value: int(value, 0), required=True)
    own, remaining = parser.parse_known_args()
    assert 0 <= own.constructor_word <= 0xfffffffc and own.constructor_word % 4 == 0
    output = Path(remaining[remaining.index('--output')+1])
    original_observer = parent.watched_tracer
    parent.watched_tracer = lambda source, address: dual_observer(original_observer(source, address), own.constructor_word)
    sys.argv = [sys.argv[0], *remaining]
    result = parent.main()
    trace = json.loads((output/'writer-trace.json').read_text())
    report_path = output/'report.json'
    report = json.loads(report_path.read_text())
    report.update(constructor_word_address=hex(own.constructor_word),
                  constructor_initial_word=hex(trace['constructor_initial_word']),
                  constructor_event_count=trace['constructor_event_count'],
                  constructor_events_truncated=trace['constructor_event_count']>len(trace['constructor_events']),
                  dual_observer_script_sha256=parent.sha(Path(__file__)),
                  scope='Single unchanged original execution: constructor-word read/write, heap-write and copy-entry '
                        'chronology. No field-lifetime waiver, semantic-source attribution or native parity claim.')
    report_path.write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps({key: report[key] for key in ('constructor_word_address', 'constructor_initial_word', 'constructor_event_count')}))
    return result


if __name__ == '__main__':
    raise SystemExit(main())
