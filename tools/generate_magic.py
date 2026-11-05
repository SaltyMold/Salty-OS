#!/usr/bin/env python3
magic = int.from_bytes(input("Enter word : ").strip().encode("ascii"), "little")
print(f"Magic : 0x{magic:08X}")