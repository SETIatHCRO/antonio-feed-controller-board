import sys

crc = 0

crc_table = (0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
             0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef)

if (not(len(sys.argv) == 2)):
    sys.exit(1)

for j in range(0, len(sys.argv[1]), 2):
    digits = sys.argv[1][j:j+2]
    data = int(digits, 16)
    i = (crc >> 12) ^ (data >> 4)
    crc = crc_table[i & 0x0f] ^ (crc << 4)
    i = (crc >> 12) ^ (data >> 0)
    crc = crc_table[i & 0x0f] ^ (crc << 4)

print ("{0:04x}".format(crc & 0xffff))


