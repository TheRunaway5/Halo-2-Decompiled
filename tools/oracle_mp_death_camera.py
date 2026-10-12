#!/usr/bin/env python3
"""Original death-camera packet and observer dependency diagnostic.

Reuses the allocated equipped-unit/decoded-pose fixture. Current ownership
release, previous-unit binding, local RNG input and frame dt are declared API
stimuli. No fatal hit, natural ragdoll, solver or original session is supplied.
Stops at missing dependencies without assigning successful game returns.
"""
import argparse
import hashlib
import json
from pathlib import Path
from unicorn import UcError
from unicorn.x86_const import UC_X86_REG_EDX
from oracle_mp_scope_camera_1_1 import (
    ScopeCameraTrace, INITIALIZERS, OBSERVER, PACKET, COMMAND, PERSISTENT,
    ROOT, PIN, CACHE_PIN, STOP, h, UC_X86_REG_EAX, UC_X86_REG_ECX,
    UC_X86_REG_ESI, UC_X86_REG_EDI, UC_X86_REG_ESP, UC_X86_REG_EIP)
from oracle_smg_firing_1_1 import PLAYER_DATA, USERS, UNIT, UI, SEED


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


class DeathCameraTrace(ScopeCameraTrace):
    def invalid(self, machine, access, address, size, value, user):
        self.invalids = dict(address=h(address), access=access, size=size,
            recent=[h(entry) for entry in self.recent])
        return super().invalid(machine, access, address, size, value, user)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    trace = DeathCameraTrace()
    stages = []
    try:
        for entry in INITIALIZERS:
            stages.append(trace.measure('source_initializer_' + h(entry), entry))
        stages.append(trace.measure('source_observer_constructor', 0x170f30,
            regs=((UC_X86_REG_ESI, OBSERVER),)))
        trace.w(COMMAND, 0)
        trace.f(COMMAND + 8, 1 / 30)
        trace.w(PLAYER_DATA + 0x2c, -1)
        trace.w(PLAYER_DATA + 0x30, UI)
        trace.w(USERS + 12, 0)
        trace.w(SEED + 4, 0x12345678)
        stages.append(trace.measure('source_death_camera_constructor', 0x23fb00,
            (-1,), ((UC_X86_REG_EAX, PERSISTENT), (UC_X86_REG_EDX, 0)),
            boundary={'current_unit': 'ffffffff', 'previous_unit': h(UI),
                'local_rng_input': '12345678', 'local_view': 0}))
        controller = bytes(trace.g.mem_read(PERSISTENT, 0x40)).hex()
        trace.g.mem_write(PACKET, bytes(0xac))
        stages.append(trace.measure('source_death_camera_packet', 0x23ff50,
            (PERSISTENT, COMMAND, PACKET)))
        packet = bytes(trace.g.mem_read(PACKET, 0xac)).hex()
        stages.append(trace.measure('source_packet_registration', 0x170c10,
            regs=((UC_X86_REG_EAX, PACKET), (UC_X86_REG_ECX, 0))))
        stages.append(trace.measure('source_packet_adoption', 0x170ff0, (0,)))
        trace.w(0x21fd004, 0x3d088889)
        trace.g.reg_write(UC_X86_REG_ESP, 0x21fd000)
        trace.g.emu_start(0x170d00, 0x170d1c, count=20)
        stages.append(trace.measure('source_observer_motion', 0x171910,
            regs=((UC_X86_REG_EDI, 0),)))
        stages.append(trace.measure('source_death_camera_world_render', 0x172a50, (0,)))
        stop = None
    except UcError as error:
        stop = dict(phase=trace.active, eip=h(trace.g.reg_read(UC_X86_REG_EIP)),
            error=str(error), camera=trace.camera_state(),
            inherited_stop=getattr(trace, 'invalids', None))
        controller = bytes(trace.g.mem_read(PERSISTENT, 0x40)).hex()
        packet = bytes(trace.g.mem_read(PACKET, 0xac)).hex()
    report = dict(tool_sha256=sha(__file__), original_xbe_sha256=PIN,
        cache_sha256=CACHE_PIN, boundaries=__doc__, stages=stages,
        controller_hex=controller, packet_hex=packet, stop=stop,
        original_node0_hex=bytes(trace.g.mem_read(UNIT + trace.u(UNIT + 0x116, 2), 0x34)).hex(),
        local_rng_after=h(trace.u(SEED + 4)),
        inherited_services=[dict(entry=h(entry), contract=contract)
            for entry, contract in trace.services.items()],
        instruction_patches=[], native_camera_installed=False)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(dict(stages=len(stages), stop=stop and
        {key:stop[key] for key in ('phase', 'eip', 'error')})))


if __name__ == '__main__':
    main()
