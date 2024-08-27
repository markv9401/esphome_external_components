import sys
import re


f = open(sys.argv[1], 'r')
lines = f.readlines()

for l in lines:
  if not "ACK RS:" in l:
    continue
  l = l.strip()
  l = re.sub(r'.+ACK RS:([\w\d,]+)\\r\\n"', r'\1', l)
  l = l.replace(',', ' ')

  # 00 80 junk
  # C4 - in motion  |  E6 - stopped   |  A2 - end state
  # XX percentage (if opening, offset by 128)
  # YY speed (?)
  # ZZ ??
  # FF FF FF junk

  hd = l.split(' ')
  opening = False

  percentage = int(hd[3], 16)
  # magic...
  if percentage > 100:
    percentage = percentage - 128
  speed = int(hd[4], 16)
  unknown = int(hd[5], 16)
  print(f"{l}   percentage: {percentage}  speed: {speed}   unknown: {unknown}")

