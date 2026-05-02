"""Lockchip cipher implementation for Pump It Up Exceed data decryption."""

CHIP_TRANSFORM = [0xF0, 0x78, 0xF9, 0xFD, 0x1C, 0x20, 0xC2, 0x02]

INITIAL_SBOX = [0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x7F]


class LockchipCipher:
    def __init__(self):
        self.state = 0xFC

    def _apply_initial_sbox(self):
        r = 0
        for i in range(8):
            if self.state & (1 << i):
                r ^= INITIAL_SBOX[i]
        self.state = r & 0xFF

    def _compute_sbox_coef(self, sel, bit):
        if sel == 0:
            return CHIP_TRANSFORM[bit]
        r = self._compute_sbox_coef((sel - 1) & 7, (bit - 1) & 7)
        r = ((r << 1) | (((r >> 7) ^ (r >> 6)) & 1)) & 0xFF
        if bit != 7:
            return r
        return (r ^ self._compute_sbox_coef(sel, 0)) & 0xFF

    def _apply_bit_sbox(self, bit):
        r = 0
        for i in range(8):
            if self.state & (1 << i):
                r ^= self._compute_sbox_coef(bit, i)
        self.state = r & 0xFF

    def step(self, data):
        result = 0
        for bit in range(8):
            if bit == 0:
                self._apply_initial_sbox()
            result ^= ((self.state >> bit) & 1) << bit
            if not ((data >> bit) & 1):
                self._apply_bit_sbox(bit)
        return result & 0xFF


def lockchip_transform(source: bytes) -> bytes:
    cipher = LockchipCipher()
    length = len(source)
    target = bytearray(length)

    last_result = cipher.step(source[0] ^ 0xFF)
    for i in range(1, length):
        result = cipher.step(source[i] ^ 0xFF)
        target[i - 1] = ((last_result >> 3) ^ (result << 5)) & 0xFF
        last_result = result

    last_byte = cipher.step(0)
    target[length - 1] = ((last_result >> 3) ^ (last_byte << 5)) & 0xFF

    return bytes(target)
