"""Source-canvas Vista list, password, download and progress graphics.

All contours are authored paths. Measurements specify semantic rails, gaps,
corner cuts and animation windows, not bitmap regions converted into vectors.
"""
import copy

from vista_prompt_svg import element


SHELL='ui\\screens\\game_shell\\'
LIST='ui\\list_skins\\'
PROGRESS_NUMBERS={
    1:'M12 2 H16 V43 H24 V48 H1 V43 H10 V11 L4 17 L0 13 Z',
    2:'M2 9 C4 5 7 1.5 11 1.5 H18 C25 1.5 29 7 29 14.5 '
      'C29 22 23 26 16 30 C11 33 8.5 36 8.5 41 V43 H28 V48 H3 V41 '
      'C3 32 8 29 14.5 25 C21 21 23 18 23 14.5 C23 9 20.5 6.5 17 6.5 '
      'H12 C9 6.5 7 9 5.5 12 Z',
    3:'M1.5 9 C4 5 7 1.5 11 1.5 H18 C24.5 1.5 28 6.5 28 13.5 '
      'V15 C28 19.8 25.5 22.8 22 24.5 C26 26.4 28 29.5 28 34.5 V36 '
      'C28 44 23 48.5 17 48.5 H11 C5 48.5 1.5 45 .5 42 L5.5 39 '
      'C7.5 42 9.5 43.5 12 43.5 H16.5 C20 43.5 22 40 22 36 V34.5 '
      'C22 29.5 19 27.5 14 27.5 H9 V22 H14 C19.5 22 22 19.5 22 15 '
      'V13.5 C22 9 20.5 6.5 17 6.5 H12 C9 6.5 7 8.5 6 12 Z',
    4:'M18.5 2.5 H25 V32 H31 V37 H25 V48 H19 V37 H0 V31 Z M19 11 L6 32 H19 Z',
}

# Individually traced fill phases. Vista does not animate a rectangular mask
# over the whole glyph: its white front branches around the arrow's negative
# space, then travels down the shaft. Preserve those per-frame contours.
DOWNLOAD_WHITE={
    2:'M19 33.5 H23 V35 H19 Z',
    3:'M9 25.5 H12 L20 33.5 H22 L30 25.5 H33 V35 H9 Z',
    4:'M7 17 H9.5 V23.5 L20.5 34.5 H7 Z M35 17 H32.5 V23.5 L21.5 34.5 H35 Z',
    5:'M7 9 H9.5 V23.5 L16 30 V35 H7 Z M35 9 H32.5 V23.5 L26 30 V35 H35 Z',
    6:'M7 7 H16 V9.5 H9.5 V23.5 L14 28 L7 35 Z M35 7 H26 V9.5 H32.5 V23.5 L28 28 L35 35 Z',
    7:'M7 7 H35 V27 L32.5 24 V9.5 H9.5 V24 L7 27 Z',
    8:'M7 7 H35 V19 H32.5 V9.5 H23.5 V17 H18.5 V9.5 H9.5 V19 H7 Z',
    9:'M7 7 H35 V9.5 H23.5 V21 H29 L27 23 H15 L13 21 H18.5 V9.5 H7 Z',
    10:'M14 7 H28 V9.5 H23.5 V21 H29 L21 29.5 L13 21 H18.5 V9.5 H14 Z',
    11:'M18.5 17.5 H23.5 V21 H29 L21 29.5 L13 21 H18.5 Z',
    12:'M17.5 25 H24.5 L21 29.5 Z',
}


def soft_stroke(d,parent,path,colour='#adc3d9',width=1,opacity=1,name='control'):
    element(parent,'path',d=path,fill='none',stroke='#3382cd',stroke_width=width+2,
            opacity=opacity*.45,filter=d.glow(name+'-halo',1))
    element(parent,'path',d=path,fill='none',stroke=colour,stroke_width=width,opacity=opacity)


def download(d):
    if not 1<=d.frame<=12:return False
    d.family='lobby-download-progress'
    g=element(d.root,'g',id='download-state',filter=d.glow('download-edge-softness',.3))
    outline=('M7 7 H35 V35 H7 Z '
        'M9.5 9 H18.5 V21 H13 L21 29.5 L29 21 H23.5 '
        'V9 H32.5 V23 L21 32.5 L9.5 23 Z')
    element(g,'path',id='download-outline',d=outline,fill='#3382cd',fill_rule='evenodd')
    clip=element(d.defs,'clipPath',id='download-surface')
    element(clip,'path',d=outline,clip_rule='evenodd')
    element(g,'path',d=outline,fill='none',stroke='#82bce9',stroke_width=.6,
            opacity=.48,clip_path='url(#download-surface)')
    if d.frame in DOWNLOAD_WHITE:
        element(g,'path',id='download-white-phase',d=DOWNLOAD_WHITE[d.frame],fill='white',clip_path='url(#download-surface)')
    halo=copy.deepcopy(g);halo.set('id','download-halo');halo.set('opacity','.7')
    halo.set('filter',d.glow('download-blue-halo',.65))
    for node in halo.iter():
        if node is not halo:node.attrib.pop('id',None)
        if node.get('fill')=='#3382cd':node.set('fill','#2e9aff')
    d.root.insert(list(d.root).index(g),halo)
    return True


def controls(d,name):
    w,h=d.width,d.height
    if name==SHELL+'pregame_lobby\\download_progress':return download(d)
    if name==SHELL+'xbox_live\\matchmaking_progress_dialog\\progress_numbers':
        if not 0<=d.frame<=7:return False
        d.family='matchmaking-progress-number'
        element(d.root,'path',d=PROGRESS_NUMBERS[d.frame//2+1],
                fill='#386191' if d.frame%2 else '#dadada',fill_rule='evenodd',
                filter=d.glow('number-edge-softness',.18))
    elif name==SHELL+'xbox_live\\matchmaking_progress_dialog\\progress_dots':
        d.family='matchmaking-progress-dots'
        for x in (4,18,32):
            element(d.root,'circle',cx=x,cy=4,r=3,fill='#b97d35',opacity=.61,
                    filter=d.glow('progress-dot-'+str(x),.65))
    elif name in ('ui\\global_bitmaps\\fs_dialog','ui\\global_bitmaps\\xl_dialog') and d.frame==1:
        d.family='dialog-title-strip'
        path='M1 10 H401 L451 60 H51 Z' if name.endswith('fs_dialog') else 'M1 0 H519 L569 50 H51 Z'
        element(d.root,'path',d=path,fill='#3382cd',fill_opacity=207/255)
    elif name==LIST+'game_browser\\bkd_spacer':
        d.family='browser-list-spacer'
        grad=element(d.defs,'linearGradient',id='spacer-fade',x1=0,y1=0,x2=1,y2=0)
        for x,a in ((0,.18),(1,.52)):element(grad,'stop',offset=x,stop_color='#001326',stop_opacity=a)
        element(d.root,'rect',width=w,height=h,fill='url(#spacer-fade)')
    elif name==LIST+'game_browser\\end_caps':
        d.family='browser-list-cap'
        path='M12.5 9 L15 7 V38 L12 35 V25 L7 30 V17 L12 22 Z M9 21 V26 L12 23.5 Z'
        element(d.root,'path',d=path,fill='#2e9aff',stroke='#2e9aff',stroke_width=2,
                filter=d.glow('list-cap-light',1.4))
        element(d.root,'path',d=path,fill='white',fill_rule='evenodd')
    elif name==LIST+'matchmaking\\player_in':
        d.family='matchmaking-player-slot'
        element(d.root,'path',d='M.5 .5 H18.5 V18.5 H.5 Z',fill='none',stroke='#6394c0',
                stroke_width=1.4,opacity=.35,filter=d.glow('player-slot-soft-edge',.35))
        element(d.root,'path',d='M.5 .5 H18.5 V18.5 H.5 Z',fill='none',stroke='#6394c0',stroke_width=1)
        element(d.root,'path',d='M1.25 1.25 H17.75 V17.75 H1.25 Z',fill='none',
                stroke='#29465c',stroke_width=.3,opacity=.55)
        element(d.root,'path',d='M.25 18.75 V.25 H18.75',fill='none',stroke='#b9d1e2',
                stroke_width=.25,opacity=.4)
        if d.frame:
            element(d.root,'rect',x=3,y=3,width=13,height=13,fill='#fcaf5a',
                    stroke='#ffda92',stroke_width=.7,filter=d.glow('player-present',.3))
            element(d.root,'rect',x=4,y=4,width=11,height=11,fill='none',stroke='#ffe5ae',
                    stroke_width=.35,opacity=.35)
    elif name in (LIST+'pcr_lists\\player',LIST+'pcr_lists\\pvp_others'):
        d.family='postgame-player-columns'
        columns=((0,344),(346,162),(510,160),(672,160),(834,160)) if name.endswith('\\player') else \
                ((0,344),(346,324),(672,322))
        for x,width in columns:element(d.root,'rect',x=x,width=width,height=h,fill='white',fill_opacity=99/255)
    elif name==LIST+'pcr_lists\\selecto':
        d.family='postgame-selection-outline'
        element(d.root,'rect',x=.5,y=.5,width=387,height=41,fill='none',stroke='#adc3d9',stroke_width=1)
    elif name==LIST+'playlist_entries\\select':
        d.family='playlist-selection-bracket'
        path='M12 5 H13 L8 10.5 V31 L6 29.5 H5 V17.5 H6 V10.5 Z'
        element(d.root,'path',d=path,fill='#8bbcff',opacity=.35,filter=d.glow('playlist-bracket',1.2))
        element(d.root,'path',d=path,fill='#adc4d7',opacity=.8)
        element(d.root,'path',d='M7 11 H8 V31 L7 30 Z',fill='#e1e7ea',opacity=.7)
        element(d.root,'path',d='M6.5 10.5 L12.5 5.5',fill='none',stroke='#dde3e9',stroke_width=.8,opacity=.8)
    elif name==LIST+'variants\\lock':
        d.family='variant-lock'
        lock=element(d.root,'g',filter=d.glow('lock-edge-softness',.18))
        element(lock,'path',d='M1.3 5.5 V2.8 C1.3 -.4 6.7 -.4 6.7 2.8 V5.5',
                fill='none',stroke='#78a2c8',stroke_width=1.6)
        element(lock,'rect',y=5,width=8,height=6,rx=.5,fill='#78a2c8')
    elif name==LIST+'y_menu_list\\hilite':
        d.family='live-list-highlight'
        element(d.root,'path',d='M3 .5 H471.5 V35 L465 41.5 H3 Z',fill='none',
                stroke='#adc3d9',stroke_width=1)
        element(d.root,'path',d='M8 8 L4 12 V16 L8 20 M8 22 L4 26 V31 L8 35',
                fill='none',stroke='#adc3d9',stroke_width=1)
    elif name=='ui\\player_skins\\ymenu_yourself':
        d.family='live-self-row'
        element(d.root,'rect',width=w,height=h,fill='#cad7e3',fill_opacity=.75)
        element(d.root,'rect',width=21,height=h,fill='#011c33',stroke='#000000',stroke_width=1)
    elif name==SHELL+'dialog_live_password\\password':
        d.family='password-entry-hexagon'
        path='M15 3 H40 L52 24.5 L40 46 H15 L3 24.5 Z'
        element(d.root,'path',d=path,fill='none',stroke='#bfe0ff',stroke_width=1,
                stroke_opacity=.75,filter=d.glow('password-outline-softness',.28))
        element(d.root,'path',d=path,fill='none',stroke='#bfe0ff',stroke_width=2,
                stroke_opacity=.45,filter=d.glow('password-outline-halo',1.3))
        if d.frame:
            element(d.root,'circle',cx=27.5,cy=24.5,r=7,fill='#bfe0ff',opacity=.3,
                    filter=d.glow('password-dot-halo',1.2))
            element(d.root,'circle',cx=27.5,cy=24.5,r=6.5,fill='white',opacity=.75,
                    filter=d.glow('password-dot',.35))
    elif name==SHELL+'load_level\\diff_border':
        d.family='difficulty-selection-border'
        element(d.root,'path',d='M6 .5 H44.5 V38 L38 44.5 H.5 V6 Z',fill='none',
                stroke='#9aabbd',stroke_width=1)
    elif name==SHELL+'network_squad_browser_screen\\arrows' and d.frame in (1,2):
        d.family='browser-scroll-arrow'
        path='M12 25 L26 9 L40 25 H29 L26 22 L23 25 Z'
        if d.frame==2:path='M12 7 H23 L26 10 L29 7 H40 L26 23 Z'
        element(d.root,'path',d=path,fill='#2e9aff',stroke='#2e9aff',stroke_width=2,
                opacity=.5,filter=d.glow('browser-arrow-blue',2))
        element(d.root,'path',id='browser-scroll-arrow-foreground',d=path,fill='#ffffff',opacity=179/255)
    elif name==SHELL+'online_y_menu\\beeper':
        d.family='live-notification-panel'
        path='M7 2 H174 L179 7 V46 H7 L2 41 V2 Z'
        gradient=d.linear('notification-blue',[(0,(0,59,117)),(1,(0,45,90))],y1=2,y2=46,calibrate=False)
        element(d.root,'path',d=path,fill='black',stroke='black',stroke_width=2,opacity=.4,
                filter=d.glow('notification-shadow',1))
        element(d.root,'path',d=path,fill=gradient,fill_opacity=.87,stroke='#adc3d9',stroke_width=.7)
    elif name==SHELL+'online_y_menu\\y_menu_compact':
        d.family='live-compact-panel'
        element(d.root,'path',d='M0 .5 H172 L230 59.5 H529',fill='none',stroke='#3c95d8',stroke_width=1)
        element(d.root,'path',d='M0 1 H172 L230 60 H0 Z',fill='#001a2e')
    elif name==SHELL+'pregame_lobby\\game_settings':
        d.family='lobby-settings-row'
        element(d.root,'path',d='M0 44 V31 L31 .5 H691.5 V44',fill='#122d51',stroke='#adc3d9',stroke_width=1)
    elif name==SHELL+'xbox_live\\xbox_live_main_menu\\y_callout':
        d.family='live-callout-arrow'
        element(d.root,'path',d='M0 0 H19 L9.5 10 Z',fill='#b3cde4',filter=d.glow('callout-softness',.4))
    else:return False
    return True
