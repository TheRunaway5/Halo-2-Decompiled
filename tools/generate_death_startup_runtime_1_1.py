#!/usr/bin/env python3
"""Compile complete original retained death/object startup and async owners.

Original physical-bank, game-arena, object header/body-heap and type startup
bodies share their real retained globals. Async queue initialization, insertion,
worker, file-open and file-extent callbacks remain complete original bodies. Resource,
thread, synchronization, formatting and I/O providers own their actual ABI;
this generator neither supplies completion bytes nor manufactures readiness.
"""
import generate_unit_animation_selection_runtime_1_1 as generator
import hashlib,json
from pathlib import Path

generator.ENTRIES=(
    0xb76b0,0xc0e50,0xc48f0,0xd5e30,0x10ca70,0x1174d0,0x1181d0,
    0x10a5f0,0x11e0f0,0x121fd0,0x122020,0x1220e0,0x122150,0x1221b0,0x122350,0x122360,
    0x125280,0x125520,0x125e50,0x12cb70,0x1656a0,0x165700,
    0x16cff0,0x16d070,0x16d0d0,0x18fc80,0x1a4250,0x1a4c20,0x1a4c70,0x1a5080,
    0x1ce5b0,0x217020,0x217090,0x29c520,0x2d79aa,0x2d7a1a,
)
generator.ENTRIES=tuple(sorted(set(generator.ENTRIES)|{0xb78c0,0xb7aa0,0xc0400,0xc0bb0,0xc0c10,0xc0ef0,0xc0f50,0xd5e90,0x10caf0,0x11d460,0x11e150,0x1482a0,0x1c5f00,0x1c5fd0,0x1d2620,0x1e1800,0x1e1890,0x1e1c10,0x1e1cb0,0x1eef40,0x1eefe0,0x1f15b0,0x2284c0,0x33785c,0x117510,0x118210,0xc4930}))
generator.RUNTIME_HEADER='halo2/death_startup_runtime.hpp'
generator.ENTRIES=tuple(sorted(set(generator.ENTRIES)|{0x10a670,0x10bbc0}))
generator.ENTRIES=tuple(sorted(set(generator.ENTRIES)|{
    # Original render-cache lifecycle, reached from the live source world.
    # Allocation remains125E50 via the actual arena allocator method table;
    # activation joins the already compiled16D220/326970 reset owners.
    0x3d260,0x2b520,
    0x124290,0x1c5cf0,0x32cffc,0x32ebdf,0x3274e4,0x327544,
    0x32750c,0x327464,0x32d120,0x32d080,0x32ebcf,0x37d8c0,
    0x32a866,0x32a86f,0x32a948,0x32a779,0x32d06e,0x2da2bc,0x32753e,
    0x147cc0,0x147d20,0x14cc00,0x14ccd0,0x10d490,0x10d4f0,
    0x1efe10,0x1efe70,0x1cb9d0,
    0x133040,0x133170,0xaa690,0xaa720,
    # Complete reached birth/registration owners from the independently
    # executed actual-cache live-body-world reference; no result callbacks.
    0x2e1c0,0xb81f0,0xb9780,0xbf580,0xbfb90,0xc10e0,0xcdc60,0xcdf00,
    0xd5f00,0x10cd10,0x10d520,0x178120,0x1cb990,0x1d2aa0,0x1d3340,
    0x1da6f0,0x1de920,0x3141d0,
    0x177da0,0x177a10,0x20e30,0xb9430,0x1c7200,
    # Complete effect storage and the actual attachment creation/visibility
    # graph. Optional attachments are not replaced with supplied failure.
    0x1775d0,0x175860,0x177690,0x17cbd0,0x1781f0,
    0x179b90,0x179ad0,0x179d10,0x179cb0,0x17a540,0x167d32,0x167d71,
    0x167daf,0x179dd0,0xb9c20,0x1dc8f0,0x1dc940,0x17a030,
    0x178d80,0x17b2c0,0x17af50,0x17b1a0,0x17b280,0x17ad20,0x11d620,
    0x11d650,0x3eb10,0x179080,0x178f60,0x178e10,0x17bdf0,
    0x17c840,0x1788b0,0x18aa90,0x18ad70,
    0x158ba0,0x164720,0x16d680,0x16807e,0x156eb0,0x1441d0,0x1dcaa0,
    0x1677c3,0x1677e5,0x125490,0x1254d0,0x1ce9b0,0x1c9d30,
    0x17d1c0,
    # Direct source call closure of the retained effect graph. Each address
    # comes from an actual call operand, including unindexed original bodies.
    0x177500,0x1779c0,0x177c80,0x178c10,0x178cd0,0x178ed0,0x179000,
    0x179690,0x1796c0,0x179780,0x1799d0,0x179a90,0x17a160,0x17a460,
    0x17a630,0x17a6f0,0x17a920,0x17aa90,0x17ac00,0x17af10,0x17ba20,
    0x17c310,0x17c8c0,0x17c930,0x17c9a0,0x17cce0,0x17d040,
    0x16d560,
    0xc1b30,0xc19b0,0xc40d0,0x133390,0x1334d0,0x17e480,
    0xbac10,0xbbc40,0xb9b30,0xbb1f0,0xc16f0,0x13d300,0x167f00,
    0x10cf20,0xbdb10,0xc7ef0,0xdf9e0,
    0xbd860,0xbd960,0xbd9c0,0xbda00,0xbda50,0xbdaa0,0xc0e10,
    0xc2790,0x159ef0,0xd28c0,0x1037a0,
    0xbf650,0xb9dd0,0xba1c0,0xba2a0,
    0xc56d0,0xe7f30,0x1eb150,
    0x14d800,0x14d600,0x14d3d0,0x328274,0x14e240,0xcd3b0,0x153a90,
    0x159730,0x159970,0x159a70,0x14d730,0x1508b0,0x150950,
    0x3f4f0,0x170060,
    # Original visibility notification and its complete BSP decorator owner.
    0x17f050,0x23cbd0,
    0xb7ce0,0x1c8970,0x1c88f0,0x10cec0,
    0x1c71b0,
    0x320d20,0x320d50,
    0x3213e0,
    0x115560,
    # Remaining direct unit-update callees, with their small inventory closure.
    0xc9920,0xcd310,0x1045e0,0x1154e0,0x115520,0x1155b0,
    0x18ac40,0x1060d0,0x10a350,0x32909c,0x18a800,
    0xc5cd0,0xc5c50,0xb9a60,0x1ce7e0,0x29c590,
    # Model-resource storage, activation and selected physical reservation.
    # The stage comes from1908E0 over the actual native session options;
    # the other190930 callbacks remain separate, unclaimed startup owners.
    0x12f020,0x12f160,0x12f260,0x13e8e0,0x13f020,0x1908e0,
    # Full model/header admission. Queued headers retain their original
    # nonresident result until the original IO/fixup graph completes.
    0x1703d0,0x12f420,
    # Complete original model-resource read/admission/fixup graph. Native
    # files supply real bytes;122360 alone publishes async completion.
    0x12f5e0,0x12fb10,0x13eae0,0x13efa0,0x13ea90,0x19d8c0,
    0x1a4870,0x1a47d0,0x122400,0x1274b0,0x225d80,
    0x12f980,0x12fa00,0xa1450,0x225e90,0x225f10,0x337a40,
    # Complete original CRT-thread/RNG construction and delivered-options
    # seed writer. Native TLS and clocks remain named external input owners.
    # Authenticated full bodies: fatal-rng-research/HANDOFF.md and its paired
    # inventories, including the literal1A0EC0 nine-way variant dispatch.
    0x1479b0,0x147a20,0x32803e,0x328505,0x32c609,0x32e813,
    0x2d8674,0x336983,0x327290,0x139540,0x1a0ec0,0x32b205,
    0x32e7f1,0x32e7c5,0x3293b8,0x2d7969,0x2dac5e,0x32c7a6,
    0x2d84e9,0x32eeec,0x32d045,0x2d83be,0x32c754,0x32b221,
    # Complete live model-node update reached by the native world builder.
    # Authenticated268-row body: extra-owners/000be500-tu11.asm.
    0xbe500,
}))
generator.RUNTIME_FUNCTION='source_run_death_startup_bodies_1_1'
generator.EXTRA_TABLES={0x133270:6,0xbfd38:4,0x17c2f8:4,
                        0x179ac0:4,0x17a908:4,0xbdafc:4,0x159fbc:4,
                        0x1701c8:9,0x12f220:6,0x19d94c:4,0x1a16c4:9}
# Literal complete-body inventory recovered from the original3DAF0 lighting
# graph. Withheld329168 and platform/callback boundaries remain required.
lighting_report=Path(__file__).resolve().parents[1]/'analysis/damage-shields-goal-20261012/shield-live-research/lighting-closure/report.json'
assert hashlib.sha256(lighting_report.read_bytes()).hexdigest()=='2c3ee0e79a7999b55a65282afdfd65d95196b84ebb2cfe4e13a07eff9c8c5cf5'
lighting=json.loads(lighting_report.read_text())
generator.ENTRIES=tuple(sorted(set(generator.ENTRIES)|{int(row['entry'],16) for row in lighting['owners']}))
for row in lighting['owners']:
    for table in row['jump_tables']:
        if table['width']==4:generator.EXTRA_TABLES[int(table['address'],16)]=table['count']
# Complete direct closure of the live deferred-node builder. Every body and
# branch target is byte-checked against the selected original image.
node_report=Path(__file__).resolve().parents[1]/'analysis/damage-shields-goal-20261012/shield-live-research/node-closure/report.json'
assert hashlib.sha256(node_report.read_bytes()).hexdigest()=='1474c9cfdb1e49ad76a9a0da5349a22092a306983ea6377aa780287f43f57e58'
node_owners=json.loads(node_report.read_text())['owners']
generator.ENTRIES=tuple(sorted(set(generator.ENTRIES)|{int(row['entry'],16) for row in node_owners}))
generator.CONFIGURATION_TOOL=__file__
generator.FORWARD_TIMESTAMP_COUNTER=True
generator.FORWARD_BREAKPOINT=True
if __name__=='__main__':generator.main()
