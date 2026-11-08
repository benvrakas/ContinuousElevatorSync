"""Draws the Continuous Elevator Sync icon: a space elevator cable with a clock face beside it.

  python Tools/make_icon.py

Writes Docs/Images/icon-1080.png (full resolution), Docs/Images/icon-512.png (mod page) and Resources/Icon128.png
(shown in the game's Mods menu). The design is laid out on a 512-unit grid and rendered at full resolution.
"""
import math
import os
from PIL import Image, ImageDraw

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SIZE = 512  # Design grid; all coordinates below are in these units.
FULL_SIZE = 1080
SUPERSAMPLE = 2  # Drawn larger than FULL_SIZE and scaled down for smooth edges.
SCALE = FULL_SIZE * SUPERSAMPLE / SIZE

BACKGROUND_TOP = (24, 34, 58)
BACKGROUND_BOTTOM = (10, 14, 28)
CABLE = (150, 160, 180)
CAR = (120, 126, 140)
CAR_EDGE = (78, 82, 94)
CLOCK = (235, 238, 245)
CLOCK_EDGE = (150, 110, 230)
ORANGE = (250, 149, 73)
ORANGE_DARK = (196, 104, 40)
STAR = (214, 220, 250)


def s(value):
    return value * SCALE


def ellipse(draw, cx, cy, r, **kwargs):
    draw.ellipse([s(cx - r), s(cy - r), s(cx + r), s(cy + r)], **kwargs)


def main():
    big = int(FULL_SIZE * SUPERSAMPLE)
    image = Image.new("RGB", (big, big))
    draw = ImageDraw.Draw(image)
    for row in range(big):
        t = row / big
        color = tuple(int(BACKGROUND_TOP[i] + (BACKGROUND_BOTTOM[i] - BACKGROUND_TOP[i]) * t) for i in range(3))
        draw.line([(0, row), (big, row)], fill=color)

    for x, y in [(60, 70), (130, 140), (440, 60), (90, 330), (470, 250), (300, 40), (40, 450)]:
        ellipse(draw, x, y, 3, fill=STAR)

    # Elevator cable running up the left side, with a car on it.
    draw.rectangle([s(150), 0, s(158), big], fill=CABLE)
    draw.rectangle([s(120), s(300), s(188), s(380)], fill=CAR, outline=CAR_EDGE, width=int(s(6)))
    draw.rectangle([s(134), s(318), s(174), s(346)], fill=ORANGE, outline=ORANGE_DARK, width=int(s(4)))

    # Clock face on the right.
    cx, cy, r = 336, 256, 128
    ellipse(draw, cx, cy, r + 10, fill=CLOCK_EDGE)
    ellipse(draw, cx, cy, r, fill=CLOCK)
    for k in range(12):
        angle = math.radians(k * 30)
        inner = r - (22 if k % 3 == 0 else 12)
        draw.line([(s(cx + math.sin(angle) * inner), s(cy - math.cos(angle) * inner)),
                   (s(cx + math.sin(angle) * (r - 4)), s(cy - math.cos(angle) * (r - 4)))],
                  fill=(60, 64, 80), width=int(s(6)))
    for angle_deg, length, width in [(0, 96, 10), (120, 66, 12)]:
        angle = math.radians(angle_deg)
        draw.line([(s(cx), s(cy)), (s(cx + math.sin(angle) * length), s(cy - math.cos(angle) * length))],
                  fill=ORANGE_DARK, width=int(s(width)))
    ellipse(draw, cx, cy, 10, fill=ORANGE_DARK)

    full = image.resize((FULL_SIZE, FULL_SIZE), Image.LANCZOS)
    docs = os.path.join(ROOT, "Docs", "Images")
    resources = os.path.join(ROOT, "Resources")
    os.makedirs(docs, exist_ok=True)
    os.makedirs(resources, exist_ok=True)
    full.save(os.path.join(docs, "icon-1080.png"))
    full.resize((512, 512), Image.LANCZOS).save(os.path.join(docs, "icon-512.png"))
    full.resize((128, 128), Image.LANCZOS).save(os.path.join(resources, "Icon128.png"))
    print("Wrote Docs/Images/icon-1080.png, Docs/Images/icon-512.png and Resources/Icon128.png")


if __name__ == "__main__":
    main()
