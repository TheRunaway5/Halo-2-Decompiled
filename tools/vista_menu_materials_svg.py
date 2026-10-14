"""Authored Vista frontend row materials, tab rails and progress graphics.

Native canvases include substantial empty padding. The measured bounds below
belong to painted parts, not replacement texture sizes. Soft fields use a few
editable paint anchors; no source raster, sampled pixel grid or font is used.
"""
import math

from vista_prompt_svg import element
from vista_menu_panels_svg import stripe_material


SHELL='ui\\screens\\game_shell\\'
LIST='ui\\list_skins\\'


def linear(d,name,stops,x1=0,y1=0,x2=0,y2=1):
    gradient=element(d.defs,'linearGradient',id=name,gradientUnits='userSpaceOnUse',
                     x1=x1,y1=y1,x2=x2,y2=y2)
    for offset,colour,alpha in stops:
        element(gradient,'stop',offset=offset,stop_color=colour,stop_opacity=alpha)
    return f'url(#{name})'


def fade_mask(d,name,paint):
    mask=element(d.defs,'mask',id=name,maskUnits='userSpaceOnUse',
                 x=0,y=0,width=d.width,height=d.height)
    element(mask,'rect',width=d.width,height=d.height,fill=paint)
    return f'url(#{name})'


def vertical_fade(d,name,anchors):
    return linear(d,name,[(y/d.height,'white',alpha) for y,alpha in anchors],y2=d.height)


def menu_wave(d,name):
    d.family='main-menu-wave-material'
    w,h=d.width,d.height
    multiply='multiply' in name
    rings=name.endswith('rings_right')
    if multiply:
        fade=vertical_fade(d,'ripple-height',[(0,0),(4,0),(12,.3),(23,.8),
            (31,1),(43,.8),(54,.25),(59,0),(62,0)])
        paint=linear(d,'ripple-length',[(0,'#f4faff',.08),(.25,'#f4faff',.05),
            (.5,'#f4faff',.045),(.75,'#f4faff',.025),(.89,'#f4faff',0),(1,'#f4faff',0)],x2=w,y2=0)
    else:
        if rings:
            fade=vertical_fade(d,'wave-height',[(0,0),(4,0),(10,.23),(15,.33),
                (23,.6),(31,1),(40,.7),(46,.5),(54,.13),(59,0),(62,0)])
            stops=[(0,'#15497c',0),(.033,'#15497c',0),(.125,'#15497c',.047),
                (.25,'#1a5a92',.074),(.5,'#2c96dd',.145),(.75,'#1a5a92',.055),
                (.875,'#15497c',.043),(.925,'#15497c',0),(1,'#15497c',0)]
        else:
            fade=vertical_fade(d,'wave-height',[(0,0),(9,0),(15,.19),(20,.65),
                (25,.92),(31,1),(36,.8),(42,.4),(47,.1),(53,0),(62,0)])
            stops=[(0,'#15497c',0),(.015,'#15497c',0),(.125,'#15497c',.23),
                (.25,'#1a5a92',.31),(.5,'#2c96dd',.32),(.75,'#1a5a92',.27),
                (.875,'#15497c',.22),(.98,'#15497c',0),(1,'#15497c',0)]
        paint=linear(d,'wave-length',stops,x2=w,y2=0)
    g=element(d.root,'g',mask=fade_mask(d,'wave-envelope',fade))
    if name.endswith('multiply_left'):
        g=element(g,'g',transform=f'translate({w} 0) scale(-1 1)')
    element(g,'rect',width=w,height=h,fill=paint)
    if multiply:
        # The source has five broad, nested crescent bands fading away from
        # the outside edge. Their smooth boundaries remain editable curves.
        bands=element(g,'g',fill='#f4faff',filter=d.glow('ripple-softness',2))
        for x,alpha in ((15,.018),(44,.035),(80,.035),(122,.03),(163,.02)):
            element(bands,'path',d=f'M{x-35} 7 C{x+35} 13 {x+35} 48 {x-35} 55 '
                f'C{x+15} 43 {x+15} 19 {x-35} 7 Z',opacity=alpha)
    elif rings:
        # Separate elliptical wavefronts, rather than a single blue smudge.
        field=vertical_fade(d,'ring-height',[(0,0),(4,0),(12,.65),(25,1),
            (40,1),(53,.4),(59,0),(62,0)])
        ring_group=element(d.root,'g',mask=fade_mask(d,'ring-envelope',field),
                           fill='none',stroke='#2c96dd',filter=d.glow('ring-softness',2.4))
        for rx,ry,width,alpha in ((35,24,7,.05),(67,24,6,.035),(99,23,5,.025)):
            element(ring_group,'ellipse',cx=240,cy=31,rx=rx,ry=ry,
                    stroke_width=width,opacity=alpha)


def browser_hover(d,name):
    d.family='browser-column-hover'
    w,h=d.width,d.height
    if name.endswith('column_head_hover'):
        paint=linear(d,'header-height',[(0,'#2c95dd',0),(30/62,'#2c95dd',0),
            (32/62,'#2c95dd',4/255),(40/62,'#2c95dd',18/255),
            (50/62,'#2c95dd',38/255),(60/62,'#2c95dd',58/255),(1,'#2c95dd',58/255)],y2=62)
        end=linear(d,'header-ends',[(0,'white',0),(9/480,'white',0),
            (15/480,'white',1),(465/480,'white',1),(470/480,'white',0),(1,'white',0)],x2=w,y2=0)
        element(d.root,'rect',width=w,height=61.5,fill=paint,mask=fade_mask(d,'header-envelope',end))
    else:
        extent=48 if name.endswith('_small') else 104
        fade=vertical_fade(d,'column-height',[(0,.18),(8,.4),(18,.78),
            (31,1),(44,.78),(54,.4),(62,.18)])
        group=element(d.root,'g',mask=fade_mask(d,'column-envelope',fade))
        for side,x1,x2 in (('left',0,extent),('right',w,w-extent)):
            paint=linear(d,'column-'+side,[(0,'#2c95dd',.2),(.05,'#2c95dd',.2),
                (.3,'#2c95dd',.15),(.6,'#2c95dd',.075),(.85,'#2c95dd',.012),
                (1,'#2c95dd',0)],x1=x1,x2=x2,y2=0)
            element(group,'rect',x=0 if side=='left' else w-extent,
                    width=extent,height=h,fill=paint)


def tabs(d):
    d.family='live-navigation-tabs'
    left='M0 .5 H172 L209 37.5 H0 Z'
    right='M192 22 L209 5.5 H528.5 V37.5 H209 Z'
    paint=linear(d,'tab-blue',[(0,'#1f4e7d',232/255),(1,'#122a46',232/255)],
                 x1=192,x2=529,y2=0)
    element(d.root,'path',d=right,fill=paint,stroke='#45a7f0',stroke_width=1,stroke_opacity=.68)
    shade=linear(d,'tab-bottom',[(0,'#000000',0),(.65,'#000000',0),(1,'#000000',.18)],y2=38)
    element(d.root,'path',d=right,fill=shade)
    element(d.root,'path',d=left,fill='#00192d',fill_opacity=232/255)
    element(d.root,'path',d='M0 .5 H172 L209 37.5',fill='none',
            stroke='#45a7f0',stroke_width=1,stroke_opacity=.88)
    for x,direction in ((63,-1),(169,1)):
        g=element(d.root,'g',transform=f'translate({x} 18.5) scale({direction} 1)')
        path='M-3 -8 L4 0 L-3 8 V3 L0 0 L-3 -3 Z'
        element(g,'path',d=path,fill='#2e9aff',stroke='#2e9aff',stroke_width=2,
                filter=d.glow('tab-arrow-'+str(x),1.3))
        element(g,'path',d=path,fill='#e3f2ff',filter=d.glow('tab-arrow-edge-'+str(x),.25))


def progress_ring(d):
    d.family='matchmaking-segmented-ring'
    # Measured circular bands, with individual radial gaps and narrow index
    # marks. Angles run clockwise from the top around Vista's off-centre ring.
    bands=((40.5,51,((310,386),)),
           (45,51,((29,71),(88,126),(128,133),(135,160),(170,230))),
           (40,51,((76,84),)),
           (54,57,((8,66),(279,316))),
           (54,63,((121,197),(223,279))),
           (58,63,((7,22),(68,118),(339,358),
                   (201,204),(208,212),(216,219),(322,328),(331,335))))
    paint=linear(d,'progress-ring-paint',[(0,'#1b3870',.5),(1,'#203f7d',.5)],x2=128,y2=0)
    group=element(d.root,'g',fill=paint,filter=d.glow('progress-ring-edge',.2))
    def point(radius,angle):
        angle=math.radians(angle)
        return 62.5+radius*math.sin(angle),62.5-radius*math.cos(angle)
    for inner,outer,sectors in bands:
        for start,end in sectors:
            a,b=point(outer,start),point(outer,end);c,e=point(inner,end),point(inner,start)
            large=int(end-start>180)
            path=f'M{a[0]} {a[1]} A{outer} {outer} 0 {large} 1 {b[0]} {b[1]} '
            path+=f'L{c[0]} {c[1]} A{inner} {inner} 0 {large} 0 {e[0]} {e[1]} Z'
            element(group,'path',d=path)
    a,b=point(51,230),point(51,308);c,e=point(45,308),point(45,265)
    element(group,'path',d=f'M{a[0]} {a[1]} A51 51 0 0 1 {b[0]} {b[1]} '
            f'L{c[0]} {c[1]} A45 45 0 0 0 {e[0]} {e[1]} '
            f'C15.5 77 17 87 {a[0]} {a[1]} Z')


def materials(d,name):
    w,h=d.width,d.height
    if name in (LIST+'main_menu\\list_bkd',LIST+'main_menu\\list_bkd_multiply',
                LIST+'main_menu\\list_bkd_multiply_left',LIST+'main_menu\\list_bkd_rings_right'):
        menu_wave(d,name)
    elif name in (SHELL+'live_game_browser\\column_head_hover',SHELL+'live_game_browser\\column_hover',
                  SHELL+'live_game_browser\\column_hover_small'):
        browser_hover(d,name)
    elif name==LIST+'matchmaking\\item_background':
        d.family='matchmaking-row-background'
        element(d.root,'rect',width=168,height=19,fill='#6394c0',fill_opacity=102/255)
    elif name==LIST+'y_menu_list\\item_background':
        d.family='live-list-row-material'
        path='M.5 .5 H422.5 V31.5 L416.5 37.5 H.5 Z'
        clip=element(d.defs,'clipPath',id='live-row-shape');element(clip,'path',d=path)
        paint=stripe_material(d,'live-row-engraving',4,1,(255,255,255),133,119)
        element(d.root,'path',d=path,fill=paint,stroke='white',stroke_width=1)
    elif name==SHELL+'main_menu_screen\\track_brace':
        d.family='main-menu-track-brace'
        g=element(d.root,'g',fill='#314d9c',opacity=65/255,filter=d.glow('track-brace-edge',.2))
        element(g,'path',d='M9 39 L12 36 V24 L14 21 V44 H36 V21 L38 24 V36 '
                'L41 39 V49 H30 L25 44 L20 49 H9 Z')
        element(g,'path',d='M21 50 L25 46 L29 50 M25 49 V58.5 L59 93 H69',
                fill='none',stroke='#314d9c',stroke_width=1.7,stroke_linejoin='round')
    elif name==SHELL+'online_y_menu\\tabs':tabs(d)
    elif name==SHELL+'online_y_menu\\y_menu_bkd' and d.frame==1:
        d.family='live-panel-side-rail'
        element(d.root,'rect',width=16,height=409,fill='#00192e',fill_opacity=238/255)
        element(d.root,'rect',x=15,width=1,height=409,fill='#4ab2ff',fill_opacity=238/255)
    elif name==SHELL+'pregame_lobby\\matchmaking2':
        d.family='lobby-matchmaking-arrows'
        g=element(d.root,'g',fill='#5c7995',filter=d.glow('matching-arrow-edge',.3))
        for angle in (0,90,180,270):
            element(g,'path',d='M32 0 H38 V15 L46 7.5 L50.5 12 L35 27.5 L19.5 12 L24 7.5 L32 15 Z',
                    transform=f'rotate({angle} 35 35)')
    elif name==SHELL+'xbox_live\\matchmaking_progress_dialog\\done':progress_ring(d)
    elif name==LIST+'pcr_lists\\team':
        d.family='postgame-team-columns'
        for x,width in ((0,480),(482,276),(760,276)):
            element(d.root,'rect',x=x,width=width,height=38,fill='white',fill_opacity=99/255)
    elif name==LIST+'setting_lists\\settings_list_bkd':
        d.family='settings-list-material'
        path='M0 4 H1016 V44 L1012 48 H0 Z'
        paint=stripe_material(d,'settings-engraving',8,4,(201,219,249),52,36)
        element(d.root,'path',d=path,fill=paint)
    elif name in ('ui\\global_bitmaps\\fs_dialog','ui\\global_bitmaps\\xl_dialog') and d.frame==0:
        d.family='large-dialog-panel'
        path=('M.5 1.5 H611.5 L671.5 61.5 H1080.5 V802.5 H56.5 L.5 746.5 Z'
              if name.endswith('fs_dialog') else
              'M.5 1.5 H949.5 L1007.5 59.5 H1529.5 V1135 H79.5 L.5 1056 Z')
        element(d.root,'path',d=path,fill='#02162f',stroke='#9aabbd',stroke_width=1.5)
    else:return False
    return True
