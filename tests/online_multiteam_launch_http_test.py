"""Four physical native couches, twelve admitted players, original four-team core.

The single-game rotation and finite variant duration are declared validation
inputs. Four teams/three players/maxparty12 have retained Bungie evidence; the
missing downloaded hopper weights and full variant buffer are not reconstructed.
"""
import asyncio
import hashlib
import json
import os
from pathlib import Path
import socket
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from aiohttp.test_utils import TestClient,TestServer
from aiohttp import web
from multiplayer.master_server.service import Settings,create_app
from multiplayer.master_server.protocol import Problem
from multiplayer.master_server.player_identity import VerifiedOwner
from tests.helpers.mcc_online_ui import validate_cache


def port():
    with socket.socket() as s:
        s.bind(('127.0.0.1',0));return str(s.getsockname()[1])


def catalog():
    # Exercise the same source-bound offering a real service can select through
    # compose.production-matchmaking.yaml. The C++ test alone declares its two
    # finite rounds; catalog dimensions/selection/rating policy stay unchanged.
    return json.loads((ROOT/'multiplayer/master_server/examples/matchmaking-multiteam-native.json').read_text())


def receipt(output,destination,processes,verified,app):
    captures=[]
    for path in sorted(output.glob('*.ppm')):
        captures.append(dict(file=path.name,bytes=path.stat().st_size,sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    value=dict(schema='H2_TU15_FOUR_TEAM_NATIVE_FLOW_V1',passed=len(processes)==4 and all(p.returncode==0 for p in processes),
        retirement_only=bool(os.environ.get('HALO2_MULTITEAM_RETIREMENT_ONLY')),
        original_xbe_sha256='21b207e7efdacaf6124a0822837f8be570766a42505057f7fd2d2ba8f409c4fd',
        physical_native_owners=4,local_players_per_owner=3,admitted_players=12,team_sizes=[3]*4,
        catalog_sha256=hashlib.sha256((ROOT/'multiplayer/master_server/examples/matchmaking-multiteam-native.json').read_bytes()).hexdigest(),
        verified_owners=sorted(set(verified)),captures=captures,
        limits=['External Discord responses are controlled fixtures.','Single-map rotation weight and finite match duration are explicit validation inputs.',
                'Original XBE hash identifies offline compiled-kernel authority; live validation imports only MCC, with no Xbox image/map input.',
                'No installed runtime replacement or backend rollout.',
                'Fixture service rate is explicitly100000/minute with20ms queue polling; production rate-budget/cadence behavior is not qualified by this case.'])
    value['fixture_rate_per_minute']=100000
    if app is not None:
        directory=next(iter(app.values()));db=directory.store.db
        value['persistent_counts']={table:db.execute('SELECT count(*) FROM '+table).fetchone()[0]
            for table in ('players','match_results','match_result_players','match_result_memberships','match_result_skill','carnage_reports')}
    if destination:
        report=Path(destination);report.parent.mkdir(parents=True,exist_ok=True)
        value['phase_observations']={p.name:p.read_text() for p in sorted(output.glob('*-phases.tsv'))}
        captured_names={c['file'] for c in captures}
        value['capture_phase_observations']={}
        for path in sorted(output.glob('*-capture-phases.json')):
            rows=json.loads(path.read_text())
            for row in rows.values():row['actual_capture_exists']=row['file'] in captured_names
            value['capture_phase_observations'][path.name]=rows
        report.write_text(json.dumps(value,indent=2)+'\n')
        # Raw runtime frames remain scratch-owned. Retain one bounded visual
        # evidence sheet, only at the caller's explicit requested output.
        from PIL import Image,ImageDraw
        import textwrap
        # Explicit priorities cover the original menus/sidebar as well as
        # actual loading, source view fades and all six report tabs.
        selected=[]
        def choose(name):
            if name in captured_names and name not in selected:selected.append(name)
        for capture in captures:
            if capture['file'].startswith('one-') and '-loading-' in capture['file']:choose(capture['file'])
        for capture in captures:
            if '-retire-loading-before-pcr' in capture['file']:choose(capture['file'])
        for phase in ('pregame','pregame-countdown'):choose('one-'+phase+'.ppm')
        for pane in range(4):
            candidates=[c['file'] for c in captures if c['file'].startswith(f'one-matchmaking-pane-{pane}-state-')]
            # Actual final state8 carries the complete admitted twelve-row
            # sidebar. Earlier pane0's first observed state shows search.
            if candidates:choose(candidates[-1] if pane==3 else candidates[0])
        prefix='retire-' if value['retirement_only'] else 'game-'
        for phase in ('initial-black','opening-fade','opening-fade-view0','world-visible','round-over',
                      'ending-world-black','pcr-intro-black','pcr-intro-fade','pcr-intro-clear',
                      *(f'pcr-{pane}' for pane in range(6)),'prior-0'):
            choose('one-'+prefix+phase+'.ppm')
        retired=next((c['file'] for c in captures if c['file'].endswith('-retire-ending-world-black.ppm')),None)
        if retired:choose(retired)
        for capture in captures:
            if capture['file'].startswith('one-'):choose(capture['file'])
        for capture in captures:
            if 'retire-game-over' in capture['file']:choose(capture['file'])
        selected=selected[:26]
        value['contact_phase_files']=selected
        value['capture_retention_limits']={'selected_phases':26,'individual_png_bytes':1024*1024,
            'contact_sheet_bytes':1024*1024,'total_png_bytes':8*1024*1024,
            'contact_thumbnails':'Explicitly resized previews; retained individual PNGs preserve complete RGB pixels.'}
        omitted={c['file']:'selection-limit' for c in captures if c['file'] not in selected}
        if selected:
            target=report.with_suffix('.png')
            for width,height in ((256,192),(224,168),(192,144),(160,120),(128,96),(96,72)):
                step=height+32
                sheet=Image.new('RGB',(3*width,22+((len(selected)+2)//3)*step),'#071321');draw=ImageDraw.Draw(sheet)
                draw.text((4,4),'Native12-seat flow | MCC | TU1.5 source21b207e7',fill='white')
                for i,name in enumerate(selected):
                    x,y=(i%3)*width,22+(i//3)*step
                    with Image.open(output/name) as frame:
                        frame=frame.convert('RGB');frame.thumbnail((width,height));sheet.paste(frame,(x,y))
                    label='\n'.join(textwrap.wrap(Path(name).stem,width=max(12,width//6))[:2])
                    draw.multiline_text((x+3,y+height+1),label,fill='white',spacing=1)
                sheet.save(target,optimize=True,compress_level=9)
                if target.stat().st_size<=1024*1024:break
            if target.stat().st_size>1024*1024:
                target.unlink();raise AssertionError('Native contact sheet exceeds its explicit retained size cap')
            value['retained_contact_sheet']=dict(path=str(target),bytes=target.stat().st_size,sha256=hashlib.sha256(target.read_bytes()).hexdigest())
            retained=target.stat().st_size;value['retained_phase_pngs']=[]
            for name in selected:
                target=report.parent/(Path(name).stem+'.png')
                with Image.open(output/name) as frame:
                    frame=frame.convert('RGB');dimensions=list(frame.size);pixel_sha=hashlib.sha256(frame.tobytes()).hexdigest()
                    frame.save(target,optimize=True,compress_level=9)
                size=target.stat().st_size
                if size>1024*1024 or retained+size>8*1024*1024:
                    omitted[name]='individual-size-limit' if size>1024*1024 else 'total-size-limit'
                    target.unlink();continue
                retained+=size;value['retained_phase_pngs'].append(dict(path=str(target),bytes=size,
                    sha256=hashlib.sha256(target.read_bytes()).hexdigest(),dimensions=dimensions,decoded_rgb_sha256=pixel_sha,
                    raw_ppm_sha256=next(c['sha256'] for c in captures if c['file']==name)))
            value['retained_png_bytes']=retained
            assert retained<=8*1024*1024,'Native retained PNGs exceed their explicit total cap'
            value['omitted_full_pngs']=[dict(file=name,reason=reason,in_contact_sheet=name in selected,
                raw_ppm_sha256=next(c['sha256'] for c in captures if c['file']==name)) for name,reason in sorted(omitted.items())]
            report.write_text(json.dumps(value,indent=2)+'\n')
    return value


async def main():
    if len(sys.argv)<3:raise ValueError('Supply native binary and admitted MCC root')
    binary=str(Path(sys.argv[1]).resolve());mcc=validate_cache(ROOT,sys.argv[2]);mcc=Path(sys.argv[2]).resolve()
    if not os.environ.get('DISPLAY'):raise RuntimeError('Four-team native verification requires actual Vulkan display')
    output=Path(os.environ['HALO2_TEST_SCRATCH'])/'four-team';output.mkdir()
    owners=('one','two','three','four');verified=[];processes=[];logs=[];app=None
    evidence=os.environ.get('HALO2_ONLINE_FLOW_EVIDENCE')
    if evidence and not Path(evidence).is_absolute():raise ValueError('Flow evidence directory must be absolute')
    destination=os.environ.get('HALO2_MULTITEAM_REPORT') or (str(Path(evidence)/'multiteam-native.json') if evidence else None)
    with tempfile.TemporaryDirectory(prefix='halo2-multiteam-',dir=output) as temp:
        config=Path(temp)/'playlists.json';config.write_text(json.dumps(catalog()))
        async def verify(session,token,application):
            if token not in owners:raise Problem(401,'Fixture authorization denied')
            assert application=='1558522073272426519';verified.append(token)
            return VerifiedOwner('discord',str(123456789012345678+owners.index(token)),token)
        app=create_app(Settings(state=Path(temp),public_url='http://127.0.0.1:8080',allow_http=True,
            rate_per_minute=100000,matchmaking_catalog=str(config)),verify)
        @web.middleware
        async def trace(request,handler):
            response=await handler(request)
            if response.status>=400:print('HTTP failure',request.method,request.path,response.status,response.text,flush=True)
            return response
        app.middlewares.insert(0,trace)
        try:
            async with TestClient(TestServer(app)) as client:
                for owner in owners:
                    env=os.environ|dict(SDL_AUDIODRIVER='dummy',HTTP_PROXY='http://127.0.0.1:1',NO_PROXY='',no_proxy='',H2_EOS_PROFILES='0')
                    log=(output/(owner+'.log')).open('wb');logs.append(log)
                    processes.append(await asyncio.create_subprocess_exec(binary,str(mcc),str(client.make_url('/')).rstrip('/'),owner,port(),str(output),temp,
                        stdout=log,stderr=asyncio.subprocess.STDOUT,env=env))
                async def completed(owner,process):
                    code=await process.wait()
                    if code:raise RuntimeError(f'Actual native owner {owner} exited {code}')
                failure=None
                try:await asyncio.wait_for(asyncio.gather(*(completed(w,p)for w,p in zip(owners,processes))),500)
                except Exception as error:
                    failure=error
                    for process in processes:
                        if process.returncode is None:process.kill()
                    await asyncio.gather(*(p.wait()for p in processes))
                for log in logs:log.close()
                for owner,p in zip(owners,processes):
                    data=(output/(owner+'.log')).read_bytes()
                    print(owner,p.returncode,data[-128*1024:].decode(errors='replace'),flush=True)
                value=receipt(output,destination,processes,verified,app)
                if failure:raise RuntimeError(str(failure))
                assert value['passed'],'Actual native four-team world/flow failed'
                assert set(verified)==set(owners),'Not every primary login was authenticated'
                counts=value['persistent_counts']
                if value['retirement_only']:
                    assert counts['players']==4 and all(counts[n]==0 for n in counts if n!='players'),counts
                    print('PASS focused native retirement; full two-round/requeue path excluded')
                    return 0
                assert counts['players']==4 and counts['match_results']==1 and counts['match_result_players']==4,counts
                # Full first result saves four verified primaries; the second
                # remaining-team presentation has no departed-owner DB consent.
                assert counts['match_result_memberships']==4 and counts['match_result_skill']==4 and counts['carnage_reports']==1,counts
                print('PASS actual twelve-seat four-team source gameplay/PCR/requeue and authenticated opponent retirement; only unanimous first result persisted')
        finally:
            for p in processes:
                if p.returncode is None:p.kill();await p.wait()
            for log in logs:
                if not log.closed:log.close()
            if not destination:receipt(output,None,processes,verified,app)
    return 0


if __name__=='__main__':sys.exit(asyncio.run(main()))
