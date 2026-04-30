#!/usr/bin/env python3
"""
Convert a StepMania pump noteskin to openitup SPRJ-based noteskin format.

Usage:
    python3 tools/sm2noteskin.py <stepmania-noteskin-dir> <output-dir>

Example:
    python3 tools/sm2noteskin.py \
        ~/stepmania/NoteSkins/pump/newextra \
        noteskin/newextra
"""

import argparse
import json
import os
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("ERROR: Pillow is required. Install with: pip install Pillow", file=sys.stderr)
    sys.exit(1)

# StepMania direction names -> our track numbers
DIRECTIONS = {
    "DownLeft": "00",
    "UpLeft": "01",
    "Center": "02",
    "UpRight": "03",
    "DownRight": "04",
}

# StepMania mirrors UpRight from UpLeft and DownRight from DownLeft (Y-flip)
MIRROR_FALLBACKS = {
    "UpRight": "UpLeft",
    "DownRight": "DownLeft",
}


def parse_grid_hint(filename: str) -> tuple[int, int] | None:
    """Extract NxM grid hint from a StepMania sprite sheet filename."""
    stem = Path(filename).stem
    parts = stem.split()
    if not parts:
        return None
    last = parts[-1]
    if "x" in last:
        try:
            cols, rows = last.split("x")
            return int(cols), int(rows)
        except ValueError:
            return None
    return None


def extract_frames(img: Image.Image, cols: int, rows: int) -> list[Image.Image]:
    """Split a sprite sheet into individual frames, left-to-right top-to-bottom."""
    fw = img.width // cols
    fh = img.height // rows
    frames = []
    for r in range(rows):
        for c in range(cols):
            frame = img.crop((c * fw, r * fh, (c + 1) * fw, (r + 1) * fh))
            frames.append(frame)
    return frames


def pad_or_trim_frames(frames: list[Image.Image], target: int) -> list[Image.Image]:
    """Ensure exactly `target` frames by duplicating last or trimming."""
    if len(frames) >= target:
        return frames[:target]
    while len(frames) < target:
        frames.append(frames[-1].copy())
    return frames


def save_sprj(frames: list[Image.Image], output_dir: Path, sprj_name: str):
    """Save a list of frames as individual PNGs and a SPRJ manifest."""
    sprj_stem = Path(sprj_name).stem

    pictures = []
    for i, frame in enumerate(frames):
        png_name = f"{sprj_stem}_f{i:02d}.png"
        png_path = output_dir / png_name
        frame.save(png_path, "PNG")

        w, h = frame.size
        pictures.append({
            "texture": png_name,
            "rect": [0, 0, w, h],
            "uv": [0.0, 0.0, 1.0, 1.0],
        })

    sprj = {
        "source_format": "sm2noteskin",
        "mode": "ANI",
        "pictures": pictures,
    }

    sprj_path = output_dir / sprj_name
    with open(sprj_path, "w") as f:
        json.dump(sprj, f, indent=2)


def find_file_ci(directory: Path, name: str) -> Path | None:
    """Case-insensitive file lookup in a directory."""
    name_lower = name.lower()
    for entry in directory.iterdir():
        if entry.name.lower() == name_lower:
            return entry
    return None


def convert_sprite_sheet(src_dir: Path, out_dir: Path, sm_filename: str,
                         sprj_name: str, target_frames: int = 6,
                         flip_horizontal: bool = False) -> bool:
    """Convert a StepMania sprite sheet to a SPRJ with individual frame PNGs."""
    src_file = find_file_ci(src_dir, sm_filename)
    if src_file is None:
        return False

    img = Image.open(src_file).convert("RGBA")

    grid = parse_grid_hint(sm_filename)
    if grid:
        cols, rows = grid
    else:
        cols, rows = 1, 1

    frames = extract_frames(img, cols, rows)
    frames = pad_or_trim_frames(frames, target_frames)

    if flip_horizontal:
        frames = [f.transpose(Image.FLIP_LEFT_RIGHT) for f in frames]

    save_sprj(frames, out_dir, sprj_name)
    return True


def convert_noteskin(src_dir: Path, out_dir: Path):
    """Convert a full StepMania pump noteskin to openitup format."""
    out_dir.mkdir(parents=True, exist_ok=True)

    converted = 0
    skipped = 0

    for sm_dir, track in DIRECTIONS.items():
        mirror_from = MIRROR_FALLBACKS.get(sm_dir)

        def try_convert(sm_filename, sprj_name, target=6):
            """Try direct, then mirror fallback with horizontal flip."""
            nonlocal converted, skipped
            if convert_sprite_sheet(src_dir, out_dir, sm_filename, sprj_name, target):
                print(f"  {sm_filename} -> {sprj_name}")
                converted += 1
                return True
            if mirror_from:
                mirror_filename = sm_filename.replace(sm_dir, mirror_from)
                if convert_sprite_sheet(src_dir, out_dir, mirror_filename, sprj_name,
                                        target, flip_horizontal=True):
                    print(f"  {mirror_filename} -> {sprj_name} (mirrored from {mirror_from})")
                    converted += 1
                    return True
            print(f"  SKIP {sprj_name} ({sm_filename} not found)")
            skipped += 1
            return False

        # TAP: "{Dir} Tap Note 3x2.png" -> ARROW##_TAP.sprj
        tap_name = f"{sm_dir} Tap Note 3x2.png"
        sprj_tap = f"ARROW{track}_TAP.sprj"
        try_convert(tap_name, sprj_tap)

        # LONG HEAD: reuse tap note (StepMania redirects hold head to tap note)
        sprj_head = f"ARROW{track}_LONG_HEAD.sprj"
        try_convert(tap_name, sprj_head)

        # LONG BODY: "{Dir} Hold Body Active 6x1.png" -> ARROW##_LONG_BODY.sprj
        body_name = f"{sm_dir} Hold Body Active 6x1.png"
        sprj_body = f"ARROW{track}_LONG_BODY.sprj"
        try_convert(body_name, sprj_body)

        # LONG TAIL: "{Dir} Hold BottomCap Active 6x1.png" -> ARROW##_LONG_TAIL.sprj
        tail_name = f"{sm_dir} Hold BottomCap Active 6x1.png"
        sprj_tail = f"ARROW{track}_LONG_TAIL.sprj"
        try_convert(tail_name, sprj_tail)

        # PRESS: "{Dir} Ready Receptor {grid}.png" -> ARROW##_PRESS.sprj
        sprj_press = f"ARROW{track}_PRESS.sprj"
        receptor_name = f"{sm_dir} Ready Receptor 1x3.png"
        receptor_name_alt = f"{sm_dir} Ready Receptor 3x1.png"
        if not convert_sprite_sheet(src_dir, out_dir, receptor_name, sprj_press, 6):
            if not convert_sprite_sheet(src_dir, out_dir, receptor_name_alt, sprj_press, 6):
                if mirror_from:
                    mr = receptor_name.replace(sm_dir, mirror_from)
                    mr_alt = receptor_name_alt.replace(sm_dir, mirror_from)
                    if convert_sprite_sheet(src_dir, out_dir, mr, sprj_press, 6,
                                            flip_horizontal=True):
                        print(f"  {mr} -> {sprj_press} (mirrored from {mirror_from})")
                        converted += 1
                    elif convert_sprite_sheet(src_dir, out_dir, mr_alt, sprj_press, 6,
                                              flip_horizontal=True):
                        print(f"  {mr_alt} -> {sprj_press} (mirrored from {mirror_from})")
                        converted += 1
                    else:
                        print(f"  SKIP {sprj_press} (no receptor found)")
                        skipped += 1
                else:
                    print(f"  SKIP {sprj_press} (no receptor found)")
                    skipped += 1
            else:
                print(f"  {receptor_name_alt} -> {sprj_press}")
                converted += 1
        else:
            print(f"  {receptor_name} -> {sprj_press}")
            converted += 1

    # JUDGE: "_flash (doubleres).png" -> ARROW##_JUDGE.sprj (shared across all tracks)
    flash_file = find_file_ci(src_dir, "_flash (doubleres).png")
    if flash_file:
        img = Image.open(flash_file).convert("RGBA")
        # Resize to 64x64 if needed
        if img.size != (64, 64):
            img = img.resize((64, 64), Image.LANCZOS)
        frames = [img.copy() for _ in range(6)]
        for track in DIRECTIONS.values():
            sprj_judge = f"ARROW{track}_JUDGE.sprj"
            save_sprj(frames, out_dir, sprj_judge)
            print(f"  _flash (doubleres).png -> {sprj_judge}")
            converted += 1
    else:
        for track in DIRECTIONS.values():
            print(f"  SKIP ARROW{track}_JUDGE.sprj (no flash found)")
            skipped += 1

    print(f"\nDone: {converted} converted, {skipped} skipped")
    print(f"Output: {out_dir}")


def main():
    parser = argparse.ArgumentParser(
        description="Convert StepMania pump noteskin to openitup SPRJ format"
    )
    parser.add_argument("input", help="Path to StepMania noteskin directory")
    parser.add_argument("output", help="Path to output openitup noteskin directory")
    args = parser.parse_args()

    src_dir = Path(args.input)
    out_dir = Path(args.output)

    if not src_dir.is_dir():
        print(f"ERROR: Input is not a directory: {src_dir}", file=sys.stderr)
        sys.exit(1)

    print(f"Converting: {src_dir.name}")
    print(f"Source: {src_dir}")
    print(f"Output: {out_dir}")
    print()

    convert_noteskin(src_dir, out_dir)


if __name__ == "__main__":
    main()
