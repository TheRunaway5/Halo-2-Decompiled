"""Native-canvas Vista browser, profile and lobby frame geometry."""
from vista_prompt_svg import element


PREFIX='ui\\screens\\game_shell\\'


def grid_material(d,width,height,period,phase_x,phase_y,profile=False,grid_opacity=.14):
    group=element(d.root,'g',id='grid-material')
    clip=element(d.defs,'clipPath',id='grid-material-bounds')
    element(clip,'rect',x=12,y=12,width=width-24,height=height-18)
    group.set('clip-path','url(#grid-material-bounds)')
    # Two soft pools of the original blue, with a separately fading grid.
    positions=((50,50,width*.66,height*.6,.20),(width-30,height-40,width*.48,height*.5,.20)) if profile else \
              ((width-50,height*.35,width*.55,height*.75,.24),(30,height-35,width*.4,height*.55,.26))
    for i,(cx,cy,rx,ry,opacity) in enumerate(positions):
        paint=d.radial('panel-blue-'+str(i),(65,149,226),cx,cy,rx,ry)
        element(group,'ellipse',cx=cx,cy=cy,rx=rx,ry=ry,fill=paint,fill_opacity=opacity)
    pattern=element(d.defs,'pattern',id='panel-grid',patternUnits='userSpaceOnUse',
                    width=period,height=period,x=phase_x,y=phase_y,overflow='visible')
    element(pattern,'path',d=f'M0 {period} V0 H{period}',fill='none',stroke='white',stroke_width=2)
    fade=element(d.defs,'radialGradient',id='grid-visibility',gradientUnits='userSpaceOnUse',
                 cx=0,cy=0,r=1,gradientTransform=f'translate({width/2} {height/2}) scale({width*.57} {height*.64})')
    for offset,alpha in ((0,1),(.55,.95),(1,0)):
        element(fade,'stop',offset=offset,stop_color='white',stop_opacity=alpha)
    mask=element(d.defs,'mask',id='grid-visibility-mask',maskUnits='userSpaceOnUse',x=0,y=0,width=width,height=height)
    element(mask,'rect',width=width,height=height,fill='url(#grid-visibility)')
    element(group,'rect',width=width,height=height,fill='url(#panel-grid)',opacity=grid_opacity,
            mask='url(#grid-visibility-mask)')


def chrome(d,paths,lines=(),opacity=.6):
    group=element(d.root,'g',id='chrome-frame')
    stops=[(0,(205,226,246)),(.5,(173,195,216)),(1,(173,195,216))]
    if opacity>.8:stops=[(0,(255,255,255)),(1,(255,255,255))]
    material=d.linear('frame-metal',stops,
                      y1=0,y2=d.height)
    glow=d.glow('frame-blue-light',3 if d.width<1100 else 5)
    for path in paths:
        element(group,'path',d=path,fill='#4195e2',stroke='#4195e2',stroke_width=4,
                opacity=.35,filter=glow,fill_rule='evenodd')
        element(group,'path',d=path,fill=material,fill_rule='evenodd',opacity=opacity)
    for path,width in lines:
        element(group,'path',d=path,fill='none',stroke='#4195e2',stroke_width=width+4,
                opacity=.35,filter=glow)
        element(group,'path',d=path,fill='none',stroke=material,stroke_width=width,opacity=opacity)


def help_panel(d,name):
    d.family='browser-grid-panel'
    live=name.endswith('live_help_display')
    right,bottom=(732,229) if live else (496,206)
    grid_material(d,right,bottom,24.45,.5,12,grid_opacity=.16 if live else .18)
    shift=bottom-206
    paths=(f'M9 {91+shift} L18 {82+shift} V{106+shift} L12 {112+shift} V{174+shift} '
           f'L44 {201+shift} H168 V{205+shift} H33 L9 {181+shift} Z',
           f'M{right-197} 11.5 H{right-68} L{right-66} 9.5 H{right-20} L{right} 29.5 '
           f'V61 L{right-4} 65 V33 L{right-19} 17 H{right-61} L{right-64} 13.5 H{right-197} Z',
           f'M{right-7} {117+shift} L{right-1} {123+shift} V{156+shift} L{right-7} {150+shift} Z')
    chrome(d,paths,((f'M15 {115+shift} V{158+shift}',1.3),))


def profile_panel(d):
    d.family='profile-grid-panel'
    grid_material(d,528,340,26,24.5,20.5,profile=True,grid_opacity=.13)
    paths=('M12 36 L31 16 H83 L80 21 H222 L220 24 H31 L16 39 V69 L12 65 Z',
           'M14 132 L20 127 V111 H24 V145 L18 150 V225 L14 230 V151 L24 142 V129 Z',
           'M518 205 L528 215 V305 L505 340 H409 L410 337 H492 L524 305 V238 L518 233 Z')
    chrome(d,paths,(('M20 42 V128',1.5),('M521 244 V286',1.8)))


def lobby_grid(d):
    d.family='lobby-grid-panel'
    grid_material(d,710,326,26,13,13,grid_opacity=.12)
    paths=('M10 202 L18 194 V218 L13 223 V294 L44 321 H180 V325 H32 L10 302 Z',
           'M500 12 H637 L639 10 H688 L710 32 V63 L706 66 V34 L687 16 H644 L642 14 H500 Z',
           'M703 127 L709 133 V165 L703 159 Z')
    chrome(d,paths,(('M16 228 V281',1.5),))


def browser_frame(d):
    d.family='browser-list-frame'
    paths=('M40 120 L52 108 H100 L110 118 H74 L67 125 V148 Z',
           'M1384 118 L1394 108 H1462 L1500 146 L1480 166 V148 L1454 122 H1384 Z',
           'M67 596 L39 624 V674 L47 682 V760 L67 782 L100 750 H264 L269 744 '
           'H75 L67 735 Z')
    chrome(d,paths,(('M67 735 V126 M76 120 H1385 M100 747 H264',4),))


def chat_panel(d):
    d.family='lobby-chat-frame'
    # The panel is open at the upper right; the bottom rail and its two
    # small feet are independent from the large content opening.
    group=element(d.root,'g',id='chat-background')
    clip=element(d.defs,'clipPath',id='chat-content')
    element(clip,'path',d='M14 38 L32 18 H634 V247 L620 260 H32 L14 247 Z')
    group.set('clip-path','url(#chat-content)')
    for i,(cx,cy,rx,ry) in enumerate(((22,0,430,120),(636,260,400,110))):
        paint=d.radial('chat-blue-'+str(i),(65,149,226),cx,cy,rx,ry)
        element(group,'ellipse',cx=cx,cy=cy,rx=rx,ry=ry,fill=paint,
                fill_opacity=(.14,.30,.20)[d.frame])
    paths=('M12 37 L32 16 H636 L629 23 H584 L577 19 H33 L16 39 V70 L12 66 Z',
           'M14 247 L32 259 H620 L634 246 V222 L630 217 V199 L636 205 V265 '
           'L602 299 H554 L550 295 H88 L84 299 H48 L14 265 Z '
           'M36 262 H600 V286 Q600 289 597 289 H39 Q36 289 36 286 Z')
    chrome(d,paths,(('M14 53 V247 M32 260 H620',1.5),),opacity=(.5,.85,.62)[d.frame])


def framing(d,name):
    if name in (PREFIX+'network_squad_browser_screen\\help_display',PREFIX+'network_squad_browser_screen\\live_help_display'):
        help_panel(d,name)
    elif name in (PREFIX+'network_squad_browser_screen\\list_frame',PREFIX+'network_squad_browser_screen\\live_list_frame'):
        browser_frame(d)
    elif name in (PREFIX+'settings_screen\\player_profile\\display',PREFIX+'settings_screen\\variant_settings\\variant_display'):
        profile_panel(d)
    elif name==PREFIX+'pregame_lobby\\lobby_display':lobby_grid(d)
    elif name==PREFIX+'pregame_lobby\\chat_display2':chat_panel(d)
    else:return False
    return True
