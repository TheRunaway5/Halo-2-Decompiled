#!/usr/bin/env python3
"""Authored vector studies for the Vista-only executable loading artwork.

Circular bands, radial joints and fastener holes are individually editable.
The original images supply reference identity and canvas dimensions only at
build time. Geometry below was measured against the native reference views;
no raster contours, embedded images, fonts or pixel-region paths are emitted.
These are unfinished review studies, not a complete loading pack.
"""
import argparse
import json
import math
from pathlib import Path

from PIL import Image

from audit_rendering import file_sha
from verify_vista_menus import svg_shapes
from vista_menu_svg import MenuDrawing
from vista_loading_marks_svg import tick_strip
from vista_prompt_source import write_dds
from vista_prompt_svg import element, render

ROOT = Path(__file__).resolve().parents[1]
EXPECTED_REFERENCE = "226b31e6deae10b191fc4e974e6454bc41d0b51ea5a123403033b072c3d63814"
PAGES = {
    3: "5aa17d5b92d1a462073f51d58b6575733be53185346c576ec7f06decd5d64230",
    4: "b73ac539e6274521a16b469c28659fa04844b4845f03f8345c8fc8fb94b541a4",
    5: "8e2273ca87d0236efe74fde88449023e670637a5a93bd3ec5518aa75cdf109d5",
    9: "9999e4fe611e8c2ab57f25f239b0fbea3efa729430e2695c0ddabd1bc813d478",
    10: "9968e6c5af4e8665d49f958a8bb8774bf7eae656eea893afdb96f8eed3dd70b9",
}


def point(radius, angle, cx=256, cy=256):
    t = math.radians(angle)
    return cx + radius * math.sin(t), cy - radius * math.cos(t)


def xy(p):
    return f"{p[0]:.4f} {p[1]:.4f}"


def arc(radius, start, end):
    a, b = point(radius, start), point(radius, end)
    return f"M{xy(a)} A{radius} {radius} 0 {int(end-start > 180)} 1 {xy(b)}"


def band(parent, inner, outer, start, end, fill="white", **attrs):
    """A semantic annular sector, with clockwise angles from the top."""
    a, b, c, d = (point(outer, start), point(outer, end),
                   point(inner, end), point(inner, start))
    large = int(end - start > 180)
    path = (f"M{xy(a)} A{outer} {outer} 0 {large} 1 {xy(b)} L{xy(c)} "
            f"A{inner} {inner} 0 {large} 0 {xy(d)} Z")
    return element(parent, "path", d=path, fill=fill, **attrs)


def circle_ray(circle, angle):
    """Intersect an authored circle with a ray from the 256/256 design pivot."""
    cx, cy, radius = circle
    t = math.radians(angle)
    dx, dy = math.sin(t), -math.cos(t)
    vx, vy = 256 - cx, 256 - cy
    projection = dx * vx + dy * vy
    distance = -projection + math.sqrt(projection ** 2 - vx ** 2 - vy ** 2 + radius ** 2)
    return 256 + distance * dx, 256 + distance * dy


def separate_edges(parent, inner, outer, start, end, fill="white"):
    """Circular plate edges retain their independently measured centres.

    The constants in the recipes describe a few complete circular primitives,
    not a sampled boundary or source-dependent list of contour vertices.
    """
    a, b = circle_ray(outer, start), circle_ray(outer, end)
    c, d = circle_ray(inner, end), circle_ray(inner, start)
    large = int(end - start > 180)
    return element(parent, "path", fill=fill,
        d=f"M{xy(a)} A{outer[2]} {outer[2]} 0 {large} 1 {xy(b)} L{xy(c)} "
          f"A{inner[2]} {inner[2]} 0 {large} 0 {xy(d)} Z")


def plate(parent, inner, outer_edges):
    """Join the stepped outside edges into one fill without internal seams."""
    first_circle, start, _ = outer_edges[0]
    end = outer_edges[-1][2]
    path = "M" + xy(circle_ray(first_circle, start))
    for i, (circle, a, b) in enumerate(outer_edges):
        if i:
            path += " L" + xy(circle_ray(circle, a))
        path += f" A{circle[2]} {circle[2]} 0 {int(b-a > 180)} 1 {xy(circle_ray(circle, b))}"
    path += f" L{xy(circle_ray(inner, end))} A{inner[2]} {inner[2]} 0 {int(end-start > 180)} 0 {xy(circle_ray(inner, start))} Z"
    return element(parent, "path", d=path, fill="white")


def hole(parent, x, y, r):
    element(parent, "circle", cx=x, cy=y, r=r, fill="black")


def thin_ring(d):
    d.family = "vista-loading-thin-mechanical-ring"
    element(d.root, "rect", width=512, height=512, fill="black")
    rim = element(d.root, "g", id="outer-rim")
    # The wide top/bottom plates extend beyond the two narrow side rails.
    band(rim, 225.65, 247.6, 269.1, 333.3)
    band(rim, 228.5, 247.6, 16.6, 145.5)
    band(rim, 225.6, 252.5, 145.5, 179.5)
    # The top plate starts with a dog-legged cap; its inner arc starts later
    # than the outside arc. A simple annular sector produced the wrong tooth.
    outer_a, outer_b = point(252.5, 328.1), point(252.5, 374.1)
    inner_b, inner_a = point(226.5, 374.1), point(226.5, 333.25)
    element(rim, "path", fill="white",
        d=f"M{xy(outer_a)} A252.5 252.5 0 0 1 {xy(outer_b)} L{xy(inner_b)} "
          f"A226.5 226.5 0 0 0 {xy(inner_a)} L147 47 H128 Z")
    # Raised ends and short outside tabs belong to the rim, not a smooth torus.
    band(rim, 247.2, 252.5, 269.1, 280.2)
    band(rim, 247.2, 252, 92.8, 102.2)
    band(rim, 249, 254, 349.8, 360, fill="black")
    band(rim, 249, 254, 13.1, 14.2, fill="black")
    element(rim, "path", d=arc(235.2, 269.4, 332.2), fill="none", stroke="black", stroke_width=2.25)
    element(rim, "path", d=arc(235.2, 16.6, 145), fill="none", stroke="black", stroke_width=2.25)
    band(rim, 223.6, 228.6, 47.5, 87.8)
    band(rim, 223.6, 228.6, 92.7, 111.5)
    band(rim, 224.1, 228.6, 128.5, 132.0)
    band(rim, 225.6, 229.0, 145.5, 163, fill="black")
    band(rim, 225.65, 229.8, 300.5, 333.3, fill="black")
    element(rim, "path", d="M121 46 H135 V47.2 H140 V48.2 H145.8 L148.2 49.6 L146.6 50.7 L139.5 49.6 H135 V48.5 H121 Z", fill="black")
    element(rim, "path", d="M394 440.5 L398 443.5 V449.5 L405.5 456", fill="none", stroke="black", stroke_width=1.6)
    element(rim, "path", d="M4 254.5 H61 L59 258 H4 Z", fill="white")
    hole(rim, 10.2, 248.5, 2.8)
    hole(rim, 273.5, 486.2, 2.55)
    hole(rim, 281.3, 485.7, 2.5)
    element(rim, "path", d="M264 484 H268 V503 H264 Z", fill="black")
    # A narrow radial bridge joins the right inner band to the outer rail.
    element(d.root, "path", id="radial-bridge",
            d="M384.7 220.2 L474 193 L475.5 198.1 L386 225.2 Z", fill="white")
    inside = element(d.root, "g", id="inner-index-band")
    for start, end in ((74.68, 101.92), (104.99, 131.42),
                       (132.68, 139.85), (141.88, 147.75),
                       (149.85, 156.12), (158.19, 165.02)):
        separate_edges(inside, (258.5, 255.1, 130.3), (257.9, 255.1, 146.9), start, end)
    for start, end in ((180.0, 243.97), (245.09, 248.86),
                       (250.89, 258.07), (260.11, 266.03),
                       (268.07, 274.55), (276.55, 283.43)):
        separate_edges(inside, (257.0, 257.0, 132.0), (256.2, 255.9, 149.0), start, end)
    separate_edges(inside, (253.2, 258.5, 128.6), (256.4, 260.1, 148.2), 298.73, 320.79)


def wide_ring(d):
    d.family = "vista-loading-wide-mechanical-ring"
    element(d.root, "rect", width=512, height=512, fill="black")
    body = element(d.root, "g", id="segmented-plates")
    # Deliberately separate plates retain the asymmetric gaps and stepped rim.
    inner = (256.0, 256.0, 153.2)
    plate(body, inner, (((252.9, 257.7, 216.3), 20.4, 43.0),
                        ((257.1, 256.5, 208.0), 43.0, 70.5)))
    plate(body, inner, (((256, 256, 214.0), 90.5, 112.8),
                        ((259.4, 260.7, 203.1), 112.8, 164.7),
                        ((254.1, 257.1, 212.4), 164.7, 179.8)))
    plate(body, inner, (((254.1, 257.1, 212.4), 180.0, 269.1),
                        ((263.7, 259.8, 218.7), 269.1, 316.8),
                        ((256, 256, 214.5), 316.8, 328.8)))
    plate(body, inner, (((256, 256, 211.0), 331.1, 349.5),
                        ((256, 256, 214.5), 349.5, 359.1)))
    element(body, "path", d="M217.5 46 H230 V42 H252.5 V103.3 L246.8 104 L246.8 49.5 H217.5 Z", fill="white")
    # Small internal and outside shoulders are independent of the main arcs.
    for start, end in ((31.0, 59.0), (101.5, 131.0), (229.0, 308.0)):
        band(body, 152.7, 158.2, start, end, fill="black")
    element(body, "path", d="M388 175 L394 191 L398 194 H403 L407 189 L415 185 L422 186 V189 L431 190 L441 194 L453 186 L468 221 H388 Z", fill="black")
    element(body, "path", d="M256 408.5 H265 V474 H256 Z", fill="black")
    element(body, "path", d="M242.8 408.2 L247.8 409 L242.2 472 H237.4 Z", fill="black")
    element(body, "path", d="M193.7 405.7 L200 408.5 L172.7 458.8 L166.4 455.6 Z", fill="black")
    element(body, "path", d="M145.9 73.5 L152 70.1 L182.9 123.2 L176.2 127.2 Z", fill="black")
    # The narrow circular channels terminate at dog-legged plate joints.
    channels = element(d.root, "g", id="channels-and-joints", fill="none", stroke="black",
                       stroke_width=4.1, stroke_linejoin="miter")
    element(channels, "path", d=arc(173.3, 90.5, 137.0))
    element(channels, "path", d=arc(173.3, 218.8, 309.5))
    element(channels, "path", d="M111 96 L119 104 V141.5 L128.3 151")
    element(channels, "path", d="M111.6 415.1 L121 405 H133 L161.5 376.3")
    element(channels, "path", d="M152.4 387.5 H172.1 L174.2 385.5", stroke_width=3)
    element(channels, "path", d="M382.1 422.8 L374 413.5 V382 L357.4 366.8")
    element(d.root, "path", d="M376 382.5 H379.5 V389 H374.5 V383 Z", fill="black")
    element(channels, "path", d="M48 223.5 L84.2 231.6 M44.5 238.8 L82 241.2 M50.2 330.2 L92.5 314.2",
            stroke_width=7.1)
    # Fastener bores are hand-positioned circular features, not sampled pixels.
    bores = element(d.root, "g", id="fastener-bores")
    for x, y, radius in ((249.7, 87.1, 2.8), (249.7, 95.5, 2.6),
                         (337.0, 67.9, 2.5), (345.3, 71.8, 2.65),
                         (460.1, 269.3, 2.6), (459.4, 278.0, 2.7),
                         (357.7, 380.5, 2.6), (129.4, 399.8, 2.5),
                         (133.1, 146.5, 2.8)):
        hole(bores, x, y, radius)
    for path in ("M232.8 47 H245 V52 H237.2 Z",
                 "M315.1 98.2 L318.4 99.3 L313.6 110.8 L309.9 109.1 Z",
                 "M414.2 258.3 H422.5 L425 261.5 H414.2 Z"):
        element(bores, "path", d=path, fill="black")


def optical_glint(d):
    d.family = "vista-loading-optical-glint"
    element(d.root, "rect", width=2048, height=4, fill="black")
    # All four original scanlines share one smooth optical profile. These
    # sixteen sparse paint anchors describe the highlight and its shoulders;
    # no original row, per-pixel values or sampled palette grid is stored.
    gradient = element(d.defs, "linearGradient", id="glint-profile",
        gradientUnits="userSpaceOnUse", x1=560, y1=0, x2=784, y2=0)
    for x, gray in ((560, 0), (576, 1), (592, 9), (608, 31),
                    (624, 83), (640, 160), (656, 230), (667, 255),
                    (675, 255), (688, 228), (704, 155), (720, 80),
                    (736, 29), (752, 6), (768, 0), (784, 0)):
        color = "#" + f"{gray:02x}" * 3
        element(gradient, "stop", offset=(x - 560) / 224, stop_color=color)
    element(d.root, "rect", width=2048, height=4, fill="url(#glint-profile)")
    # Preserve the original separate flat grey tail instead of extending the
    # flare into this part of its native canvas.
    element(d.root, "rect", x=1344, width=704, height=4, fill="#282828")


RECIPES = {3: thin_ring, 4: wide_ring, 5: optical_glint,
           9: lambda d: tick_strip(d, dense=False),
           10: lambda d: tick_strip(d, dense=True)}


def build(reference, output):
    if file_sha(reference) != EXPECTED_REFERENCE:
        raise ValueError("The reviewed Vista reference inventory changed")
    output.mkdir(parents=True, exist_ok=False)
    inventory = json.loads(reference.read_text())
    manifest = dict(schema="halo2-vista-menu-masters-v1", complete=False,
        experimental=True, admitted=False, reference_inventory_sha256=EXPECTED_REFERENCE,
        source_caches={k: v for k, v in inventory["sources"].items() if "loading" in k},
        pages={}, coverage=dict(source_unique_pages=862, bundled_unique_pages=len(PAGES),
            canonical_coverage_change=0, paused_emblem_drafts=243, unfinished_unique_pages=211),
        vista_only_scope=dict(unmatched_mcc_pages=13, loading_pages=12, server_icon_states=1,
            authored_loading_studies=len(PAGES), completed_or_admitted_loading_pages=0),
        policy="Experimental loading studies only; no original raster, contour tracing, font substitution or activation.",
        rights_review="Required. Faithful recreation is not redistribution clearance.")
    for index, key in PAGES.items():
        source = inventory["pages"][key]
        original = reference.parent / source["png"]
        if file_sha(original) != source["png_sha256"]:
            raise ValueError("Original loading reference changed")
        svg = output / "svg" / (key + ".svg")
        svg.parent.mkdir(parents=True, exist_ok=True)
        source_image = Image.open(original)
        density = min(8, 4096 // max(source_image.size))
        drawing = MenuDrawing(source_image, svg, density, "executable\\loading\\" + str(index), 0)
        drawing.root.find("{http://www.w3.org/2000/svg}desc").text = (
            "Unfinished hand-authored Vista loading-ring study: explicit circular arcs, plate joints and fastener bores. "
            "No raster contours, embedded pixels, source fonts or external assets. Visual review remains required.")
        if index == 5:
            drawing.root.find("{http://www.w3.org/2000/svg}desc").text = (
                "Unfinished hand-authored Vista loading-glint study: one sixteen-stop optical gradient and a separate flat tail. "
                "No embedded image, per-pixel paint grid, source font or external asset.")
        elif index in (9, 10):
            drawing.root.find("{http://www.w3.org/2000/svg}desc").text = (
                "Unfinished hand-authored Vista loading-marker study: shared vector tick bodies, measured cadences and grouped gaps. "
                "No source pixel row, raster mask, font substitution or per-pixel geometry.")
        RECIPES[index](drawing)
        drawing.finish()
        svg_shapes(svg)
        raster = render(svg, density)
        dds = output / "bitmaps" / (key + ".dds")
        dds.parent.mkdir(parents=True, exist_ok=True)
        write_dds(dds, raster)
        bank = next(b for b in inventory["owners"]["loading.bin"]
                    if b["name"] == "executable\\loading\\" + str(index))
        manifest["pages"][key] = dict(family=drawing.family, density=density, physical=list(raster.size),
            logical=bank["frames"][0].get("source_extent", list(source_image.size)),
            source_png_sha256=source["png_sha256"],
            vector=svg.relative_to(output).as_posix(), vector_sha256=file_sha(svg),
            dds=dds.relative_to(output).as_posix(), dds_sha256=file_sha(dds),
            state="authored-needs-review", experimental=True, admitted=False, fidelity_state="unfinished",
            bindings=[dict(cache="loading.bin", name=bank["name"], **bank["frames"][0])])
    (output / "masters.json").write_text(json.dumps(manifest, indent=2) + "\n")
    return manifest


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--reference", type=Path,
        default=ROOT / "analysis/frontend/vista-menu-vectors/pack/reference.json")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    manifest = build(args.reference, args.output)
    print(json.dumps(dict(pages=len(manifest["pages"]), complete=False, admitted=False), indent=2))


if __name__ == "__main__":
    main()
