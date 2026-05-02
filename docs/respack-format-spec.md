# RESPACK Archive Format Specification

## Overview

RESPACK is a compressed and encrypted archive format used in Pump It Up arcade data files (`.DAT` extension). It contains multiple files — typically BGA animation data, sprite definitions, and texture images.

## Archive Structure

```
[Archive Header]       8 + 16 bytes
[File Info Table]      300 bytes × file_count
[File Data Blocks]     variable
```

## Archive Header

```
Offset  Size    Description
0x00    8       Magic: "RESPACK\x1A"
0x08    4       Version (uint32, little-endian)
0x0C    4       Number of files (uint32, little-endian)
0x10    4       Total uncompressed size (uint32, little-endian)
0x14    4       Total compressed size (uint32, little-endian)
```

Total: 24 bytes.

## File Info Table

Each entry is 300 bytes. The entire table is XOR-encrypted with a repeating 256-byte key.

### File Info Entry (after decryption)

```
Offset  Size    Description
0x000   260     Filename (null-terminated string, 260-byte buffer)
0x104   4       Name hash (uint32)
0x108   4       Compression flags (uint32)
0x10C   4       Uncompressed size (uint32)
0x110   4       Compressed size (uint32)
0x114   4       Adler-32 checksum (uint32)
0x118   16      Key seed (16 bytes, used for per-file decryption)
0x128   4       Data offset (int32, relative to start of data section)
```

### File Info Decryption

The file info table is decrypted by XOR with a static 256-byte key, cycling per entry:

```python
for entry_idx in range(file_count):
    for byte_idx in range(300):
        info_bytes[offset + byte_idx] ^= file_info_key[byte_idx % 256]
```

## Per-File Data Encryption

Each file's compressed data is encrypted with a per-file key derived from the 16-byte key seed stored in the file info entry.

### Key Derivation

The 16-byte key seed undergoes a lockchip transformation (same algorithm as ENC2 format) to produce the final 16-byte decryption key.

### Decryption

```python
key = lockchip_transform(key_seed)
for i in range(compressed_size):
    data[i] ^= key[i % 16]
    key[i % 16] = (key[i % 16] + 0x54) & 0xFF
```

Note: the key byte rotates by adding `0x54` after each byte is decrypted.

### Decompression

After decryption, decompress using zlib (`uncompress`).

### Verification

Compute Adler-32 over the decompressed data and compare against the stored checksum.

## Contained File Types

| Type | Description |
|------|-------------|
| `.bga` | BGA binary animation (see bga-binary-format-spec.md) |
| `.spr` | SPR sprite definition (see spr-format-spec.md) |
| `.tga` | TGA texture image |

## Extraction Summary

1. Read and validate `RESPACK\x1A` magic
2. Parse archive header for file count
3. Read and decrypt file info table (XOR with static 256-byte key)
4. For each file entry:
   a. Seek to data offset
   b. Read `compressed_size` bytes
   c. Derive decryption key from seed via lockchip transform
   d. Decrypt data (XOR with rotating key)
   e. Decompress with zlib
   f. Verify Adler-32 checksum
   g. Write extracted file
