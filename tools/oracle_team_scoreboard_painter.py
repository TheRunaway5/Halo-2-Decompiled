#!/usr/bin/env python3
"""Original Team scoreboard background/quality/pulse submission fragment.

The original builder executes against the unchanged map with declared API
roster/stat/view inputs. Its exact produced layout and clock are delivered to
the renderer's post-begin stack/register boundary. The void rectangle service
records arguments and returns with EAX unchanged. Graphics begin, emblems,
font startup, GPU execution and a complete match are not established.
"""
import argparse, copy, json
from pathlib import Path
from oracle_mp_scoreboard_draw_1_5 import DrawTrace, C, S
from oracle_mp_full_scoreboard_1_5 import ROOT, NEW, NEW_PIN, HudCache, O, sha, UC_X86_REG_EAX
from oracle_team_full_scoreboard import configure, bits
from unicorn.x86_const import UC_X86_REG_EBX

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output',type=Path,default=ROOT/'analysis/multiplayer/title-update-1.5/team-full-scoreboard/painter.json')
    args=ap.parse_args();fixture_path=ROOT/'tests/fixtures/team_full_scoreboard_original.json'
    fixture=json.loads(fixture_path.read_text());assert sha(NEW)==NEW_PIN
    t=DrawTrace(HudCache(ROOT/'assets/disc/maps/lockout.map'));t.fixture()
    assert t.run('original_engine_constructor',0x3825d0)['completion']=='complete_original_return'
    wanted=('columns-2-False-False-0-25','columns-2-False-False-3-1','masks-255-0','split-scroll-3-33.25')
    cases=[]
    for name in wanted:
        original=next(c for c in fixture['cases'] if c['name']==name);x=copy.deepcopy(original['input']);configure(t,x)
        t.calls.clear();t.services.clear()
        builder=t.run('original_complete_team_builder',0x163360,(0x80000000,O,bits(x['alpha'])))
        assert builder['completion']=='complete_original_return'
        raw=bytes(t.g.mem_read(O,0x293c));clock=bytes(t.g.mem_read(C,12))
        current=next(i for i in range(t.u(O+0x2938)) if t.u(O+0xcb8+i*0x1c8+0x1c4,1))
        t.rectangles=[]
        pulse=t.run('original_current_player_pulse',0x162a00,regs=((UC_X86_REG_EAX,O+0xcb8+current*0x1c8),))
        assert pulse['completion']=='complete_original_return' and len(t.rectangles)==4
        pulse_color=t.rectangles[0]['color16_hex']
        t.g.mem_write(S-0x294c+0x10,raw);t.g.mem_write(C,clock);t.rectangles=[]
        fragment=t.run('original_team_background_fragment',0x163ef5,regs=((UC_X86_REG_EBX,0),),tail=0x294c,block=0x1640dd)
        assert fragment['completion']=='stopped_original_instruction' and fragment['stop']['kind']=='bounded_background_fragment_end_before_text_state'
        cases.append(dict(name=name,clock12_hex=clock.hex(),pulse_color16_hex=pulse_color,
            original_builder_completion=builder['completion'],original_layout293c_hex=raw.hex(),
            fragment=fragment,rectangles=t.rectangles))
    report=dict(schema='original-team-scoreboard-background-v1',boundaries=__doc__,oracle_sha256=sha(__file__),
        source_xbe_sha256=NEW_PIN,fixture_sha256=sha(fixture_path),
        void_service_contract='rectangle submission records arguments; EAX unchanged; cleanup4',
        instruction_patches=[],supplied_game_or_gpu_results=[],cases=cases,
        summary=dict(original_team_background_fragments=len(cases),rectangle_requests=sum(len(c['rectangles']) for c in cases)))
    args.output.parent.mkdir(parents=True,exist_ok=True);args.output.write_text(json.dumps(report,indent=2,sort_keys=True)+'\n')
    print(json.dumps(report['summary']))
if __name__=='__main__':main()
