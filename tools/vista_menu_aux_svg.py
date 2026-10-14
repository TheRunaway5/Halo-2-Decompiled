"""Source-measured Vista browser dividers, grids and small status drawings."""
from vista_prompt_svg import element
from vista_menu_materials_svg import linear,fade_mask


SHELL='ui\\screens\\game_shell\\'
CALLOUTS={
    0:('M8 108.5 H123 L198 183','M306 360 H500'),
    1:('M2 213 H166','M344 309 L403 250 H488','M32 476 H162 L215 423'),
    2:('M373 206 H502','M9 270 H155 L190 235','M3 412 H87'),
    3:('M295 210 H508','M3 390 H167 L181 376 M110 390 L151 349'),
}


def divider(d,name):
    d.family='browser-divider-panel'
    if name.endswith('live_divider'):
        path='M1 31 L31 1 H562 V227 L541 248 H1 Z'
        anchors=((0,1),(320,1),(366,.75),(394,.494),(422,.216),(450,.039),(470,0),(563,0))
        height=249
    elif 'network_squad_browser_screen' in name:
        path='M1 31 L31 1 H818 V260 L797 281 H1 Z'
        anchors=((0,1),(532,1),(573,.721),(614,.337),(655,.035),(675,0),(819,0))
        height=282
    else:
        path='M462 1 H31 L1 31 V413 H462'
        anchors=((0,1),(305,1),(333,.933),(358,.749),(384,.518),(410,.255),(438,.05),(462,0),(512,0))
        height=414
    paint=linear(d,'divider-height',[(0,'#09152a',1),(.5,'#0e1e32',1),(1,'#12283a',1)],y2=height)
    fade=linear(d,'divider-fade',[(x/d.width,'white',alpha) for x,alpha in anchors],x2=d.width,y2=0)
    element(d.root,'path',d=path+' Z',fill=paint,mask=fade_mask(d,'divider-fill-envelope',fade))
    element(d.root,'path',d=path,fill='none',stroke='#9aabbd',stroke_width=2)


def grid(d,profile):
    d.family='profile-grid-material' if profile else 'message-grid-material'
    step,width,height,colour,alpha=(26,444,652,'#2680db',46/255) if profile else (13,269,189,'#217bdb',34/255)
    pattern=element(d.defs,'pattern',id='grid-lines',patternUnits='userSpaceOnUse',width=step,height=step)
    element(pattern,'path',d=f'M1 0 V{step} M0 1 H{step}' if profile else f'M.5 0 V{step} M0 .5 H{step}',
            fill='none',stroke=colour,stroke_width=2 if profile else 1,opacity=alpha)
    xstops=((0,1),(400/512,1),(444/512,0),(1,0)) if profile else \
           ((0,0),(5/512,0),(24/512,1),(245/512,1),(269/512,0),(1,0))
    ystops=((0,.87),(26/1024,1),(625/1024,1),(652/1024,.65),(1,.65)) if profile else \
           ((0,0),(8/256,0),(28/256,1),(168/256,1),(189/256,0),(1,0))
    horizontal=linear(d,'grid-width',[(x,'white',a) for x,a in xstops],x2=d.width,y2=0)
    vertical=linear(d,'grid-height',[(y,'white',a) for y,a in ystops],y2=d.height)
    group=element(d.root,'g',mask=fade_mask(d,'grid-width-envelope',horizontal))
    element(group,'rect',width=width,height=height,fill='url(#grid-lines)',
            mask=fade_mask(d,'grid-height-envelope',vertical))


def reticle(d):
    d.family='difficulty-reticle-symbol'
    group=element(d.root,'g',fill='none',stroke='#5686b3',stroke_width=1.2,
                  opacity=.92,filter=d.glow('reticle-edge',.35))
    element(group,'circle',cx=25,cy=25,r=24)
    if d.frame==0:
        element(group,'path',d='M1 25 H49 M25 1 V49 M22 9 H28 M22 17 H28 M22 33 H28 M22 41 H28')
    else:
        element(group,'circle',cx=25,cy=20,r=12.8)
        element(group,'path',d='M21.5 49 V36.8 A17.2 17.2 0 1 1 28.5 36.8 V49')


def server_icon(d):
    d.family='browser-server-status'
    g=element(d.root,'g',filter=d.glow('server-edge',.34))
    if d.frame<3:
        outline='M7.5 6.5 L10.5 3.5 H24.5 V24.5 L20.5 28.5 H7.5 Z'
        element(d.root,'path',d=outline,fill='none',stroke='#78b1ff',stroke_width=2,
                opacity=.18,filter=d.glow('server-blue-halo',1.2))
        element(g,'path',d=outline,fill='none',stroke='#d4dfed',stroke_width=1.2,opacity=.58)
        element(g,'path',d='M7.5 6.5 H19 Q21 6.5 21 8.5 V28.5',fill='none',
                stroke='#c1cfe0',stroke_width=1,opacity=.6)
        for y in (8.5,10.7,12.8):
            element(g,'path',d=f'M10 {y} H18.5',fill='none',stroke='#bbcce2',stroke_width=1,opacity=.55)
    else:
        element(d.root,'circle',cx=16,cy=16,r=11.5,fill='none',stroke='#78b1ff',
                stroke_width=2.2,opacity=.16,filter=d.glow('server-circle-halo',1.2))
        element(g,'circle',cx=16,cy=16,r=11.5,fill='none',stroke='#c6d7ec',stroke_width=1.1,opacity=.55)
    if d.frame:
        colour='#dce6ed' if d.frame==1 else '#ffdf5c'
        x,y,width,height=(6,21,10,7.5) if d.frame<3 else (11,14,10,8)
        lock=element(g,'g',fill=colour,stroke=colour,opacity=.58)
        element(lock,'path',d=f'M{x+2} {y} V{y-2} C{x+2} {y-4.7} {x+8} {y-4.7} {x+8} {y-2} V{y}',
                fill='none',stroke_width=1.3)
        element(lock,'rect',x=x,y=y,width=width,height=height,rx=.7,stroke_width=.5)


def auxiliary(d,name):
    if name in (SHELL+'network_squad_browser_screen\\divider',
                SHELL+'network_squad_browser_screen\\live_divider',SHELL+'pregame_lobby\\divider'):
        divider(d,name)
    elif name==SHELL+'online_y_menu\\y_menu_bkd' and d.frame==0:
        d.family='live-panel-background'
        element(d.root,'rect',width=512,height=409,fill='#00192e',fill_opacity=238/255)
    elif name==SHELL+'profile_picker\\profile_display':grid(d,True)
    elif name==SHELL+'xbox_live\\message_send\\friend_grid':grid(d,False)
    elif name==SHELL+'difficulty\\difficulty_callouts' and d.frame in CALLOUTS:
        d.family='difficulty-callout-line'
        for path in CALLOUTS[d.frame]:
            element(d.root,'path',d=path,fill='none',stroke='#219aef',stroke_width=1,opacity=234/255)
    elif name==SHELL+'difficulty\\flavor_ani' and d.frame in (0,1):reticle(d)
    elif name==SHELL+'network_squad_browser_screen\\ds_icon' and d.frame in range(4):server_icon(d)
    else:return False
    return True
