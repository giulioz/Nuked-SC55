out_data = []

with open("tmpl_3.txt", "r") as file:
  lines = file.readlines()

  for line in lines:
    line = line.strip()
    if line:
      last_chars = line[-2:]
      nibble = int(last_chars, 16)
      if nibble <= 0xf:
        out_data.append(nibble)

with open("tmph_3.txt", "r") as file:
  lines = file.readlines()

  i = 0
  for line in lines:
    line = line.strip()
    if line:
      last_chars = line[-2:]
      nibble = int(last_chars, 16)
      if nibble <= 0xf:
        if i >= 0 and i < len(out_data):
          out_data[i] |= nibble << 4
        i += 1

with open("dump3.bin", "wb") as file:
  file.write(bytes(out_data[1:]))
