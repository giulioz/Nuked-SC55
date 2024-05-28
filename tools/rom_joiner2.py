out_data = []

with open("tmpl_tt.txt", "r") as file:
  lines = file.readlines()

  for line in lines:
    line = line.strip()
    if line:
      last_chars = line[-2:]
      part = int(last_chars, 16)
      out_data.append(part)

with open("tmph_tt.txt", "r") as file:
  lines = file.readlines()

  i = 0
  for line in lines:
    line = line.strip()
    if line:
      last_chars = line[-2:]
      part = int(last_chars, 16)
      if i >= 0 and i < len(out_data):
        if (out_data[i] & 0b1111110) != (part & 0b1111110):
          print("error!!", hex(i))
        out_data[i] |= part
      i += 1

with open("dump_tt.bin", "wb") as file:
  file.write(bytes(out_data[1:]))
