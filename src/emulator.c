#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "emulator.h"

#define TEST_CLOCK 1000000 // May be lowered for easier debugging

// Helper function to print register contents for debugging
void print_registers(int *registers) {
  printf("IP: %d ", registers[IP]);
  for (int i = 0; i < 5; i++) {
    printf("R%d: %d ", i, registers[i]);
  }
  printf("\n");
}

// Helper function to print instruction information for debugging
void print_instruction(Instruction inst) {
  printf("%s src=%d dst=%d imm=%d\n", instruction_names[inst.op], inst.src, inst.dst, inst.immediate);
}

void fetch_instruction(int32_t * memory, int *registers) {
  // Load instruction pointed to by IP into the IR register
  registers[IR] = memory[registers[IP]];
}

Instruction decode_instruction(int32_t reg) {
  // Return the decoded form of the IR register
  return *(Instruction *)&reg;
}

int tick(int32_t * memory, int32_t * registers) {
  // Emulates one CPU clock tick
  fetch_instruction(memory, registers);
  Instruction inst = decode_instruction(registers[IR]);

  // Print the fetched instruction for debugging
  print_instruction(inst);

  // Execute the instruction
  if (inst.op == Halt) {
    return 0; // Halt the processor
  } else if (inst.op == LoadImmediate) {
    registers[inst.dst] = inst.immediate;
  } else if (inst.op == Add) {
    registers[inst.dst] += registers[inst.src];
  } else if (inst.op == AddImmediate) {
    registers[inst.dst] = registers[inst.src] + inst.immediate;
  } else if (inst.op == And) {
    registers[inst.dst] &= registers[inst.src];
  } else if (inst.op == Or) {
    registers[inst.dst] |= registers[inst.src];
  } else if (inst.op == Xor) {
    registers[inst.dst] ^= registers[inst.src];
  } else if (inst.op == Jump) {
    registers[IP] = inst.immediate;
    return 1;
  } else if (inst.op == BranchOnEqual) {
    if (registers[inst.src] == registers[inst.dst]) {
      registers[IP] = inst.immediate;
      return 1;
    }
  } else {
    printf("Unknown operation: %d\n", inst.op);
    exit(1);
  }

  // Increment the instruction pointer (IP) to point to the next instruction
  registers[IP]++;

  return 1; // Instruction executed
}
/*
 * It is not recommended to modify any code below this point
 */

void update_display(int32_t * memory) {
  // 16x3 character mode display
  // Update output based on memory values
  char lines[3][17];
  char c;

  // Check the display mode
  if (memory[64] == 1) {
    memory[64] = 0;
    // Prebuffer text to minimize flicker
    for (int y = 0; y < 3; y++) {
      for (int x = 0; x < 16; x++) {
        c = (char) memory[80 + y * 16 + x];
        if (c == 0) {
          c = ' ';
        }
        lines[y][x] = c;
      }
      lines[y][16] = 0;
    }
    printf("\033[2J"
           "\033[H"
           "##################\n" "#%s#\n" "#%s#\n" "#%s#\n" "##################\n\n", lines[0], lines[1], lines[2]);
  }
}

void emulate_system(int32_t * memory, int *registers, int clock_rate) {
  int clock_interval_us = 1000000 / clock_rate;

  // Loop until the next instruction is halt (0x00000000)
  while (tick(memory, registers)) {
    update_display(memory);
    usleep(clock_interval_us);
  }
}

void load_rom(char *filename, int32_t * memory) {
  FILE *rom = fopen(filename, "rb");
  if (!rom) {
    printf("Error loading %s\n", filename);
    exit(1);
  }
  fread(memory, 4, 256, rom);
  fclose(rom);
}

/* 
Test code 
*/

int _test_int_equal_result;

#define test_int_equal(exp, result) \
printf("Testing %s == %d\n", #exp, result); \
_test_int_equal_result = exp; \
if ((_test_int_equal_result) != result) { \
  printf("Test for %s == %d FAILED (got %d)\n", #exp, result, _test_int_equal_result); \
  exit(1); \
}

int main() {
  // Part 1 (Addition and LoadImmediate)

  int32_t registers[7] = { 0, 0, 0, 0, 0, 0, 0 };
  printf("Instruction Fetch Tests\n");

  int32_t fetch_instructions[256] = {0,1,42};

  registers[IP] = 0;
  fetch_instruction(fetch_instructions, registers);
  test_int_equal(registers[IP], 0);
  test_int_equal(registers[IR], 0);
  registers[IP] = 1;
  fetch_instruction(fetch_instructions, registers);
  test_int_equal(registers[IP], 1);
  test_int_equal(registers[IR], 1);
  registers[IP] = 2;
  fetch_instruction(fetch_instructions, registers);
  test_int_equal(registers[IP], 2);
  test_int_equal(registers[IR], 42);

  printf("Halt Tests\n");
  Instruction halt_instructions[256] = {
    {Halt, 0, 0, 0}
  };
  memset(registers, 0, sizeof(registers));
  emulate_system((void *) halt_instructions, registers, TEST_CLOCK);
  test_int_equal(registers[IP], 0);
  test_int_equal(registers[R0], 0);
  test_int_equal(registers[R1], 0);
  test_int_equal(registers[R2], 0);

  printf("Load Immediate Tests\n");
  Instruction load_imm_instructions[256] = {
    {LoadImmediate, R0, 0, 1},
    {LoadImmediate, R1, 0, 80},
    {LoadImmediate, R2, 0, 13}
  };
  memset(registers, 0, sizeof(registers));
  emulate_system((void *) load_imm_instructions, registers, TEST_CLOCK);
  test_int_equal(registers[IP], 3);
  test_int_equal(registers[R0], 1);
  test_int_equal(registers[R1], 80);
  test_int_equal(registers[R2], 13);

  printf("Add Tests\n");
  Instruction add_instructions[256] = {
    {LoadImmediate, R0, 0, 42},
    {LoadImmediate, R1, 0, 100},
    {Add, R1, R0, 0}
  };
  memset(registers, 0, sizeof(registers));
  emulate_system((void *) add_instructions, registers, TEST_CLOCK);
  test_int_equal(registers[IP], 3);
  test_int_equal(registers[R0], 42);
  test_int_equal(registers[R1], 142);
  test_int_equal(registers[R2], 0);

  printf("Add Immediate Tests\n");
  Instruction addi_instructions[256] = {
    {AddImmediate, R0, R0, 1},
    {AddImmediate, R0, R0, 2},
    {AddImmediate, R1, R1, 21},
    {AddImmediate, R1, R1, 30},
    {AddImmediate, R2, R1, 1}
  };
  memset(registers, 0, sizeof(registers));
  emulate_system((void *) addi_instructions, registers, TEST_CLOCK);
  test_int_equal(registers[IP], 5);
  test_int_equal(registers[R0], 3);
  test_int_equal(registers[R1], 51);
  test_int_equal(registers[R2], 52);

  // Part 2 (Control flow and additional ALU instructions)
  printf("Bitwise Tests\n");
  Instruction bitwise_instructions[256] = {
    {LoadImmediate, R0, 0, 1},
    {LoadImmediate, R1, 0, 1},
    {And, R1, R0, 0},
    {LoadImmediate, R2, 0, 0},
    {Or, R2, R0, 0},
    {LoadImmediate, R3, 0, 0},
    {Xor, R3, R0, 0},
    {Xor, R3, R0, 0}
  };
  memset(registers, 0, sizeof(registers));
  emulate_system((void *) bitwise_instructions, registers, TEST_CLOCK);
  test_int_equal(registers[IP], 8);
  test_int_equal(registers[R0], 1);
  test_int_equal(registers[R1], 1);
  test_int_equal(registers[R2], 1);
  test_int_equal(registers[R3], 0);

  printf("Jump Tests\n");
  Instruction jump_instructions[256] = {
    {LoadImmediate, R0, 0, 1},
    {Jump, 0, 0, 3},
    {LoadImmediate, R1, 0, 2}
  };
  memset(registers, 0, sizeof(registers));
  emulate_system((void *) jump_instructions, registers, TEST_CLOCK);
  test_int_equal(registers[IP], 3);
  test_int_equal(registers[R0], 1);
  test_int_equal(registers[R1], 0);
  test_int_equal(registers[R2], 0);

  printf("BranchOnEqual equal test\n");
  Instruction beq_instructions[256] = {
    {LoadImmediate, R0, 0, 1},
    {LoadImmediate, R1, 0, 1},
    {BranchOnEqual, R1, R0, 4}
  };
  memset(registers, 0, sizeof(registers));
  emulate_system((void *) beq_instructions, registers, TEST_CLOCK);
  test_int_equal(registers[IP], 4);
  test_int_equal(registers[R0], 1);
  test_int_equal(registers[R1], 1);
  test_int_equal(registers[R2], 0);

  printf("BranchOnEqual not equal Test\n");
  Instruction beq2_instructions[256] = {
    {LoadImmediate, R0, 0, 1},
    {LoadImmediate, R1, 0, 2},
    {BranchOnEqual, R1, R0, 4}
  };
  memset(registers, 0, sizeof(registers));
  emulate_system((void *) beq2_instructions, registers, TEST_CLOCK);
  test_int_equal(registers[IP], 3);
  test_int_equal(registers[R0], 1);
  test_int_equal(registers[R1], 2);
  test_int_equal(registers[R2], 0);

  printf("Extended Tests\n");
  Instruction ext_instructions[256] = {
    {LoadImmediate, R0, 0, 1},
    {LoadImmediate, R1, 0, 12},
    {LoadImmediate, R2, 0, 100},
    {Add, R1, R0, 0},
    {Add, R1, R2, 0},
    {Add, R1, R2, 0},
    {LoadImmediate, R0, 0, -12},
    {Add, R1, R0, 0},
    {LoadImmediate, R0, 0, 1},
    {LoadImmediate, R2, 0, 3}
  };
  memset(registers, 0, sizeof(registers));
  emulate_system((void *) ext_instructions, registers, TEST_CLOCK);
  test_int_equal(registers[IP], 10);
  test_int_equal(registers[R0], 1);
  test_int_equal(registers[R1], 201);
  test_int_equal(registers[R2], 3);

  printf("Loop Test\n");
  Instruction loop_instructions[256] = {
    {LoadImmediate, R0, 0, 0}, // Stays zero
    {LoadImmediate, R1, 0, -1}, // Stays -1
    {LoadImmediate, R2, 0, 11}, // Loop counter
    {LoadImmediate, R3, 0, 3}, // Stays 3
    {LoadImmediate, R4, 0, 0}, // Counting by 3s
    // Decrement loop counter
    {Add, R2, R1, 0}, // Instruction 5
    {BranchOnEqual, R0, R2, 9}, // Exit loop
    // Increment output
    {Add, R4, R3, 0},
    {Jump, 0, 0, 5}
  };
  memset(registers, 0, sizeof(registers));
  emulate_system((void *) loop_instructions, registers, TEST_CLOCK);
  test_int_equal(registers[IP], 9);
  test_int_equal(registers[R0], 0);
  test_int_equal(registers[R1], -1);
  test_int_equal(registers[R2], 0);
  test_int_equal(registers[R3], 3);
  test_int_equal(registers[R4], 30);

  printf("All tests passed\n");
}
