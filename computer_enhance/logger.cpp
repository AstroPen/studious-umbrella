
#include "types.h"

void write_src_or_dest(char *buffer, u64 bufsize, RmUnion data, bool wide) {
  char op = '+';
  s16 displacement = data.addr.displacement;
  char *explicit_size = "byte";
  u8 width_mod = 0;
  switch (data.kind) {
    case RM_IMMEDIATE:
      if (data.immediate.requires_explicit_size) {
        if (wide) {
          explicit_size = "word";
        }
        snprintf(buffer, bufsize, "%s %d", explicit_size, data.immediate.immediate);
      } else {
        snprintf(buffer, bufsize, "%d", data.immediate.immediate);
      }
      break;
    case RM_REG:
      if (wide) width_mod = 8;
      snprintf(buffer, bufsize, "%s", register_names[data.reg.reg + width_mod]);
      break;
    case RM_SEGMENT_REG:
      snprintf(buffer, bufsize, "%s", segment_register_names[data.sr.sr]);
      break;
    case RM_DIRECT_ADDR:
      snprintf(buffer, bufsize, "[%d]", data.direct_addr);
      break;
    case RM_ADDR_FROM_REG:
      if (displacement < 0) {
        displacement = -displacement;
        op = '-';
      }
      if (displacement) {
        snprintf(buffer, bufsize, "[%s %c %d]",
            register_names[data.addr.r1],
            op, displacement);
      } else {
        snprintf(buffer, bufsize, "[%s]", register_names[data.addr.r1]);
      }
      break;
    case RM_ADDR_ADD:
      if (displacement < 0) {
        displacement = -displacement;
        op = '-';
      }
      if (displacement) {
        snprintf(buffer, bufsize, "[%s + %s %c %d]",
            register_names[data.addr.r1],
            register_names[data.addr.r2],
            op, displacement);
      } else {
        snprintf(buffer, bufsize, "[%s + %s]",
            register_names[data.addr.r1],
            register_names[data.addr.r2]);
      }
      break;
    default:
      INVALID_SWITCH_CASE(data.kind);
  }
}

void print_binary_rm_instruction(Instruction data) {
  char dest_str[64];
  char src_str[64];
  ASSERT(data.dest.kind != RM_IMMEDIATE);
  write_src_or_dest(dest_str, 64, data.dest, data.wide);
  write_src_or_dest(src_str, 64, data.src, data.wide);
  printf("%s %s, %s", operator_names[data.cmd], dest_str, src_str);
}

void print_jump_instruction(Instruction inst) {
  printf("%s %d ", operator_names[inst.cmd], inst.src.jump_offset);
}

void print_instruction(Instruction data) {
  u8 def_flags = get_op_def_flags(data.cmd);
  if (def_flags & INST_BINARY) print_binary_rm_instruction(data);
  if (def_flags & INST_JUMP) print_jump_instruction(data);
}

void print_aflags_str(u8 flags) {
  if (flags & AFLAG_PARITY) printf("%c", 'P');
  if (flags & AFLAG_ZERO) printf("%c", 'Z');
  if (flags & AFLAG_SIGN) printf("%c", 'S');
}

u32 get_effective_address_clocks(RmUnion dest) {
  switch (dest.kind) { 
    case RM_DIRECT_ADDR: return 6;
    case RM_ADDR_FROM_REG: return dest.addr.displacement ? 9 : 5;
    case RM_ADDR_ADD:
      if (dest.addr.displacement) {
        if (dest.addr.r1 == REG_BP) {
          if (dest.addr.r2 == REG_DI) return 11;
          else return 12; // SI
        } else { // BX
          if (dest.addr.r2 == REG_SI) return 11;
          else return 12; // DI
        }
      } else {
        if (dest.addr.r1 == REG_BP) {
          if (dest.addr.r2 == REG_DI) return 7;
          else return 8; // SI
        } else { // BX
          if (dest.addr.r2 == REG_SI) return 7;
          else return 8; // DI
        }
      }
    default: INVALID_SWITCH_CASE(dest.kind);
  }
  return 1;
}

u32 get_instruction_clocks(Instruction inst) {
  switch (inst.cmd) {
    case OP_MOV:
      switch (inst.src.kind) {
        case RM_IMMEDIATE:
          switch (inst.dest.kind) {
            case RM_REG: return 4;
            case RM_DIRECT_ADDR:
            case RM_ADDR_FROM_REG:
            case RM_ADDR_ADD:
              return 10 + get_effective_address_clocks(inst.dest);
            default: INVALID_SWITCH_CASE(inst.dest.kind);
          }
        case RM_REG:
          switch (inst.dest.kind) {
            case RM_REG: return 2;
            case RM_DIRECT_ADDR:
            case RM_ADDR_FROM_REG:
            case RM_ADDR_ADD:
              return 9 + get_effective_address_clocks(inst.dest);
            default: INVALID_SWITCH_CASE(inst.dest.kind);
          }
        case RM_DIRECT_ADDR:
        case RM_ADDR_FROM_REG:
        case RM_ADDR_ADD:
          return 8 + get_effective_address_clocks(inst.src);
        default: INVALID_SWITCH_CASE(inst.src.kind);
      }
    case OP_ADD: 
      switch (inst.src.kind) {
        case RM_IMMEDIATE:
          switch (inst.dest.kind) {
            case RM_REG: return 4;
            case RM_DIRECT_ADDR:
            case RM_ADDR_FROM_REG:
            case RM_ADDR_ADD:
              return 17 + get_effective_address_clocks(inst.dest);
            default: INVALID_SWITCH_CASE(inst.dest.kind);
          }
        case RM_REG:
          switch (inst.dest.kind) {
            case RM_REG: return 3;
            case RM_DIRECT_ADDR:
            case RM_ADDR_FROM_REG:
            case RM_ADDR_ADD:
              return 16 + get_effective_address_clocks(inst.dest);
            default: INVALID_SWITCH_CASE(inst.dest.kind);
          }
        case RM_DIRECT_ADDR:
        case RM_ADDR_FROM_REG:
        case RM_ADDR_ADD:
          return 9 + get_effective_address_clocks(inst.src);
        default: INVALID_SWITCH_CASE(inst.src.kind);
      }
    default: INVALID_SWITCH_CASE(inst.src.kind);
  }
  return 1;
}

u32 print_instruction_clocks(Instruction inst, u32 total) {
  u32 n = get_instruction_clocks(inst);
  total += n;
  printf("Clocks: +%d = %d", get_instruction_clocks(inst), total);
  return total;
}


