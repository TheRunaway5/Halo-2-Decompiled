"""Vista menu status symbols with their original translucent paint semantics."""
from vista_prompt_svg import element


BROWSER='ui\\screens\\game_shell\\network_squad_browser_screen\\'


def browser_icon(d,name):
    kind=name.rsplit('\\',1)[1]
    if kind not in ('qos_icon','custom_map_icon','favorites_icon','friends_icon'):return False
    d.family='browser-status-icon'
    if kind=='qos_icon':
        filled=max(0,d.frame-1)
        group=element(d.root,'g',opacity=205/255)
        for i,(x,y,height) in enumerate(((2.5,17.5,11),(12.5,12.5,16),(22.5,6.5,22))):
            element(group,'rect',x=x,y=y,width=6,height=height,fill='none',
                    stroke='#4194e6',stroke_width=3,stroke_opacity=70/205)
            element(group,'rect',x=x,y=y,width=6,height=height,
                    fill='#a9cef3' if i<filled else 'none',stroke='#a9cef3',stroke_width=1)
    elif d.frame!=1:return False
    elif kind=='custom_map_icon':
        path=('M5 5 H28 V28 H5 Z M8 8 H14 V17 H11 L16.5 22 L22 17 H19 V8 H25 '
              'V19 L16.5 26 L8 19 Z')
        group=element(d.root,'g',opacity=180/255)
        element(group,'path',d=path,fill='#a9cef3',
                fill_rule='evenodd',stroke='#4194e6',stroke_opacity=70/180,stroke_width=1,
                stroke_linejoin='miter')
    elif kind=='favorites_icon':
        path='M15.5 5.2 L19 13 H26.5 L20.5 18.5 L22.6 25.6 L15.5 21 L8.4 25.6 L10.5 18.5 L4.5 13 H12 Z'
        group=element(d.root,'g',opacity=180/255)
        element(group,'path',d=path,fill='#a9cef3',
                stroke='#4194e6',stroke_width=1,stroke_opacity=70/180,
                filter=d.glow('star-edge-softness',.35))
    elif kind=='friends_icon':
        group=element(d.root,'g',fill='#a9cef3',fill_opacity=155/255,
                      filter=d.glow('friend-edge-softness',.4))
        element(group,'circle',cx=16.5,cy=8, r=3.6)
        element(group,'path',d='M9 13 H23 L11 29 Z')
    return True


def speaker(d):
    d.family='lobby-speaker-icon'
    color='#ffffff' if d.frame==0 else '#3382cd'
    group=element(d.root,'g',fill=color)
    if d.frame==2:
        element(group,'rect',x=9,y=9,width=28,height=26,fill='none',stroke=color,stroke_width=2)
        element(group,'path',d='M13 17.5 H15 V27 H13 Z M17 17 L23 13 V30 L17 27 Z')
        element(group,'path',d='M26 19 L28 21.5 L26 24 Z')
        for x,y in ((32,24),(32,29)):
            element(group,'rect',x=x,y=y,width=2,height=2)
        return
    paths=('M7 14 H13 V29 H7 Z M14 14 L22 7 V36 L14 29 Z',
           'M29 11 C34 16 34 26 29 31', 'M27 15 C30 18 30 24 27 27')
    if d.frame==0:
        element(group,'path',d=paths[0],stroke='#2e9aff',stroke_width=4,
                opacity=.65,filter=d.glow('speaker-glow',2.2))
    element(group,'path',d=paths[0])
    for path in paths[1:]:
        if d.frame==0:
            element(group,'path',d=path,fill='none',stroke='#2e9aff',stroke_width=4,
                    opacity=.65,filter=d.glow('speaker-wave-glow-'+str(len(group)),1.5))
        element(group,'path',d=path,fill='none',stroke=color,stroke_width=1.7,
                stroke_linecap='round')


def status(d,name):
    if name.startswith(BROWSER):return browser_icon(d,name)
    if name=='ui\\screens\\game_shell\\pregame_lobby\\speaker':speaker(d)
    elif name=='ui\\screens\\game_shell\\postgame_statistics\\team':
        d.family='postgame-team-panel'
        for x,width in ((0,480),(482,276),(760,276)):
            element(d.root,'rect',x=x,width=width,height=38,fill='black',fill_opacity=99/255)
    elif name=='ui\\screens\\game_shell\\postgame_statistics\\spinners':
        d.family='postgame-spinner-arrow'
        path='M23 7 L6 23 L23 39 V29 L17 23 L23 17 Z'
        group=element(d.root,'g',opacity=227/255,
                      transform='translate(32 0) scale(-1 1)' if d.frame else 'translate(0 0)')
        element(group,'path',d=path,fill='#2e9aff',stroke='#2e9aff',stroke_width=3,
                filter=d.glow('spinner-glow',2.2))
        element(group,'path',d=path,fill='white')
    elif name=='ui\\screens\\game_shell\\pregame_lobby\\settings_framing':
        d.family='lobby-settings-divider'
        element(d.root,'rect',width=346,height=2,fill='#7a848d')
    else:return False
    return True
