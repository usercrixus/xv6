"""
used to modify a binary file, specifically a boot block, by appending padding and a signature.
"""

import sys

filename: str = sys.argv[1]

with open(filename, 'rb') as f:
  # reads up to 1000 bytes from the file, or fewer if the end of the file is reached.
  buf: bytes = f.read(1000)
  n: int = len(buf)

# Error
if n > 510:
  print(f"boot block too large: {n} bytes (max 510)", file=sys.stderr)
  sys.exit(1)

print(f"boot block is {n} bytes (max 510)", file=sys.stdout)

# appends padding by adding null bytes (b'\x00') to the buf to reach a length of 510 bytes.
buf += b'\x00' * (510 - n)
buf += b'\x55\xAA'

with open(filename, 'wb') as f:
  f.write(buf)