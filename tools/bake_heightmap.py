#!/usr/bin/env python3
"""
Terrain Heightmap Bake Tool

Converts a heightmap image to raw binary tiles for the Imhotep terrain system.

Usage:
    python bake_heightmap.py <input_image> [options]

Options:
    --output-dir, -o    Output directory (default: ../res/terrain/height/)
    --tile-size, -t     Tile size in pixels (default: 512)
    --preview           Generate a preview image showing tile grid
    --dry-run           Show what would be done without writing files

Output format:
    - Files named tile_X_Y.bin
    - Raw binary, 16-bit unsigned integers (little-endian)
    - tile_size x tile_size texels per file
    - Values 0-65535 representing normalized height
"""

import argparse
import os
import sys
import struct
from pathlib import Path

try:
    from PIL import Image
    import numpy as np
    # Allow very large images (disable decompression bomb protection)
    Image.MAX_IMAGE_PIXELS = None
except ImportError:
    print("Error: This tool requires Pillow and NumPy")
    print("Install with: pip install Pillow numpy")
    sys.exit(1)


def load_heightmap(path: str) -> np.ndarray:
    """Load heightmap image and convert to grayscale float array."""
    print(f"Loading: {path}")
    img = Image.open(path)

    # Convert to grayscale if needed
    if img.mode != 'L':
        print(f"  Converting from {img.mode} to grayscale...")
        img = img.convert('L')

    # Convert to numpy array
    data = np.array(img, dtype=np.float32)

    print(f"  Dimensions: {data.shape[1]} x {data.shape[0]}")
    print(f"  Value range: {data.min():.1f} - {data.max():.1f}")

    return data


def normalize_to_uint16(data: np.ndarray) -> np.ndarray:
    """Normalize float data to uint16 range (0-65535)."""
    min_val = data.min()
    max_val = data.max()

    if max_val - min_val < 1e-6:
        print("  Warning: Flat heightmap, all values will be 0")
        return np.zeros(data.shape, dtype=np.uint16)

    # Normalize to 0-1, then scale to 0-65535
    normalized = (data - min_val) / (max_val - min_val)
    scaled = (normalized * 65535).astype(np.uint16)

    print(f"  Normalized to uint16: {scaled.min()} - {scaled.max()}")

    return scaled


def split_into_tiles(data: np.ndarray, tile_size: int) -> dict:
    """Split heightmap into tiles, padding if necessary."""
    height, width = data.shape

    # Calculate number of tiles
    tiles_x = (width + tile_size - 1) // tile_size
    tiles_y = (height + tile_size - 1) // tile_size

    print(f"  Splitting into {tiles_x} x {tiles_y} = {tiles_x * tiles_y} tiles")

    # Pad image to exact tile boundaries
    padded_width = tiles_x * tile_size
    padded_height = tiles_y * tile_size

    if padded_width != width or padded_height != height:
        print(f"  Padding from {width}x{height} to {padded_width}x{padded_height}")
        padded = np.zeros((padded_height, padded_width), dtype=data.dtype)
        padded[:height, :width] = data
        # Edge-extend padding
        if padded_width > width:
            padded[:height, width:] = data[:, -1:].repeat(padded_width - width, axis=1)
        if padded_height > height:
            padded[height:, :width] = data[-1:, :].repeat(padded_height - height, axis=0)
        if padded_width > width and padded_height > height:
            padded[height:, width:] = data[-1, -1]
        data = padded

    tiles = {}
    for ty in range(tiles_y):
        for tx in range(tiles_x):
            x_start = tx * tile_size
            y_start = ty * tile_size
            tile_data = data[y_start:y_start + tile_size, x_start:x_start + tile_size]
            tiles[(tx, ty)] = tile_data

    return tiles


def write_tile(tile_data: np.ndarray, output_path: str):
    """Write tile data as raw binary uint16."""
    # Ensure correct dtype
    if tile_data.dtype != np.uint16:
        tile_data = tile_data.astype(np.uint16)

    # Write as raw binary (little-endian is default for numpy)
    tile_data.tofile(output_path)


def generate_preview(data: np.ndarray, tiles: dict, tile_size: int, output_path: str):
    """Generate a preview image with tile grid overlay."""
    try:
        from PIL import ImageDraw
    except ImportError:
        print("  Skipping preview (PIL.ImageDraw not available)")
        return

    # Scale down for preview
    max_preview_size = 2048
    scale = min(1.0, max_preview_size / max(data.shape))

    preview_height = int(data.shape[0] * scale)
    preview_width = int(data.shape[1] * scale)

    # Normalize to 0-255 for preview
    normalized = ((data - data.min()) / (data.max() - data.min() + 1e-6) * 255).astype(np.uint8)

    img = Image.fromarray(normalized, mode='L')
    img = img.resize((preview_width, preview_height), Image.Resampling.LANCZOS)
    img = img.convert('RGB')

    # Draw tile grid
    draw = ImageDraw.Draw(img)
    scaled_tile = int(tile_size * scale)

    tiles_x = (data.shape[1] + tile_size - 1) // tile_size
    tiles_y = (data.shape[0] + tile_size - 1) // tile_size

    for tx in range(tiles_x + 1):
        x = int(tx * scaled_tile)
        if x < preview_width:
            draw.line([(x, 0), (x, preview_height)], fill=(255, 0, 0), width=1)

    for ty in range(tiles_y + 1):
        y = int(ty * scaled_tile)
        if y < preview_height:
            draw.line([(0, y), (preview_width, y)], fill=(255, 0, 0), width=1)

    img.save(output_path)
    print(f"  Preview saved: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description="Convert heightmap image to terrain tiles",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__
    )
    parser.add_argument("input", help="Input heightmap image (PNG, JPG, TIFF, etc.)")
    parser.add_argument("-o", "--output-dir", default="../res/terrain/height/",
                        help="Output directory for tiles (default: ../res/terrain/height/)")
    parser.add_argument("-t", "--tile-size", type=int, default=512,
                        help="Tile size in pixels (default: 512)")
    parser.add_argument("--preview", action="store_true",
                        help="Generate preview image with tile grid")
    parser.add_argument("--dry-run", action="store_true",
                        help="Show what would be done without writing files")

    args = parser.parse_args()

    # Resolve paths
    input_path = Path(args.input).expanduser().resolve()
    output_dir = Path(args.output_dir).expanduser()

    if not input_path.exists():
        print(f"Error: Input file not found: {input_path}")
        sys.exit(1)

    print(f"Terrain Heightmap Bake Tool")
    print(f"===========================")
    print(f"Input: {input_path}")
    print(f"Output: {output_dir}")
    print(f"Tile size: {args.tile_size}x{args.tile_size}")
    print()

    # Load and process heightmap
    heightmap = load_heightmap(str(input_path))
    heightmap_uint16 = normalize_to_uint16(heightmap)
    tiles = split_into_tiles(heightmap_uint16, args.tile_size)

    print()

    if args.dry_run:
        print("Dry run - would write:")
        for (tx, ty), tile_data in sorted(tiles.items()):
            tile_path = output_dir / f"tile_{tx}_{ty}.bin"
            size_kb = tile_data.nbytes / 1024
            print(f"  {tile_path} ({size_kb:.0f} KB)")
        total_mb = sum(t.nbytes for t in tiles.values()) / (1024 * 1024)
        print(f"\nTotal: {len(tiles)} tiles, {total_mb:.1f} MB")
        return

    # Create output directory
    output_dir.mkdir(parents=True, exist_ok=True)
    print(f"Writing {len(tiles)} tiles to {output_dir}...")

    # Write tiles
    for (tx, ty), tile_data in sorted(tiles.items()):
        tile_path = output_dir / f"tile_{tx}_{ty}.bin"
        write_tile(tile_data, str(tile_path))

    total_mb = sum(t.nbytes for t in tiles.values()) / (1024 * 1024)
    print(f"  Written {len(tiles)} tiles ({total_mb:.1f} MB)")

    # Generate preview if requested
    if args.preview:
        preview_path = output_dir / "preview.png"
        generate_preview(heightmap, tiles, args.tile_size, str(preview_path))

    print()
    print("Done!")
    print()
    print("To use with terrain system, update TerrainInit.lua:")
    tiles_x = (heightmap.shape[1] + args.tile_size - 1) // args.tile_size
    tiles_y = (heightmap.shape[0] + args.tile_size - 1) // args.tile_size
    print(f"  mapWidth = {tiles_x},")
    print(f"  mapHeight = {tiles_y},")
    print(f"  tileSize = {args.tile_size},")


if __name__ == "__main__":
    main()
