"""Authored Vista light-only corners and composite menu divider drawings."""
from vista_prompt_svg import element
from vista_menu_corner_svg import fit_material_opacity
from vista_menu_framing_svg import grid_material


SHELL='ui\\screens\\game_shell\\'
GLOWS={
 'ui\\global_bitmaps\\dialog1':(
    'M17 0 H512 V512 H1 V114 L6 108 V55 L0 50 V17 Z',
    ((120,0,320,160),(0,140,160,270))),
 'ui\\global_bitmaps\\dialog2':(
    'M0 0 H510 V381 L512 388 V489 L489 512 H0 Z',
    ((400,512,230,140),(512,400,150,240))),
 SHELL+'settings_screen\\variant_settings\\ul_glow':(
    'M17 0 H512 V512 H2 V114 L7 108 V55 L0 50 V17 Z',
    ((110,0,280,135),(0,110,105,245))),
 SHELL+'settings_screen\\variant_settings\\br_glow':(
    'M0 0 H285 V167 L278 163 V191 L285 198 V272 L261 296 H0 Z',
    ((205,296,210,110),(286,145,130,235))),
}


def pool(d,parent,key,cx,cy,rx,ry,colour='#3ea2ff',opacity=1):
    paint=element(d.defs,'radialGradient',id=key,gradientUnits='userSpaceOnUse',
                  cx=0,cy=0,r=1,gradientTransform=f'translate({cx} {cy}) scale({rx} {ry})')
    for offset,weight in ((0,1),(.25,.85),(.5,.33),(.75,.055),(1,0)):
        element(paint,'stop',offset=offset,stop_color=colour,stop_opacity=weight*opacity)
    element(parent,'ellipse',cx=cx,cy=cy,rx=rx,ry=ry,fill=f'url(#{key})')


def corner_light(d,name):
    d.family='dialog-corner-light'
    outline,fields=GLOWS[name]
    clip=element(d.defs,'clipPath',id='dialog-light-bounds');element(clip,'path',d=outline)
    group=element(d.root,'g',clip_path='url(#dialog-light-bounds)')
    washes=[]
    for i,field in enumerate(fields):
        wash=element(group,'g',id='light-wash-'+str(i));pool(d,wash,'light-pool-'+str(i),*field)
        washes.append(wash)
    fit_material_opacity(d,*washes)


def metal(d,parent,paths,lines=()):
    group=element(parent,'g',filter=d.glow('panel-metal-edge',.4),fill='#e5effb')
    for path in paths:
        element(group,'path',d=path,opacity=.68)
        element(group,'path',d=path,fill='none',stroke='white',stroke_width=.7,opacity=.16)
    for path,width in lines:
        element(group,'path',d=path,fill='none',stroke='#e5effb',stroke_width=width,opacity=.7)
    return group


def signin(d):
    d.family='four-way-signin-divider'
    light=element(d.root,'g',id='signin-light')
    for i,(path,field) in enumerate((
        ('M463 0 H1024 V386 H463 Z',(500,370,380,350)),
        ('M0 395 H458 V1024 H0 Z',(300,480,450,410)))):
        clip=element(d.defs,'clipPath',id='signin-quadrant-'+str(i));element(clip,'path',d=path)
        g=element(light,'g',clip_path=f'url(#signin-quadrant-{i})')
        pool(d,g,'signin-pool-'+str(i),*field,colour='#4395e2',opacity=.28)
    paths=('M450 250 L462 242 V300 L450 292 Z',
        'M450 365 L463 352 V418 H454 L449 412 V396 L440 387 H449 V380 L450 379 Z',
        'M459 606 L465 612 V652 L459 656 Z',
        'M332 386 H371 L368 390 H334 Z',
        'M380 386 H736 L732 391 H694 L690 388 H607 L602 395 H474 '
        'L470 399 H463 V391 H380 L376 393 H373 Z')
    face=metal(d,d.root,paths,(('M459 300 V654',1.5),))
    fit_material_opacity(d,light,face)


def news_frame(d):
    d.family='news-grid-panel'
    grid_material(d,1184,400,26,0,0,grid_opacity=.14)
    light=next(n for n in d.root if n.get('id')=='grid-material')
    paths=('M962 12 H1097 L1100 10 H1149 L1171 32 V63 L1167 67 '
           'V34 L1147 16 H1106 L1103 14 H962 Z',
           'M1164 126 L1170 132 V164 L1164 158 Z',
           'M10 266 L18 258 V284 L13 289 V358 L44 388 H180 L178 384 '
           'H49 L16 355 V294 L10 300 Z')
    face=metal(d,d.root,paths,(('M16 294 V340',1.5),))
    fit_material_opacity(d,light,face)


def lightpanels(d,name):
    if name in GLOWS:corner_light(d,name)
    elif name==SHELL+'4way_multiplayer_signin\\mp_signin':signin(d)
    elif name==SHELL+'bungie_news\\live_display':news_frame(d)
    elif name=='ui\\screens\\misc\\loading_screen\\sweep':
        d.family='loading-sweep-band'
        element(d.root,'rect',width=2048,height=512,fill='#181818')
        element(d.root,'rect',x=948,width=128,height=512,fill='white')
    else:return False
    return True
