
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

