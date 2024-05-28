#include <fstream>
#include <iostream>

int main() {
  std::ifstream file0("a/bit0.bin");
  std::ifstream file1("a/bit1.bin");
  std::ifstream file2("a/bit2.bin");
  std::ifstream file3("a/bit3.bin");
  std::ifstream file4("a/bit4.bin");
  std::ifstream file5("a/bit5.bin");
  std::ifstream file6("a/bit6.bin");
  std::ifstream file7("a/bit7.bin");

  uint8_t array[0x200000] = {0};
  uint8_t bit;

  for (size_t i = 0; i < 0x200000; i++) {
    char temp;
    
    file0.read(&temp, sizeof(uint8_t));
    if (temp == '1') array[i] |= 0b00000001;
    
    file1.read(&temp, sizeof(uint8_t));
    if (temp == '1') array[i] |= 0b00000010;
    
    file2.read(&temp, sizeof(uint8_t));
    if (temp == '1') array[i] |= 0b00000100;
    
    file3.read(&temp, sizeof(uint8_t));
    if (temp == '1') array[i] |= 0b00001000;
    
    file4.read(&temp, sizeof(uint8_t));
    if (temp == '1') array[i] |= 0b00010000;
    
    file5.read(&temp, sizeof(uint8_t));
    if (temp == '1') array[i] |= 0b00100000;
    
    file6.read(&temp, sizeof(uint8_t));
    if (temp == '1') array[i] |= 0b01000000;
    
    file7.read(&temp, sizeof(uint8_t));
    if (temp == '1') array[i] |= 0b10000000;
  }

  file0.close();
  file1.close();
  file2.close();
  file3.close();
  file4.close();
  file5.close();
  file6.close();
  file7.close();

  std::ofstream outputFile("output.bin", std::ios::binary);
  outputFile.write(reinterpret_cast<char*>(array), sizeof(array));
  outputFile.close();

  return 0;
}
