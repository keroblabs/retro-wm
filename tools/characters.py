for i in range(0x20, 0x80, 8):
    chars = ''.join('█' if c == 0x7F else chr(c) for c in range(i, i + 8))
    print(f"0x{i:02X}: {chars}")

