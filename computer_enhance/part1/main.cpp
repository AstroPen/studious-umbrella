
#include <common.h>
#include <push_allocator.h>
#include <unix_file_io.h>
#include <lstring.h>

#include "types.h"
#include "parser.cpp"
#include "logger.cpp"

bool has_odd_parity(u8 n) {
  return __builtin_parity(n);
}


void *get_register_ptr(ExecutionState *state, RegisterID reg_id) {
  switch (reg_id) {
    case REG_AL:
      return &state->reg_a.low_bits;
    case REG_CL:
      return &state->reg_c.low_bits;
    case REG_DL:
      return &state->reg_d.low_bits;
    case REG_BL:
      return &state->reg_b.low_bits;
    case REG_AH:
      return &state->reg_a.high_bits;
    case REG_CH:
      return &state->reg_c.high_bits;
    case REG_DH:
      return &state->reg_d.high_bits;
    case REG_BH:
      return &state->reg_b.high_bits;
    case REG_AX:
      return &state->reg_a.all_bits;
    case REG_CX:
      return &state->reg_c.all_bits;
    case REG_DX:
      return &state->reg_d.all_bits;
    case REG_BX:
      return &state->reg_b.all_bits;
    case REG_SP:
      return &state->reg_sp;
    case REG_BP:
      return &state->reg_bp;
    case REG_SI:
      return &state->reg_si;
    case REG_DI:
      return &state->reg_di;
    default:
      INVALID_SWITCH_CASE(reg_id); return NULL;
  }
}

u8 *get_valid_address_or_null(ExecutionState *state, u16 addr) {
  if (addr < state->end_of_instructions) return NULL;
  if (addr >= state->mem_size) return NULL;
  return state->memory+addr;
}

template <typename T>
T *get_dest_location(ExecutionState *state, RmUnion dest, bool wide) {
  u16 addr;
  switch (dest.kind) {
    case RM_REG:
      return (T*)get_register_ptr(state, get_reg_id(dest.reg.reg, wide));
    case RM_DIRECT_ADDR:
      return (T*)get_valid_address_or_null(state, dest.direct_addr);
    case RM_ADDR_FROM_REG:
      addr = *((u16*)get_register_ptr(state, RegisterID(dest.addr.r1)));
      addr += dest.addr.displacement;
      return (T*)get_valid_address_or_null(state, addr);
    case RM_ADDR_ADD:
      addr = *((u16*)get_register_ptr(state, RegisterID(dest.addr.r1)));
      addr += *((u16*)get_register_ptr(state, RegisterID(dest.addr.r2)));
      addr += dest.addr.displacement;
      return (T*)get_valid_address_or_null(state, addr);
    case RM_IMMEDIATE:
      FAILURE("An immediate is not a valid MOV destination."); return NULL;
    default:
      INVALID_SWITCH_CASE(dest.kind); return NULL;
  }
}

template <typename T>
T get_src_value(ExecutionState *state, RmUnion src, bool wide) {
  if (src.kind == RM_IMMEDIATE) return (T) src.immediate.immediate;
  return *get_dest_location<T>(state, src, wide);
}

template <typename T>
void exec_binary_instruction(ExecutionState *state, Instruction inst, u16 op_flags) {
  char dest_str[64];
  write_src_or_dest(dest_str, sizeof(dest_str), inst.dest, inst.wide);
  T *loc = get_dest_location<T>(state, inst.dest, inst.wide);
  T old_val = *loc;
  T src_val = get_src_value<T>(state, inst.src, inst.wide);
  T new_val;
  switch (inst.cmd) {
    case OP_ADD: new_val = old_val + src_val; break;
    case OP_SUB: new_val = old_val - src_val; break;
    case OP_CMP: new_val = old_val - src_val; break;
    default: new_val = src_val;
  }
  if (op_flags & INST_WRITE_TO_DEST) {
    *loc = new_val;
    if (state->print_value_updates) printf("%s:0x%x->0x%x ", dest_str, old_val, new_val);
  }

  if (state->print_ip_updates) printf("ip:0x%x->0x%x ", inst.initial_ip, get_ip(state));

  if (op_flags & INST_SET_FLAGS) {
    u8 old_flags = state->flags;
    u8 new_flags = 0;
    if (!new_val)                         FLAG_SET(new_flags, AFLAG_ZERO);
    if (inst.wide  && (new_val & 0x8000)) FLAG_SET(new_flags, AFLAG_SIGN);
    if (!inst.wide && (new_val & 0x80))   FLAG_SET(new_flags, AFLAG_SIGN);
    if (!has_odd_parity(new_val))         FLAG_SET(new_flags, AFLAG_PARITY);

    state->flags = new_flags;

    if (state->print_flag_updates && old_flags != state->flags) {
      printf("flags:");
      print_aflags_str(old_flags);
      printf("->");
      print_aflags_str(state->flags);
      printf(" ");
    }
  }
}

void exec_jump_instruction(ExecutionState *state, Instruction inst, u16 op_flags) {
  bool condition_met = false;
  u16 *cx_ptr = (u16*)get_register_ptr(state, REG_CX);
  if (FLAG_TEST(op_flags, INST_DECREMENT_CX)) (*cx_ptr)--;

  if (FLAG_TEST(op_flags, INST_ON_EQUAL)) {
    condition_met = !!FLAG_TEST(state->flags, AFLAG_ZERO);
  } else if (FLAG_TEST(op_flags, INST_ON_CX_NOT_ZERO)) {
    condition_met = (*cx_ptr) != 0;
  } else {
    FAILURE("Only JE and JNE are implemented. Flags: ", op_flags);
  }
  if (FLAG_TEST(op_flags, INST_NOT)) condition_met = !condition_met;

  if (condition_met) {
    state->stream.cursor += inst.src.jump_offset;
  }
  if (state->print_ip_updates) printf("ip:0x%x->0x%x ", inst.initial_ip, get_ip(state));
}

void exec_instruction(ExecutionState *state, Instruction inst) {
  ASSERT(inst.cmd < OP_COUNT);

  u16 op_flags = get_op_def_flags(inst.cmd);
  if (op_flags & INST_BINARY) {
    if (inst.wide) exec_binary_instruction<u16>(state, inst, op_flags);
    else exec_binary_instruction<u8>(state, inst, op_flags);
    return;
  }
  if (op_flags & INST_JUMP) {
    exec_jump_instruction(state, inst, op_flags);
    return;
  }
  FAILURE("Only binary instructions are currently handled in exec mode.");
}

void usage(char *program_name) {
  printf("usage: %s machine_code_file [-dump] [-exec]\n", program_name);
  exit(0);
}

int main(int argc, char *argv[]) {
  if (argc < 2 || argc > 4) usage(argv[0]);
  char *filename = argv[1];
  bool exec = false;
  bool dump = false;
  bool clock = false;
  for (int i = 2; i < argc; i++) {
    if (!strcmp(argv[i], "-exec")) {
      exec = true;
    } else if (!strcmp(argv[i], "-dump")) {
      dump = true;
    } else if (!strcmp(argv[i], "-clock")) {
      clock = true;
    } else {
      usage(argv[0]);
    }
  }

  if (!exec) {
    printf("; Disasembling %s.\n", filename);
    printf("bits 16\n");
  } else {
    printf("--- %s execution ---\n", filename);
  }

  u32 max_mem_size = 0x100000; // 1024*1024
  PushAllocator allocator = new_push_allocator(max_mem_size);
  File file = open_file(filename);
  int err = read_entire_file(&file, &allocator);
  ASSERT(!err);
  close_file(&file);

  ExecutionState state = ExecutionState{
    .do_execution = exec,
    .memory = allocator.memory,
    .mem_size = max_mem_size,
    .end_of_instructions = allocator.bytes_allocated,
    .print_flag_updates = true,
    .print_ip_updates = true,
    .print_value_updates = true,
    .stream = get_stream(file.buffer, file.size),
  };
  u32 total_clocks = 0;

  while (has_remaining(&state.stream)) {
    Instruction inst = parse_next_instruction(&state);
    print_instruction(inst);
    if (clock || exec) {
      printf(" ; ");
      if (clock) total_clocks = print_instruction_clocks(inst, total_clocks);
      if (exec) exec_instruction(&state, inst);
    }
    printf("\n");
  }

  RegisterID print_order[] = {
    REG_AX,
    REG_BX,
    REG_CX,
    REG_DX,
    REG_SP,
    REG_BP,
    REG_SI,
    REG_DI,
  };
  if (exec) {
    printf("\nFinal registers:\n");
    for (int i = 0; i < count_of(print_order); i++) {
      RegisterID id = print_order[i];
      u16 *val = (u16 *)get_register_ptr(&state, id);
      printf("      %s: 0x%04x (%d)\n", register_names[id], *val, *val);
    }
    if (state.flags) {
      printf("   flags: ");
      print_aflags_str(state.flags);
      printf("\n");
    }
    printf("\n");

    if (dump) {
      FILE *out_file = fopen("out.data", "wb");
      ASSERT(out_file);
      fwrite(state.memory, 1, state.mem_size, out_file);
      fclose(out_file);
    }
  }
  
  return 0;
}

