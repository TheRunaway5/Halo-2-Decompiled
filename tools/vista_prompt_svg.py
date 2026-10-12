"""Hand-built Vista controls: ordinary SVG shapes, lettering and gradients.

Geometry is authored below by control family. The reference is used to measure
placement and calibrate a small set of gradient colors; it is never traced into
pixel/color regions. No image, font, bitmap grid, or custom renderer is embedded.
"""
import io
import subprocess
import xml.etree.ElementTree as ET

import numpy as np
from PIL import Image
from scipy.optimize import lsq_linear, minimize

SVG = 'http://www.w3.org/2000/svg'
ET.register_namespace('', SVG)


def element(parent, name, **attributes):
    return ET.SubElement(parent, '{'+SVG+'}'+name,
                         {key.replace('_', '-'): str(value) for key, value in attributes.items()})


def rgb(color):
    return '#'+''.join(f'{int(v):02x}' for v in np.clip(np.rint(color), 0, 255))


def render(path, scale=4):
    root = ET.parse(path).getroot()
    _, _, width, height = map(float, root.attrib['viewBox'].split())
    data = subprocess.check_output(['rsvg-convert', '--width', str(round(width*scale)),
        '--height', str(round(height*scale)), str(path)])
    return Image.open(io.BytesIO(data)).convert('RGBA')


# These are authored outlines, in the 32 x 36 Vista face-button design space.
# The A has its triangular counter; B has two independently editable counters.
LETTERS = {
 'A': 'M12.5 10 H19.1 L25.8 28 H21 L19.9 25.6 H10.7 L9.6 28 H5.9 Z '
      'M15.7 14.7 L12.9 22 H18.6 Z',
 'B': 'M7.2 10 H19.7 C23.6 10 26 12.3 26 15.6 C26 17.5 24.7 19 22.7 19.8 '
      'C25 20.6 26.8 22.2 26.8 24.5 C26.8 27.2 23.5 28.7 20.1 28.7 H7.2 Z '
      'M11.2 13.9 V17.9 H19.7 C21.3 17.9 21.5 16.9 21.5 15.9 '
      'C21.5 14.8 20.9 13.9 19.7 13.9 Z '
      'M11.2 21.8 V26.1 H20 C21.5 26.1 22 25.2 22 24 C22 22.8 21.4 21.8 20 21.8 Z',
 'X': 'M8 11.2 H12.7 L16.3 16.9 L20.8 11.2 H25.2 L18.9 19.6 '
      'L25.2 27.8 H20.6 L16.2 22.2 L12 27.8 H7.6 L13.7 19.6 Z',
 'Y': 'M5.9 12.3 H11.2 L16.4 19.4 L21.4 12.3 H26.1 L18.9 22.4 '
      'V29 H14 V22.4 Z',
 # Shoulder labels use their own narrow, straight-sided Vista letterforms.
 'L': 'M0 0 H1.5 V8.5 H6 V10 H0 Z',
 'R': 'M0 0 H4.4 C6.5 0 7.2 1.2 7.2 3 C7.2 4.4 6.5 5.3 5.2 5.5 '
      'L7.7 10 H5.9 L3.5 5.8 H1.5 V10 H0 Z '
      'M1.5 1.4 V4.4 H4.1 C5.3 4.4 5.7 4 5.7 2.9 C5.7 1.9 5.2 1.4 4.1 1.4 Z',
 'small-B': 'M0 0 H4 C6.2 0 7 1 7 2.6 C7 3.7 6.4 4.5 5.5 4.8 '
      'C6.8 5.1 7.5 6 7.5 7.3 C7.5 9.1 6.4 10 4.2 10 H0 Z '
      'M1.5 1.4 V4.2 H3.8 C5 4.2 5.5 3.8 5.5 2.8 C5.5 1.9 5 1.4 3.8 1.4 Z '
      'M1.5 5.6 V8.6 H4 C5.4 8.6 6 8.1 6 7.1 C6 6 5.4 5.6 4 5.6 Z',
}


class Drawing:
    def __init__(self, image, code, path, scale, name):
        self.source = np.array(image.convert('RGBA'))
        self.height, self.width = self.source.shape[:2]
        self.path = path
        self.scale = scale
        self.root = ET.Element('{'+SVG+'}svg', dict(width=str(self.width*scale),
            height=str(self.height*scale), viewBox=f'0 0 {self.width} {self.height}', version='1.1'))
        element(self.root, 'title').text = f'Vista {name} U+{code:04X}: rebuilt vector artwork'
        element(self.root, 'desc').text = ('Hand-authored button bodies, bevels, letter outlines and '
            'smooth gradients. Original glyphs supply placement and palette measurements only. '
            'No embedded image, font, autotrace or pixel-region paths.')
        self.defs = element(self.root, 'defs')
        self.slots = []
        self.family = None

    def group(self, width, height, **attributes):
        return element(self.root, 'g', transform=f'scale({self.width/width:.8f} {self.height/height:.8f})',
                       **attributes)

    def slot(self, target, attribute, color):
        self.slots.append((target, attribute, np.array(color, float)))
        target.set(attribute, rgb(color))

    def linear(self, name, stops, y1=0, y2=36, x1=0, x2=0, calibrate=True):
        gradient = element(self.defs, 'linearGradient', id=name, gradientUnits='userSpaceOnUse',
            x1=x1, x2=x2, y1=y1, y2=y2)
        for offset, color in stops:
            stop = element(gradient, 'stop', offset=offset)
            if calibrate:self.slot(stop, 'stop-color', color)
            else:stop.set('stop-color',rgb(color))
        return f'url(#{name})'

    def solid(self, node, color, attribute='fill'):
        self.slot(node, attribute, color)
        return node

    def radial(self, name, color, cx, cy, rx, ry):
        gradient = element(self.defs, 'radialGradient', id=name, gradientUnits='userSpaceOnUse',
            cx=0, cy=0, r=1, gradientTransform=f'translate({cx} {cy}) scale({rx} {ry})')
        for offset, opacity in ((0, 1), (.35, .9), (1, 0)):
            stop = element(gradient, 'stop', offset=offset, stop_opacity=opacity)
            # One paint color is shared across the three opacity stops.
            stop.set('stop-color', rgb(color))
        self.slots.append((tuple(gradient), 'stop-color', np.array(color, float)))
        return f'url(#{name})'

    def radial_solid(self, name, stops, cx, cy, rx, ry):
        gradient=element(self.defs,'radialGradient',id=name,gradientUnits='userSpaceOnUse',
            cx=0,cy=0,r=1,gradientTransform=f'translate({cx} {cy}) scale({rx} {ry})')
        for offset,color in stops:
            stop=element(gradient,'stop',offset=offset)
            self.slot(stop,'stop-color',color)
        return f'url(#{name})'

    @staticmethod
    def set_slot(slot, color):
        targets, attribute, _ = slot
        for target in targets if isinstance(targets, tuple) else (targets,):
            target.set(attribute, rgb(color))

    def save(self):
        self.path.parent.mkdir(parents=True, exist_ok=True)
        ET.indent(self.root)
        ET.ElementTree(self.root).write(self.path, encoding='utf-8', xml_declaration=True)

    def calibrate(self):
        """Fit only the small authored paint palette; geometry stays untouched.

        Ordinary SVG sRGB gradients are linear in their stop colors. Render
        each paint's influence, then fit bounded RGB anchor values to the
        original. A low-dimensional paint model cannot encode a pixel copy.
        """
        if not self.slots:
            return
        for slot in self.slots:
            self.set_slot(slot, (0, 0, 0))
        self.save()
        def averaged():
            rgba=np.array(render(self.path,self.scale))
            rgba[rgba[:,:,3]==0,:3]=255
            return rgba.reshape(self.height,self.scale,self.width,self.scale,4).mean((1,3))
        base=averaged()
        columns = []
        for slot in self.slots:
            self.set_slot(slot, (255, 255, 255))
            self.save()
            columns.append((averaged()[:,:,0]-base[:,:,0])/255)
            self.set_slot(slot, (0, 0, 0))
        matrix = np.stack(columns, axis=-1)
        visible = (self.source[:, :, 3] >= 128) & (base[:, :, 3] >= 128)
        weight = np.sqrt(self.source[:, :, 3][visible]/255.)
        model = matrix[visible]*weight[:, None]
        # Palette regularization prevents unsupported edge paints from
        # acquiring unrelated hues solely to compensate for outline rounding.
        ridge = .3
        model = np.vstack((model, np.eye(len(self.slots))*ridge))
        initial = np.array([slot[2] for slot in self.slots])
        colors = []
        for channel in range(3):
            target = (self.source[:, :, channel][visible]-base[:, :, channel][visible])*weight
            target = np.r_[target, initial[:, channel]*ridge]
            colors.append(lsq_linear(model, target, bounds=(0, 255), tol=1e-6).x)
        for slot, color in zip(self.slots, np.array(colors).T):
            self.set_slot(slot, color)


def face(d, code):
    d.family = 'face-button'
    unit=min(d.width/32,d.height/36)
    x=(d.width-32*unit)/2;y=d.height-36*unit
    g=element(d.root,'g',id='face-button',transform=f'translate({x} {y}) scale({unit})')
    clip = element(d.defs, 'clipPath', id='button-edge')
    element(clip, 'circle', cx=16, cy=20, r=15.95)
    g.set('clip-path', 'url(#button-edge)')
    index = code-0xe100
    hues = [(51, 136, 51), (136, 51, 51), (34, 68, 136), (187, 119, 0)]
    hue = np.array(hues[index])
    body = d.linear('button-body-paint', [(0, hue*.7), (.22, hue), (.52, hue),
        (.78, hue*1.15), (1, hue*1.32)], y1=4, y2=36)
    rim = d.linear('button-rim-paint', [(0, (150, 150, 150)), (.4, hue*.7), (1, hue)], y1=4, y2=36)
    element(g, 'circle', id='button-body', cx=16, cy=20, r=15.7,
        fill=body, stroke=rim, stroke_width='.5')
    gloss = d.radial('top-sheen-paint', (215, 215, 215), 15.4, 6.5, 11.9, 6.2)
    element(g, 'ellipse', id='top-sheen', cx=15.4, cy=6.5, rx=11.9, ry=6.2, fill=gloss)
    left = d.radial('left-reflection-paint', hue*1.8, 4.9, 17, 2.4, 8.5)
    element(g, 'ellipse', id='left-reflection', cx=4.9, cy=17, rx=2.4, ry=8.5, fill=left)
    letter = LETTERS['ABXY'[index]]
    shadow = d.linear('letter-shadow-paint', [(0, hue*.6), (1, hue*.55)], y1=10, y2=30)
    element(g, 'path', id='letter-raised-shadow', d=letter, fill=shadow, stroke=shadow,
        stroke_width='1.65', stroke_linejoin='round', fill_rule='evenodd', transform='translate(.3 .7)')
    bevel = d.linear('letter-bevel-paint', [(0, hue*.9+80), (1, hue*1.5)], y1=10, y2=30)
    element(g, 'path', id='letter-bevel', d=letter, fill=bevel, stroke=bevel,
        stroke_width='.7', stroke_linejoin='round', fill_rule='evenodd')
    ink = hue*.3+190
    ink[index if index < 3 else 0] = 255
    paint = d.linear('letter-face-paint', [(0, ink+20), (.5, ink), (1, ink)], y1=10, y2=29)
    element(g, 'path', id='letter-face', d=letter, fill=paint, fill_rule='evenodd')


def bounds(source, threshold=128):
    yy, xx = np.where(source[:, :, 3] >= threshold)
    if not len(xx):
        yy, xx = np.where(source[:, :, 3] > 0)
    return float(xx.min()), float(yy.min()), float(xx.max()+1), float(yy.max()+1)


def arrow(d, code):
    d.family = 'direction-arrow'
    x0, y0, x1, y1 = bounds(d.source)
    # Three vertices, fitted to the authored extent; no raster contour tracing.
    vertices = {
        0xe108: [(x0, y1), ((x0+x1)/2, y0), (x1, y1)],
        0xe109: [(x0, y0), (x1, y0), ((x0+x1)/2, y1)],
        0xe10a: [(x1, y0), (x0, (y0+y1)/2), (x1, y1)],
        0xe10b: [(x0, y0), (x1, (y0+y1)/2), (x0, y1)],
    }[code]
    # Calibrate only this triangle's position and scale against native edge
    # coverage. It remains three vertices, regardless of reference resolution.
    vertices=np.array(vertices,float)
    center=vertices.mean(0)
    yy,xx=np.mgrid[:d.height*8,:d.width*8]
    xx=(xx+.5)/8;yy=(yy+.5)/8
    target=d.source[:,:,3]/255.
    def transformed(p):return (vertices-center)*p[2:]+center+p[:2]
    def error(p):
        triangle=transformed(p)
        signs=[]
        for a,b in zip(triangle,np.roll(triangle,-1,axis=0)):
            signs.append((xx-a[0])*(b[1]-a[1])-(yy-a[1])*(b[0]-a[0]))
        stack=np.stack(signs)
        inside=(stack>=0).all(0)|(stack<=0).all(0)
        coverage=inside.reshape(d.height,8,d.width,8).mean((1,3))
        return float(((coverage-target)**2).mean())
    fit=minimize(error,[0,0,1,1],method='Powell',bounds=[(-1.25,1.25),(-1.25,1.25),(.85,1.15),(.85,1.15)],
        options=dict(xtol=.005,ftol=.00001,maxiter=30))
    vertices=transformed(fit.x)
    color=np.median(d.source[d.source[:,:,3]>=128,:3],axis=0)
    element(d.root, 'polygon', id='arrow', points=' '.join(f'{x},{y}' for x,y in vertices), fill=rgb(color))


def shoulder(d, code):
    d.family = 'shoulder-button'
    g = d.group(60, 36, id='shoulder-button')
    mirrored = code == 0xe105
    body = element(g, 'g', id='body', transform='translate(60 0) scale(-1 1)' if mirrored else 'translate(0 0)')
    outer = 'M5 13 C15 13 49 11.5 58.5 8 V27.8 H6 C1 27.8 1 25.5 1 21 C1 16 1.5 13 5 13 Z'
    rim = d.linear('shoulder-rim', [(0,(68,119,153)), (1,(68,119,153))], y1=10,y2=31)
    element(body, 'path', id='outer-bevel', d=outer, fill=rim)
    paint = d.linear('shoulder-face', [(0,(85,136,187)), (1,(85,136,187))], y1=15,y2=29)
    element(body, 'path', id='button-face', d='M5 14.5 H56.8 V25.6 H5 C3 25.6 2.7 24.7 2.7 21 '
        'V19 C2.7 16 3 14.5 5 14.5 Z', fill=paint)
    label = element(g, 'g', id='label', transform='translate(33 14.2) scale(1 .85)' if not mirrored else 'translate(12.8 14.2) scale(1 .85)')
    label_paint = d.linear('label-paint', [(0,(51,85,102)), (1,(34,68,85))], y1=0,y2=10,calibrate=False)
    element(label, 'path', id='letter-L' if not mirrored else 'letter-R',
        d=LETTERS['R' if mirrored else 'L'], fill=label_paint, fill_rule='evenodd')
    element(label, 'path', id='letter-B', d=LETTERS['small-B'], fill=label_paint,
        fill_rule='evenodd', transform='translate(7 0)')


TRIGGER = ('M8 .8 C15 -.2 37 5 42.5 13 C47 22 31 37 18 37 '
           'L8 36.5 C.5 36.5 -.5 33 1.8 27 L9 9 C10.2 6 7.5 6 6 3.5 '
           'C4.5 1.5 6 .8 8 .8 Z')


def material(d, name, color='white'):
    """One ordinary vector luminance mask prevents translucent overlap buildup."""
    mask=element(d.defs,'mask',id=name+'-opacity',maskUnits='userSpaceOnUse',
        x=0,y=0,width=d.width,height=d.height)
    element(mask,'rect',width=d.width,height=d.height,fill='black')
    element(d.root,'rect',id=name+'-paint',width=d.width,height=d.height,
        fill=color,mask=f'url(#{name}-opacity)')
    return mask


def trigger(d, code):
    d.family = 'trigger-pair'
    mask=material(d,'triggers')
    g=element(mask,'g',transform=f'scale({d.width/90} {d.height/38})')
    for side in range(2):
        active=side==code-0xe106
        group=element(g,'g',id=('left' if side==0 else 'right')+'-trigger',
            transform='translate(90 0) scale(-1 1)' if side else 'translate(0 0)')
        body=136 if active else 51
        edge=255 if active else 68
        stem='M9 7 C14 9 21 12 21 17 C22 24 22 31 20 36.8 L8 36.5 C.5 36.5 -.5 33 1.8 27 Z'
        element(group,'path',id='shell',d=TRIGGER,fill=rgb([body]*3))
        element(group,'path',id='stem-panel',d=stem,fill=rgb([119 if active else 51]*3))
        element(group,'path',id='outer-edge',d=TRIGGER,fill='none',
            stroke=rgb([edge]*3),stroke_width='.85',stroke_linejoin='round')
        element(group,'path',id='fold-highlight',d='M9 7 C14 9 21 12 21 17 C22 24 22 31 20 36.8',
            fill='none',stroke=rgb([edge]*3),stroke_width='.9')


def start_back(d, code, legacy=False):
    d.family = 'legacy-start-back' if legacy else 'start-back-button'
    if legacy:
        legacy_sphere(d, code)
        return
    g = d.group(33, 36, id='start-back-button')
    body = d.linear('button-body', [(0,(255,255,255)), (.55,(238,238,238)), (1,(190,190,190))],y1=8,y2=34)
    rim = d.linear('button-rim', [(0,(215,215,215)), (1,(155,155,155))],y1=8,y2=34)
    outline=('M11.4 9.5 H20.5 C27.1 9.5 30.5 13.8 30.5 20.4 V23 '
        'C30.5 29.8 26.8 33.1 20.5 33.1 H11.4 C4.5 33.1 1.2 29.1 1.2 23 '
        'V20.4 C1.2 13.7 4.6 9.5 11.4 9.5 Z')
    element(g,'path',id='rounded-body',d=outline,fill=body,stroke=rim,stroke_width='.65')
    sheen=d.radial('white-sheen',(255,255,255),7,13,13,12)
    element(g,'path',id='sheen',d='M11.4 9.8 H20 V14 C17 17 15 19 14 21 '
        'L8 22 H1.6 V20.4 C1.6 14 5 9.8 11.4 9.8 Z',fill=sheen)
    symbol='M11.8 14.2 L23.1 21.2 L11.8 28 Z'
    if code==0xe10d:symbol='M21 14.2 L9.7 21.2 L21 28 Z'
    ink=d.linear('arrow-paint',[(0,(204,204,204)),(1,(204,204,204))],y1=14,y2=28,calibrate=False)
    edge=d.linear('arrow-edge',[(0,(187,187,187)),(1,(170,170,170))],y1=14,y2=28,calibrate=False)
    element(g,'path',id='arrow',d=symbol,fill=ink,stroke=edge,stroke_width='.8',stroke_linejoin='miter')


def sticks(d, code):
    d.family = 'stick-pair'
    g=material(d,'sticks')
    active_side=0 if code in (0xe10e,0xe110,0xe12c,0xe12d,0xe12e) else 1
    for side in range(2):
        a=side*d.width//2;b=(side+1)*d.width//2
        region=d.source[:,a:b]
        x0,y0,x1,y1=bounds(region,34)
        x0+=a;x1+=a
        # The cap is an authored perspective ellipse, 21 units high in the
        # 38-unit design. Pose changes move it; they never stretch the cap
        # down into the shaft. Only its position and width are measured.
        unit=d.height/38
        cx=(x0+x1)/2;rx=(x1-x0)/2-.25
        top=y0+.3;bottom=top+21*unit
        cy=(top+bottom)/2;ry=(bottom-top)/2
        active=side==active_side
        body=136 if active else 51
        edge=187 if active else 68
        shaft_width=(x1-x0)*.49
        left=cx-shaft_width/2;right=cx+shaft_width/2
        shaft_top=cy+ry*.35;shaft_bottom=y1-.35
        rounded=min(8*unit,(shaft_bottom-shaft_top)*.65)
        group=element(g,'g',id=('left' if side==0 else 'right')+'-stick')
        shaft=(f'M{left} {shaft_top} H{right} V{shaft_bottom-rounded} '
            f'Q{right} {shaft_bottom} {cx} {shaft_bottom} '
            f'Q{left} {shaft_bottom} {left} {shaft_bottom-rounded} Z')
        element(group,'path',id='shaft',d=shaft,fill=rgb([body]*3))
        rim=(f'M{left} {cy+ry*.87} V{shaft_bottom-rounded} '
            f'Q{left} {shaft_bottom} {cx} {shaft_bottom} '
            f'Q{right} {shaft_bottom} {right} {shaft_bottom-rounded} V{cy+ry*.87}')
        element(group,'path',id='shaft-rim',d=rim,fill='none',stroke=rgb([edge]*3),stroke_width=.8*unit)
        element(group,'ellipse',id='cap',cx=cx,cy=cy,rx=rx,ry=ry,fill=rgb([body]*3),
            stroke=rgb([edge]*3),stroke_width=.9*unit)
        element(group,'ellipse',id='cap-inset',cx=cx,cy=cy-ry*.24,rx=rx*.71,ry=ry*.58,
            fill='none',stroke=rgb([204 if active else 68]*3),stroke_width=1.05*unit)
        if active:
            neck=(f'M{left+unit} {cy+ry-.1*unit} H{right-unit} '
                f'L{right-2*unit} {cy+ry+1.7*unit} '
                f'Q{cx} {cy+ry+2.4*unit} {left+2*unit} {cy+ry+1.7*unit} Z')
            element(group,'path',id='neck-shadow',d=neck,fill=rgb([119]*3))
            element(group,'path',id='neck-crease',d=f'M{left+2*unit} {cy+ry+1.1*unit} '
                f'Q{cx} {cy+ry+2.8*unit} {right-2*unit} {cy+ry+1.1*unit}',
                fill='none',stroke=rgb([68]*3),stroke_width=.9*unit)


def key_frame(d, code):
    d.family = 'keyboard-key-frame'
    w,h=d.width,d.height
    mask=material(d,'key','#6699cc')
    element(mask,'rect',id='key-face',width=w,height=h,fill=rgb([51]*3))
    unit=h/38
    top=7*unit;left=1.8*unit;right=6*unit;bottom=2*unit
    facets=(('top',f'M0 0 H{w} L{w-right} {top} H{top} Z',68),
        ('left',f'M0 0 H{left} V{h} H0 Z',68),
        ('right',f'M{w} 0 V{h} L{w-right} {h-right} V{right} Z',119),
        ('bottom',f'M{left} {h-bottom} H{w-right} L{w} {h} H{left} Z',119))
    for name,path,value in facets:
        element(mask,'path',id='bevel-'+name,d=path,fill=rgb([value]*3))
    element(mask,'path',id='top-edge',d=f'M{left} {unit} H{w}',fill='none',
        stroke=rgb([136]*3),stroke_width=2*unit)
    element(mask,'path',id='right-edge',d=f'M{w-unit} 0 V{h-unit}',fill='none',
        stroke=rgb([187]*3),stroke_width=2*unit)
    element(mask,'path',id='lower-edge',d=f'M{left} {h-unit/2} H{w-right}',fill='none',
        stroke=rgb([68]*3),stroke_width=unit)
    if code==0xe135:
        element(mask,'path',id='key-center-seam',d=f'M{w*.515} {top} V{h-bottom}',
            fill='none',stroke=rgb([34]*3),stroke_width=2*unit)


def guide(d):
    d.family = 'four-pane-guide'
    if np.any(d.source[d.source[:,:,3]>=128,:3].min(1)<170):
        lock(d)
        return
    alpha=d.source[:,:,3]
    yy,xx=np.where(alpha>=128)
    # Identify the two row/column intervals; each becomes one clean rectangle.
    def runs(values):
        indices=np.where(values)[0];breaks=np.where(np.diff(indices)>1)[0]+1
        return [(int(part.min()),int(part.max()+1)) for part in np.split(indices,breaks)]
    columns=runs((alpha>=128).any(0));rows=runs((alpha>=128).any(1))
    for j,(y0,y1) in enumerate(rows):
        for i,(x0,x1) in enumerate(columns):
            element(d.root,'rect',id=f'pane-{j}-{i}',x=x0,y=y0,width=x1-x0,height=y1-y0,fill='white')
            # The authored pale lower/right edge is a translucent bevel.
            if y1<d.height and alpha[y1,x0:x1].max()>0:
                element(d.root,'path',id=f'pane-{j}-{i}-bottom-edge',d=f'M{x0} {y1+.5} H{x1}',
                    fill='none',stroke='white',stroke_opacity=float(np.median(alpha[y1,x0:x1]))/255,stroke_width=1)


def legacy_sphere(d, code):
    d.family = 'legacy-metal-button'
    g=d.group(16,18,id='legacy-metal-button')
    white=code==0xe105
    clip=element(d.defs,'clipPath',id='sphere-edge')
    element(clip,'ellipse',cx=8,cy=10,rx=7.9,ry=7.9)
    g.set('clip-path','url(#sphere-edge)')
    rim=d.linear('sphere-rim',[(0,(102,102,102)),(.3,(68,68,68)),(.6,(170,170,170)),
        (.85,(119,119,119)),(1,(68,68,68))],y1=2,y2=18,x1=2,x2=14)
    element(g,'ellipse',id='outer-rim',cx=8,cy=10,rx=7.7,ry=7.7,fill=rim)
    body=d.radial_solid('sphere-body',[(0,(85,85,85)),(.35,(15,15,15)),(.8,(8,8,8)),(1,(17,17,17))],
        5,6,11,11)
    if white:
        body=d.radial_solid('white-body',[(0,(255,255,255)),(.48,(255,255,255)),(.8,(187,187,187)),
            (1,(102,102,102))],4.5,7.5,11,11)
    element(g,'ellipse',id='sphere-face',cx=7.8,cy=9.6,rx=6.65,ry=6.65,fill=body)
    if not white:
        gloss=d.radial('sphere-reflection',(205,205,205),5.3,5.8,3.2,4.4)
        element(g,'ellipse',id='reflection',cx=5.3,cy=5.8,rx=3.2,ry=4.4,fill=gloss)
    if code in (0xe10c,0xe10d):
        path='M5.5 6.6 L12.7 10 L5.5 13.4 Z' if code==0xe10c else 'M10.4 6.8 L4 10 L10.4 13.7 Z'
        d.solid(element(g,'path',id='direction-symbol',d=path),(235,235,235))
    elif code in (0xe10e,0xe10f,0xe110,0xe111):
        letter='L' if code in (0xe10e,0xe110) else 'R'
        reflection=d.radial('lower-reflection',(119,119,119),10,12,4.5,4.5)
        element(g,'ellipse',id='lower-reflection',cx=10,cy=12,rx=4.5,ry=4.5,fill=reflection)
        element(g,'path',id='inner-ring-highlight',d='M2.1 11 C1.6 5.9 5.6 3.3 11 3.8',
            fill='none',stroke=rim,stroke_width='.9')
        ink='M6 5.8 H7.3 V12 H10.7 V13.7 H6 Z' if letter=='L' else (
            'M5.5 5.4 H9 C11 5.4 11.4 6.6 11.4 8 C11.4 9.2 10.8 9.8 9.7 10 '
            'L11.8 13.8 H9.6 L7.8 10.3 H7.1 V13.8 H5.5 Z '
            'M7.1 7 V8.7 H8.9 C9.8 8.7 9.9 8.4 9.9 7.8 C9.9 7.3 9.5 7 8.9 7 Z')
        d.solid(element(g,'path',id='letter-'+letter,d=ink,fill_rule='evenodd'),(220,220,220))


def lock(d):
    d.family='lock-button'
    g=d.group(32,36,id='lock-button')
    ring=d.linear('outer-metal',[(0,(221,238,255)),(.5,(187,221,255)),(1,(221,238,255))],y1=6,y2=32)
    element(g,'circle',id='blue-inset',cx=16.5,cy=19,r=12,fill='#88bbff',fill_opacity=51/255)
    element(g,'circle',id='outer-glow',cx=16.5,cy=19,r=12,fill='none',stroke='#99ccff',
        stroke_width='3',stroke_opacity=34/255)
    element(g,'circle',id='outer-ring',cx=16.5,cy=19,r=12,fill='none',stroke=ring,
        stroke_width='1.05',stroke_opacity=.67)
    element(g,'circle',id='inner-metal-ring',cx=16.5,cy=19,r=9,fill='none',stroke=ring,
        stroke_width='.7',stroke_opacity=.45)
    gold=d.linear('lock-gold',[(0,(255,238,85)),(1,(255,204,102))],y1=17,y2=26,calibrate=False)
    element(g,'path',id='shackle',d='M12.5 17 V14.5 C12.5 10.6 20 10.6 20 14.5 V17',
        fill='none',stroke=gold,stroke_width=1.8,stroke_opacity=.65)
    element(g,'rect',id='lock-body',x=11.3,y=17.2,width=10.3,height=9,rx='.4',fill=gold,fill_opacity=.44)
    element(g,'path',id='lock-left-bevel',d='M11.3 17.2 H12.3 V26.2 H11.3 Z',
        fill='#ffdd77',fill_opacity=.15)


def legacy_trigger(d, code):
    d.family='legacy-shoulder-letter'
    g=d.group(17,18,id='legacy-shoulder-letter')
    body=d.linear('button-body',[(0,(0,0,0)),(1,(68,68,68))],y1=0,y2=0,x1=0,x2=17)
    rim=d.linear('button-edge',[(0,(153,153,153)),(1,(85,85,85))],y1=2,y2=18)
    group=element(g,'g',transform='translate(17 0) scale(-1 1)' if code==0xe107 else 'translate(0 0)')
    element(group,'path',id='body',d='M16 2 H8 C3 2 .5 5 .5 10 C.5 15 3 17.5 8 17.5 H16 Z',
        fill=body,stroke=rim,stroke_width='.6')
    letter='L' if code==0xe106 else 'R'
    ink='M8 5 H10 V13 H14 V15 H8 Z' if letter=='L' else (
        'M2 5 H7 C9 5 10 6 10 8 C10 9.5 8.7 10 7.6 10 '
        'L10 14 H8 L6 10 H4 V14 H2 Z M4 7 V8.6 H7 C8 8.6 8 8 8 7.8 C8 7.2 7.4 7 7 7 Z')
    d.solid(element(g,'path',id='letter-'+letter,d=ink,fill_rule='evenodd'),(220,220,220))


def draw(image, path, scale=4, code=0xe100, name='control'):
    d=Drawing(image,code,path,scale,name)
    w,h=image.size
    if 0xe100<=code<=0xe103:face(d,code)
    elif code in (0xe104,0xe105):
        if w<=16:legacy_sphere(d,code)
        else:shoulder(d,code)
    elif code in (0xe106,0xe107):
        if w<=17:legacy_trigger(d,code)
        else:trigger(d,code)
    elif 0xe108<=code<=0xe10b:arrow(d,code)
    elif code in (0xe10c,0xe10d):start_back(d,code,legacy=w<=16)
    elif code in (0xe10e,0xe10f,0xe110,0xe111,0xe12c,0xe12d,0xe12e,0xe12f,0xe130,0xe131):
        if w<=16:legacy_sphere(d,code)
        else:sticks(d,code)
    elif 0xe132<=code<=0xe135:key_frame(d,code)
    elif code==0xe13c:guide(d)
    else:raise ValueError(f'No authored design for U+{code:04X}')
    d.calibrate()
    d.save()
    result=render(path,scale)
    return result, dict(family=d.family,paint_anchors=len(d.slots),
        vector_shapes=sum(1 for node in d.root.iter() if node.tag.rsplit('}',1)[-1] in
            ('path','rect','ellipse','circle','polygon')),
        gradients=len(d.defs.findall('{*}linearGradient'))+len(d.defs.findall('{*}radialGradient')))
