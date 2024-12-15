#include <stdio.h>
#include <stdint.h>

int main() {
  // Create variables
  uint8_t myNum = 0xC4;
  uint8_t myNum2 = myNum;
  
  // Print variables
  printf("%u\n", myNum);
  printf("%X\n", myNum);
  printf("%d\n", (int8_t)myNum);
  printf("%u\n", (unsigned char)myNum);
  printf("%u\n", myNum2);
  printf("%X\n", myNum2);
  
  return 0;
}