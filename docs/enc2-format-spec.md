# ENC2 Encrypted Container Format Specification

## Overview

ENC2 is an encrypted container format used in Pump It Up arcade data files. It wraps standard media files (PNG images, MP3 audio) with a custom encryption layer. Files using this format include `.PNZ` (encrypted PNG) and `.AUD` (encrypted MP3).

## File Structure

```
Offset  Size    Description
0x00    4       Magic: "ENC2" (0x45 0x4E 0x43 0x32)
0x04    128     Reserved header (unused)
0x84    4       Payload size (uint32, little-endian)
0x88    4       Checksum offset (uint32, little-endian)
```

Total header size: 140 bytes.

After the header, the encrypted payload follows immediately.

## Checksum and Cipher Table

At position `current_pos + checksum_offset`:
- 4 bytes: Adler-32 checksum of decrypted payload (uint32, little-endian)
- 1024 bytes: Cipher substitution table

The cipher table undergoes a lockchip transformation (see below) in 16-byte chunks before use.

## Encryption Algorithm

Each payload byte is encrypted with a combination of bit-reversal and XOR substitution.

### Bit Reversal

```
def reverse_bits(byte):
    result = 0
    for i in range(8):
        result |= ((byte >> i) & 1) << (7 - i)
    return result
```

### Decryption

```
key_index = 0
for i in range(payload_size):
    byte = encrypted_data[i]
    byte = reverse_bits(byte)
    byte ^= cipher_table[key_index % 1024]
    decrypted[i] = byte
    key_index += 1
```

### Lockchip Table Transformation

The 1024-byte cipher table is transformed in 16-byte chunks using a stateful LFSR-based cipher:

1. Initial state: `0xFC`
2. For each 16-byte chunk of the table:
   - Apply S-box substitution to state
   - XOR each byte in the chunk with the derived key byte
   - Advance state via bit-manipulation feedback

The S-box is a 256-entry lookup table derived from the hardware lockchip.

## Verification

After decryption, compute Adler-32 over the decrypted payload and compare against the stored checksum.

## Payload Types

| Extension | Payload |
|-----------|---------|
| `.PNZ`    | Standard PNG image data |
| `.AUD`    | Standard MPEG Layer III (MP3) audio |

## Notes

- The decrypted payload can be written directly to a `.png` or `.mp3` file.
- The cipher table may vary between game versions.
