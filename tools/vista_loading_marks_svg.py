"""Authored Vista loading tick strips: repeated vector marks and grouped gaps.

The original line cadences are expressed as a few marker runs, rather than a
font, source pixel row, raster mask or per-pixel geometry. All marks share the
same vector body, a two-anchor paint ramp and a small optical softness filter.
"""
from vista_prompt_svg import element


SPARSE_RUNS = (
    (28.41, 7), (185.37, 7), (342.30, 7), (499.25, 7),
    (656.20, 7), (813.11, 7), (970.06, 7), (1127.00, 7),
    (1283.92, 6), (1421.25, 7), (1578.19, 7), (1737.09, 5),
    (1854.77, 7),
)
DENSE_RUNS = (
    (317.80, 86), (1033.81, 24), (1249.60, 31), (1569.34, 11),
    (1694.88, 2), (1741.98, 2), (1798.87, 25), (2018.56, 2),
)


def tick_strip(d, dense=False):
    d.family = "vista-loading-dense-ticks" if dense else "vista-loading-spaced-ticks"
    if (d.width, d.height) != (2048, 64 if dense else 32):
        raise ValueError("Unexpected Vista loading-marker canvas")
    runs = DENSE_RUNS if dense else SPARSE_RUNS
    cadence = (2048 / 1044) * (4 if dense else 10)
    width, y, height, alpha = (3.90, 24.55, 6.18, 47 / 255) if dense else (2.10, 6.1, 7.30, 43 / 255)
    softness = element(d.defs, "filter", id="tick-optical-softness",
        filterUnits="userSpaceOnUse", x=-4, y=-4, width=2056, height=d.height + 8,
        color_interpolation_filters="sRGB")
    element(softness, "feGaussianBlur", stdDeviation=".80 .45" if dense else ".60 .42")
    paint = element(d.defs, "linearGradient", id="tick-edge-colour",
        gradientUnits="userSpaceOnUse", x1=0, x2=0,
        y1=23.8 if dense else 5.2, y2=25.3 if dense else 7)
    element(paint, "stop", offset=0, stop_color="#0204fa")
    element(paint, "stop", offset=1, stop_color="#314d9c")
    body = element(d.root, "g", id="tick-runs", fill="url(#tick-edge-colour)",
        opacity=alpha, filter="url(#tick-optical-softness)")
    for run, (first, count) in enumerate(runs):
        group = element(body, "g", id=f"tick-run-{run}")
        for ordinal in range(count):
            element(group, "rect", x=first + ordinal * cadence - width / 2,
                    y=y, width=width, height=height)
