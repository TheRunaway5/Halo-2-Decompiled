"""Authored Vista emblem outlines. Unsupported designs remain unfinished."""
import math
import numpy as np
from vista_prompt_svg import element, bounds, rgb
from vista_emblem_classic_svg import native_emblem


DIGITS={
 '0':'M32 0 H96 L128 32 V224 L96 256 H32 L0 224 V32 Z M52 48 V208 H76 V48 Z',
 '1':'M0 16 L64 0 H96 V256 H32 V48 H0 Z',
 '2':'M32 0 H96 L128 32 V104 L52 180 V208 H128 V256 H0 V144 L76 80 V48 H52 V80 H0 V32 Z',
 '3':'M32 0 H96 L128 32 V96 L96 128 L128 160 V224 L96 256 H32 L0 224 V176 H52 V208 H76 V152 H44 V104 H76 V48 H52 V80 H0 V32 Z',
 '4':'M80 0 H128 V160 H148 V208 H128 V256 H80 V208 H16 L0 176 Z M80 160 V80 L40 160 Z',
 '5':'M0 0 H116 V48 H48 V96 H84 L132 136 V216 L96 256 H32 L0 224 V176 H52 V208 H80 V152 H40 L0 120 Z',
 '6':'M32 0 H96 L128 32 V80 H76 V48 H52 V104 H96 L128 136 V224 L96 256 H32 L0 224 V32 Z M52 152 V208 H76 V152 Z',
 '7':'M0 0 H132 V48 L64 256 H8 L80 48 H48 V72 H0 Z',
 '8':'M32 0 H96 L128 32 V96 L96 128 L128 160 V224 L96 256 H32 L0 224 V160 L32 128 L0 96 V32 Z M52 48 V104 H76 V48 Z M52 152 V208 H76 V152 Z',
 '9':'M32 0 H96 L128 32 V224 L96 256 H32 L0 224 V176 H52 V208 H76 V152 H32 L0 120 V32 Z M52 48 V104 H76 V48 Z',
}


PATHS={
 4:'M96 0 H160 L202 18 L218 62 L236 72 L240 178 L160 256 H96 L16 178 L20 72 L38 62 L54 18 Z '
   'M38 92 C40 164 45 183 104 194 L128 204 L152 194 C211 183 216 164 218 92 Z '
   'M104 212 H152 L149 256 H107 Z',
 5:'M256 0 C264 69 246 97 209 126 L124 182 C135 215 114 241 84 256 '
   'C99 230 102 209 98 198 L69 213 L83 185 C60 180 32 188 0 198 '
   'C23 159 45 143 76 150 L148 41 C175 9 219 -3 256 0 Z',
 7:'M0 0 L128 154 L256 0 L128 256 Z',
 9:'M0 80 L128 0 L256 80 V112 L128 32 L0 112 Z M0 144 L128 64 L256 144 V176 L128 96 L0 176 Z',
 12:'M128 0 C52 0 0 42 0 100 C0 133 21 144 34 159 C25 176 22 187 34 201 '
    'L58 192 C76 201 82 221 83 239 C85 251 97 252 101 238 V224 H109 V247 '
    'C109 259 121 259 122 247 V224 H131 V247 C133 259 145 259 145 247 V224 H154 '
    'V239 C158 253 170 251 173 238 C175 217 180 202 197 192 L222 201 '
    'C234 187 231 176 222 159 C238 144 256 132 256 100 C256 42 204 0 128 0 Z '
    'M41 139 C49 119 65 116 82 131 L112 151 C105 172 78 172 52 165 C45 163 40 153 41 139 Z '
    'M215 139 C207 119 191 116 174 131 L144 151 C151 172 178 172 204 165 C211 163 216 153 215 139 Z '
    'M119 177 C112 186 110 205 119 207 C125 205 124 184 119 177 Z '
    'M137 177 C144 186 146 205 137 207 C131 205 132 184 137 177 Z',
 14:'M0 0 L115 64 V256 L0 192 Z M141 64 L256 0 V192 L141 256 Z',
 20:'M0 64 L56 8 L128 80 L200 8 L256 64 L128 192 Z',
 22:'M128 0 C87 10 8 27 8 89 V197 C14 230 40 256 86 256 V200 '
    'C59 191 42 159 27 118 L106 163 V238 L128 256 L150 238 V163 L229 118 '
    'C214 159 197 191 170 200 V256 C216 256 242 230 248 197 V89 C248 27 169 10 128 0 Z',
 23:'M64 0 L128 256 H0 Z M192 0 L256 256 H128 Z',
 25:'M20 0 H236 Q260 0 249 25 L145 240 Q128 274 111 240 L7 25 Q-4 0 20 0 Z',
 26:'M92 0 H204 L132 78 H177 L99 160 H138 L0 256 L46 176 H14 L62 82 H34 Z',
 28:'M80 0 H120 V144 L20 204 L0 168 L80 120 Z M136 0 H176 V120 L256 168 L236 204 L136 144 Z',
 32:'M128 0 C104 39 15 85 15 138 C15 188 64 208 110 183 C124 183 105 229 91 256 '
    'H165 C151 229 132 183 146 183 C192 208 241 188 241 138 C241 85 152 39 128 0 Z',
 35:'M128 48 C83 -20 0 -2 0 71 C0 133 105 210 128 256 C151 210 256 133 256 71 '
    'C256 -2 173 -20 128 48 Z',
 41:'M60 0 H196 C235 0 256 28 256 64 V80 C256 116 235 144 196 144 H60 '
    'C21 144 0 116 0 80 V64 C0 28 21 0 60 0 Z M60 16 C31 16 16 37 16 64 V80 '
    'C16 107 31 128 60 128 H128 V16 Z M152 18 H185 C196 18 196 38 185 38 H152 '
    'C141 38 141 18 152 18 Z M216 18 A10 10 0 1 1 216 38 A10 10 0 1 1 216 18 Z',
 47:'M122 0 H134 V8 H142 V20 H134 V104 Q134 126 157 126 H214 V121 H222 V147 H214 V142 '
    'H153 Q134 142 134 162 V244 L128 256 L122 244 V162 Q122 142 103 142 H42 V147 '
    'H34 V121 H42 V126 H99 Q122 126 122 104 V20 H114 V8 H122 Z',
 53:'M128 0 L155 64 L141 59 V256 H115 V59 L101 64 Z',
}


def circle_path(cx,cy,r):
    return f'M{cx-r} {cy} A{r} {r} 0 1 1 {cx+r} {cy} A{r} {r} 0 1 1 {cx-r} {cy} Z'


def foreground(d):
    corrected=native_emblem(d)
    if corrected is not None:return corrected
    i=d.frame
    supported=set(PATHS)|{1,3,6,8,11,13,15,16,17,18,21,24,27,29,30,31,33,34,42,43}|set(range(54,64))
    if i not in supported:return False
    d.family='emblem-foreground'
    x0,y0,x1,y1=bounds(d.source)
    blue=d.source[(d.source[:,:,3]>=128)&(d.source[:,:,2]>200)&(d.source[:,:,0]<50),:3]
    ink=rgb(np.median(blue,axis=0)) if len(blue) else '#0000fe'
    unitw,unith=256,256
    if i==9:unith=176
    elif i==20:unith=192
    elif i==28:unith=204
    elif i==41:unith=144
    elif i==53:unitw=54;x0,y0,x1,y1=bounds(d.source)
    elif i>=54:unitw={'1':96,'4':148,'5':132,'7':132}.get(str(i-54),128)
    g=element(d.root,'g',id='foreground',transform=f'translate({x0} {y0}) scale({(x1-x0)/unitw} {(y1-y0)/unith})',
              fill=ink,fill_rule='evenodd')
    if i in PATHS:
        path=PATHS[i]
        if i==53:
            # The shaft's authored unit width is narrow; translate its own
            # design coordinates rather than stretching a whole square.
            element(g,'path',d=path,transform='translate(-101 0)')
        else:element(g,'path',d=path)
    elif i>=54:element(g,'path',d=DIGITS[str(i-54)])
    elif i==1:element(g,'path',d=circle_path(128,128,128)+' '+circle_path(128,128,84))
    elif i==13:element(g,'circle',cx=128,cy=128,r=128)
    elif i==3:
        element(g,'path',d='M79 109 V28 C79 5 103 5 103 28 V107 H115 V15 C115 -5 139 -5 139 15 '
            'V107 H151 V28 C151 5 175 5 175 28 V120 H187 V57 C187 34 211 34 211 57 V164 '
            'C211 227 189 256 128 256 C73 256 57 236 37 205 L3 149 C-8 131 13 116 25 132 '
            'L60 172 C75 187 79 143 79 109 Z')
    elif i==6:
        for cx,cy in ((64,64),(192,64),(128,176)):
            pts=[(cx+64*math.cos(math.radians(a)),cy+64*math.sin(math.radians(a))) for a in (-90,-30,30,90,150,210)]
            element(g,'polygon',points=' '.join(f'{x},{y}' for x,y in pts),fill='none',stroke=ink,stroke_width=28)
    elif i==8:
        for angle in range(0,360,45):
            element(g,'path',d='M128 0 L181 22 L153 45 L128 91 L103 45 L75 22 Z',transform=f'rotate({angle} 128 128)')
    elif i==11:
        pts=[(128+128*(1 if n%2==0 else .381966)*math.cos(-math.pi/2+n*math.pi/5),
              128+128*(1 if n%2==0 else .381966)*math.sin(-math.pi/2+n*math.pi/5)) for n in range(10)]
        element(g,'polygon',points=' '.join(f'{x},{y}' for x,y in pts))
        for n in range(0,10,2):
            a,b=pts[n],pts[(n+1)%10]
            element(g,'polygon',points=f'128,128 {a[0]},{a[1]} {b[0]},{b[1]}',fill='#ffffff')
    elif i==15:
        for a in (-60,60,180):
            p=[]
            for r,ang in ((128,a),(128,a+60),(32,a+60),(32,a)):
                p.append((128+r*math.cos(math.radians(ang)),128+r*math.sin(math.radians(ang))))
            element(g,'path',d=f'M{p[0][0]} {p[0][1]} A128 128 0 0 1 {p[1][0]} {p[1][1]} '
                f'L{p[2][0]} {p[2][1]} A32 32 0 0 0 {p[3][0]} {p[3][1]} Z')
    elif i in (16,17):
        for x in (88,168):element(g,'ellipse',cx=x,cy=64,rx=18,ry=26)
        smile='M9 147 C61 247 195 247 247 147 L231 146 C179 222 77 222 25 146 Z '
        hooks='M9 147 C4 126 19 114 36 106 L47 114 C28 121 20 130 9 147 Z '
        hooks+='M247 147 C252 126 237 114 220 106 L209 114 C228 121 236 130 247 147 Z'
        element(g,'path',d=smile+hooks,transform='translate(0 362) scale(1 -1)' if i==17 else 'translate(0 0)')
    elif i==18:
        path=('M103 0 H153 L145 12 H153 L145 24 H150 L135 84 H144 Q149 84 145 91 '
              'L133 112 Q128 121 123 112 L111 91 Q107 84 112 84 H121 L106 24 H111 L103 12 H111 Z')
        for a in (0,120,240):element(g,'path',d=path,transform=f'rotate({a} 128 128)')
    elif i==21:
        element(g,'circle',cx=128,cy=128,r=124,fill='none',stroke=ink,stroke_width=8)
        element(g,'path',d='M0 128 A128 128 0 0 0 256 128 A64 64 0 0 1 128 128 A64 64 0 0 0 0 128 Z '+circle_path(64,128,18))
        element(g,'circle',cx=192,cy=128,r=18)
    elif i==24:
        for a in range(0,360,60):
            element(g,'path',d='M128 128 C72 21 119 0 163 10 C168 13 182 20 200 28 '
                'C153 37 127 72 128 128 Z',transform=f'rotate({a} 128 128)')
    elif i==27:
        element(g,'path',d='M44 232 C59 210 197 210 212 232 L232 133 C235 115 226 108 219 122 '
            'C211 146 209 195 190 193 C175 191 180 133 183 116 C187 100 173 100 171 116 '
            'C168 150 159 190 144 187 C128 184 134 123 136 105 C146 96 110 96 120 105 '
            'C122 130 127 184 110 187 C95 191 90 150 90 116 C90 100 76 100 75 116 '
            'C78 134 79 193 61 193 C44 195 43 146 36 122 C29 108 20 115 23 133 Z')
        for x,y,r in ((128,72,12),(65,99,8),(191,99,8),(25,132,7),(231,132,7)):
            element(g,'circle',cx=x,cy=y,r=r)
    elif i==29:
        element(g,'path',d='M0 128 A128 128 0 0 1 256 128 H210 A82 82 0 0 0 46 128 Z')
        element(g,'circle',cx=128,cy=190,r=44)
    elif i==30:
        element(g,'path',d='M128 0 C48 0 8 44 8 108 L4 143 L16 208 C40 237 90 248 128 256 '
            'C166 248 216 237 240 208 L252 143 L248 108 C248 44 208 0 128 0 Z '
            'M18 113 H238 C223 183 184 207 128 207 C72 207 33 183 18 113 Z')
        element(g,'path',d='M40 128 L113 151 C81 178 58 160 40 128 Z M216 128 L143 151 C175 178 198 160 216 128 Z')
    elif i==31:
        mask=element(d.defs,'mask',id='moon-cutout',maskUnits='userSpaceOnUse',x=0,y=0,width=256,height=256)
        element(mask,'circle',cx=128,cy=128,r=128,fill='white')
        element(mask,'circle',cx=62,cy=83,r=120,fill='black')
        element(g,'rect',width=256,height=256,mask='url(#moon-cutout)')
    elif i==33:
        element(g,'path',d=circle_path(128,51,51)+' '+circle_path(64,139,55)+' '+circle_path(192,139,55),fill_rule='nonzero')
        element(g,'path',d='M112 152 C128 183 104 229 91 256 H165 C152 229 128 183 144 152 Z')
    elif i==34:element(g,'path',d='M128 0 L256 128 L128 256 L0 128 Z')
    elif i==42:
        element(g,'path',d=circle_path(128,128,128)+' '+circle_path(128,128,104))
        element(g,'path',d='M133 10 H157 L99 123 L144 148 L88 246 H62 L112 154 L76 129 Z')
        element(g,'circle',cx=167,cy=102,r=33)
    elif i==43:
        element(g,'path',d='M128 0 C47 0 10 53 19 119 C25 155 43 175 68 192 H188 '
            'C213 175 231 155 237 119 C246 53 209 0 128 0 Z '
            'M38 73 A27 35 -30 1 1 86 109 A27 35 -30 1 1 38 73 Z '
            'M218 73 A27 35 30 1 0 170 109 A27 35 30 1 0 218 73 Z '
            'M91 174 V158 C91 109 165 109 165 158 V174 H157 V158 C157 120 99 120 99 158 V174 Z')
        element(g,'ellipse',cx=128,cy=193,rx=70,ry=23)
        path='M58 211 A70 23 0 1 1 198 211 A70 23 0 1 1 58 211 Z '
        for x,y in ((101,208),(128,205),(155,208),(101,218),(128,221),(155,218)):
            path+=f'M{x-8} {y} A8 4 0 1 1 {x+8} {y} A8 4 0 1 1 {x-8} {y} Z '
        element(g,'path',d=path)
    return True
