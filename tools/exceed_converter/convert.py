#!/usr/bin/env python3
"""Convert Exceed arcade data into openitup engine format.

Usage:
    python -m tools.exceed_converter.convert /path/to/EXCEED_DATA /path/to/output

Input structure (Exceed arcade disc):
    EXCEED_DATA/
    ├── AUDIO/     A01.AUD (song), DA01.AUD (intro)
    ├── BGA/       A01.DAT (RESPACK archives)
    ├── STEP/      A01.STX (binary charts)
    ├── TITLE/     A01.PNZ (encrypted PNG banners)
    └── WAVE/      SFX files

Output structure (openitup engine):
    output/
    ├── songs/
    │   └── <SongName>/
    │       ├── song.mp3
    │       ├── intro.mp3
    │       ├── title.png
    │       ├── *.ksf (converted from STX)
    │       └── bga/
    │           ├── *.bga
    │           ├── *.spr
    │           └── *.tga
    └── sfx/
        └── *.wav
"""

import argparse
import os
import sys
import shutil
from pathlib import Path

from .enc2 import decode_pnz, decode_aud
from .respack import extract_respack


def find_files(data_dir: Path, subfolder: str, pattern: str) -> dict:
    """Find files matching pattern in a subfolder, keyed by song ID."""
    folder = data_dir / subfolder
    if not folder.exists():
        print(f"  Warning: {subfolder}/ not found")
        return {}

    result = {}
    for f in sorted(folder.iterdir()):
        if f.is_file() and f.suffix.upper() == pattern.upper():
            song_id = f.stem.upper()
            result[song_id] = f
    return result


def convert_exceed_data(data_dir: Path, output_dir: Path, song_names: dict = None):
    """Convert all Exceed arcade data to engine format.

    Args:
        data_dir: Path to EXCEED_DATA root
        output_dir: Path to output directory
        song_names: Optional dict mapping song IDs to names (e.g., {"A01": "Conga"})
    """
    songs_dir = output_dir / "songs"
    sfx_dir = output_dir / "sfx"
    songs_dir.mkdir(parents=True, exist_ok=True)
    sfx_dir.mkdir(parents=True, exist_ok=True)

    # Discover all song IDs from STEP folder (authoritative)
    stx_files = find_files(data_dir, "STEP", ".STX")
    aud_files = find_files(data_dir, "AUDIO", ".AUD")
    pnz_files = find_files(data_dir, "TITLE", ".PNZ")
    dat_files = find_files(data_dir, "BGA", ".DAT")

    # Find intro audio (DA01.AUD pattern)
    intro_files = {}
    audio_folder = data_dir / "AUDIO"
    if audio_folder.exists():
        for f in audio_folder.iterdir():
            if f.stem.upper().startswith("D") and f.suffix.upper() == ".AUD":
                song_id = f.stem[1:].upper()
                intro_files[song_id] = f

    print(f"Found {len(stx_files)} songs in STEP/")
    print(f"  Audio: {len(aud_files)}, Intros: {len(intro_files)}, "
          f"Titles: {len(pnz_files)}, BGAs: {len(dat_files)}")
    print()

    for song_id in sorted(stx_files.keys()):
        song_name = song_id
        if song_names and song_id in song_names:
            song_name = song_names[song_id]

        print(f"[{song_id}] {song_name}")
        song_dir = songs_dir / song_name
        song_dir.mkdir(parents=True, exist_ok=True)

        # Copy STX (engine already has STX parser)
        stx_path = stx_files[song_id]
        shutil.copy2(stx_path, song_dir / stx_path.name)
        print(f"  STX: {stx_path.name}")

        # Decrypt audio
        if song_id in aud_files:
            try:
                mp3_data = decode_aud(str(aud_files[song_id]))
                (song_dir / "song.mp3").write_bytes(mp3_data)
                print(f"  Audio: {aud_files[song_id].name} -> song.mp3 ({len(mp3_data)} bytes)")
            except Exception as e:
                print(f"  Audio FAILED: {e}")

        # Decrypt intro
        if song_id in intro_files:
            try:
                mp3_data = decode_aud(str(intro_files[song_id]))
                (song_dir / "intro.mp3").write_bytes(mp3_data)
                print(f"  Intro: {intro_files[song_id].name} -> intro.mp3")
            except Exception as e:
                print(f"  Intro FAILED: {e}")

        # Decrypt title image
        if song_id in pnz_files:
            try:
                png_data = decode_pnz(str(pnz_files[song_id]))
                (song_dir / "title.png").write_bytes(png_data)
                print(f"  Title: {pnz_files[song_id].name} -> title.png ({len(png_data)} bytes)")
            except Exception as e:
                print(f"  Title FAILED: {e}")

        # Extract BGA archive
        if song_id in dat_files:
            bga_dir = song_dir / "bga"
            bga_dir.mkdir(exist_ok=True)
            try:
                files = extract_respack(str(dat_files[song_id]))
                for fname, fdata in files:
                    out_path = bga_dir / fname
                    out_path.parent.mkdir(parents=True, exist_ok=True)
                    out_path.write_bytes(fdata)
                print(f"  BGA: {dat_files[song_id].name} -> {len(files)} files extracted")
            except Exception as e:
                print(f"  BGA FAILED: {e}")

        print()

    # Copy SFX files
    wave_dir = data_dir / "WAVE"
    if wave_dir.exists():
        sfx_count = 0
        for f in wave_dir.iterdir():
            if f.is_file():
                shutil.copy2(f, sfx_dir / f.name)
                sfx_count += 1
        print(f"SFX: copied {sfx_count} files to sfx/")

    print(f"\nDone! Output written to {output_dir}")


def main():
    parser = argparse.ArgumentParser(
        description="Convert Exceed arcade data to openitup engine format"
    )
    parser.add_argument("data_dir", help="Path to EXCEED_DATA root directory")
    parser.add_argument("output_dir", help="Path to output directory")
    parser.add_argument("--song-list", help="Path to song list CSV (id,name)")

    args = parser.parse_args()
    data_dir = Path(args.data_dir)
    output_dir = Path(args.output_dir)

    if not data_dir.exists():
        print(f"Error: {data_dir} does not exist", file=sys.stderr)
        sys.exit(1)

    song_names = None
    if args.song_list:
        song_names = {}
        with open(args.song_list) as f:
            for line in f:
                line = line.strip()
                if line and "," in line:
                    parts = line.split(",", 1)
                    song_names[parts[0].strip().upper()] = parts[1].strip()

    convert_exceed_data(data_dir, output_dir, song_names)


if __name__ == "__main__":
    main()
