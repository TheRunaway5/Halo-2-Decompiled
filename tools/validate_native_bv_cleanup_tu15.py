#!/usr/bin/env python3
"""Physical TU1.5 comparison of BV cleanup and inherited cleanup regressions.

Uses unchanged original instructions and the existing real-pool cleanup
harness. Declared callback input changes exercise current child/table reads;
neither callback outcomes nor engine state postimages are supplied.
"""
import inspect
import validate_native_agent_cleanup_children_tu15 as parent


def cases():
    rows = parent.cases()
    address = parent.base.UI
    template = next(row for row in rows if row['name'] == 'wrapped_leaf')

    def add(name, secondary=False, **extra):
        row = dict(template, name='bv_'+name, entry=0x2f0da0,
                   user_words=[*template['user_words'],
                               (0x110, address+0x400 if secondary else 0),
                               (0x400, address+0x820), (0x500, address+0x820)],
                   **extra)
        rows.append(row)

    add('primary_only')
    add('two_children', True)
    for name, value in [('secondary_current', address+0x500), ('secondary_removed', 0)]:
        add(name, True, mutations=[dict(instruction=0x31d400, address=address+0x110,
                                       written=value.to_bytes(4, 'little').hex())])
    add('destructor_current', True, mutations=[dict(instruction=0x31d400,
        address=address+0x100, written=(address+0x900).to_bytes(4, 'little').hex())])
    for name, offset, write in [('primary', 0x10c, False), ('primary_table', 0x300, False),
                                ('secondary', 0x110, False), ('secondary_table', 0x400, False),
                                ('clear_after_callback', 0x110, True), ('self_table', 0x100, False)]:
        add('fault_'+name, True, fault=dict(address=address+offset, bytes=4,
                                           occurrence=1, write=write))
    return rows


def main(*, additional_cases=(), additional_owners=(), additional_pins=(), source_transform=None):
    source = inspect.getsource(parent.main)
    old = 'else if(entry==0x2f6c80u)native_tu15_cleanup_wrapped_contact_agent(memory,{receiver},fp,services);'
    new = old+'\n        else if(entry==0x2f0da0u)native_tu15_cleanup_bv_contact_agent(memory,{receiver},fp,services,observe);'
    assert source.count(old) == 1
    source = source.replace(old, new)
    old = "  for name,before,after in mutants:"
    new = '''  mutants += [
   ('bv_secondary_skipped','if(const auto secondary=m.word(agent.identity+16u))','if(const auto secondary=0u)'),
   ('bv_clear_omitted','m.word(0x2f0db7u,agent.identity+16u,0);',''),
   ('bv_stale_secondary','cleanup(m.word(agent.identity+12u));\\n    if(const auto secondary=m.word(agent.identity+16u))',
     'const auto secondary=m.word(agent.identity+16u);\\n    cleanup(m.word(agent.identity+12u));\\n    if(secondary)')]
  for name,before,after in mutants:'''
    assert source.count(old) == 1
    source = source.replace(old, new)
    # The inherited project pins every source it reads and freezes it before
    # compiling. Include this adapter in that same end-of-run pin check.
    old = "'tools/validate_native_agent_cleanup_children_tu15.py',"
    assert source.count(old) == 1
    source = source.replace(old, old+"'tools/validate_native_bv_cleanup_tu15.py',")
    for pin in additional_pins:
        source=source.replace(old, old+repr(pin)+',')
    if source_transform:
        source=source_transform(source)
    environment = dict(parent.__dict__, cases=lambda:[*cases(),*additional_cases],
                       OWNERS=(*parent.OWNERS, 0x2f0da0, *additional_owners),
                       __doc__=__doc__)
    exec(source, environment)
    return environment['main']()


if __name__ == '__main__':
    raise SystemExit(main())
