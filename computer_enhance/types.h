#ifndef _TYPES_H_
#define _TYPES_H_

#include <common.h>
#include <lstring.h>

enum RegisterID : u8 {
  REG_AL = 0,
  REG_CL,
  REG_DL,
  REG_BL,
  REG_AH,
  REG_CH,
  REG_DH,
  REG_BH,
  REG_AX,
  REG_CX,
  REG_DX,
  REG_BX,
  REG_SP,
  REG_BP,
  REG_SI,
  REG_DI,

  REG_COUNT,
};

RegisterID get_reg_id(u8 reg, bool wide) {
  return RegisterID(wide ? reg + 8 : reg);
}

const char *register_names[REG_COUNT] = {
  "al",
  "cl",
  "dl",
  "bl",
  "ah",
  "ch",
  "dh",
  "bh",
  "ax",
  "cx",
  "dx",
  "bx",
  "sp",
  "bp",
  "si",
  "di",
};

enum SegmentRegisterId : u8 {
  SR_ES = 0,
  SR_CS,
  SR_SS,
  SR_DS,
  SR_COUNT,
};

const char *segment_register_names[SR_COUNT] = {
  "es",
  "cs",
  "ss",
  "ds",
};

enum AsmOperatorID : u8 {
  // NOTE: These 8 match the codes used for accumulator instructions.
  OP_ADD,
  OP_OR,
  OP_ADC,
  OP_SBB,
  OP_AND,
  OP_SUB,
  OP_XOR,
  OP_CMP,

  OP_MOV,

  OP_JO,
  OP_JNO,
  OP_JB,
  OP_JNB,
  OP_JE,
  OP_JNE,
  OP_JBE,
  OP_JNBE,
  OP_JS,
  OP_JNS,
  OP_JP,
  OP_JNP,
  OP_JL,
  OP_JNL,
  OP_JLE,
  OP_JNLE,

  OP_LOOPNZ,
  OP_LOOPZ,
  OP_LOOP,
  OP_JCXZ,

  OP_COUNT,
};

const char *operator_names[OP_COUNT] = {
  "add",
  "or",
  "adc",
  "sbb",
  "and",
  "sub",
  "xor",
  "cmp",

  "mov",

  "jo",
  "jno",
  "jb",
  "jnb",
  "je",
  "jne",
  "jbe",
  "jnbe",
  "js",
  "jns",
  "jp",
  "jnp",
  "jl",
  "jnl",
  "jle",
  "jnle",

  "loopnz",
  "loopz",
  "loop",
  "jcxz",
};

enum ImmediateRmCmd: u8 {
  ImmediateRmCmd_MOV,
  ImmediateRmCmd_ADD_SUB_CMP,
};

enum RmKind: u8 {
  RM_IMMEDIATE = 0,
  RM_REG,
  RM_SEGMENT_REG,
  RM_DIRECT_ADDR,
  RM_ADDR_FROM_REG,
  RM_ADDR_ADD,
  RM_JUMP_OFFSET,
};

struct RmImmediate {
  u16 immediate;
  bool requires_explicit_size;
};

struct RmReg {
  u8 reg;
};

struct RmSegmentReg {
  u8 sr;
};

struct RmAddr {
  s16 displacement;
  u8 r1;
  u8 r2;
};

struct RmUnion {
  RmKind kind;
  union {
    RmImmediate immediate;
    RmReg reg;
    RmSegmentReg sr;
    RmAddr addr;
    u16 direct_addr;
    s8 jump_offset;
  };
};

enum OpDefinitionFlag: u16 {
  INST_BINARY = 1,
  INST_SET_FLAGS = 1 << 1,
  INST_WRITE_TO_DEST = 1 << 2,
  INST_JUMP = 1 << 3,
  INST_NOT = 1 << 4,
  INST_ON_EQUAL = 1 << 5,
  INST_DECREMENT_CX = 1 << 6,
  INST_ON_CX_NOT_ZERO = 1 << 7,
};

u16 get_op_def_flags(AsmOperatorID id) {
  switch (id) {
    case OP_MOV: return INST_BINARY | INST_WRITE_TO_DEST;
    case OP_ADD: return INST_BINARY | INST_SET_FLAGS | INST_WRITE_TO_DEST;
    case OP_SUB: return INST_BINARY | INST_SET_FLAGS | INST_WRITE_TO_DEST;
    case OP_CMP: return INST_BINARY | INST_SET_FLAGS;
    case OP_JE: return INST_JUMP | INST_ON_EQUAL;
    case OP_JNE: return INST_JUMP | INST_NOT | INST_ON_EQUAL;
    case OP_LOOP: return INST_DECREMENT_CX | INST_JUMP | INST_ON_CX_NOT_ZERO;
    default: INVALID_SWITCH_CASE(id); return 0;
  }
}

struct Instruction {
  RmUnion src;
  RmUnion dest;
  u16 initial_ip;
  u16 encoded_size;
  AsmOperatorID cmd;
  bool wide;
};

RmUnion NewImmediate(u16 immediate, bool requires_explicit_size) {
  return RmUnion{
    .kind = RM_IMMEDIATE,
    .immediate = RmImmediate{
      .immediate = immediate,
      .requires_explicit_size = requires_explicit_size,
    },
  };
}

RmUnion NewReg(u8 reg) {
  return RmUnion{
    .kind = RM_REG,
    .reg = RmReg{
      .reg = reg,
    },
  };
}

RmUnion NewSegmentReg(u8 sr) {
  return RmUnion{
    .kind = RM_SEGMENT_REG,
    .sr = RmSegmentReg{
      .sr = sr,
    },
  };
}

RmUnion NewDirectAddr(u16 addr) {
  return RmUnion{
    .kind = RM_DIRECT_ADDR,
    .direct_addr = addr,
  };
}

RmUnion NewJumpOffset(s8 offset) {
  return RmUnion{
    .kind = RM_JUMP_OFFSET,
    .jump_offset = offset,
  };
}

RmUnion NewAddr(u8 rm, s16 displacement) {
  if (rm & 0b100) {
    u8 r;
    switch (rm) {
      case 0b100: r = REG_SI; break;
      case 0b101: r = REG_DI; break;
      case 0b110: r = REG_BP; break;
      case 0b111: r = REG_BX; break;
    }
    return RmUnion{
      .kind = RM_ADDR_FROM_REG,
      .addr = RmAddr{
        .displacement = displacement,
        .r1 = r,
      },
    };
  } else {
    u8 r1, r2;
    if (rm & 0b010) r1 = REG_BP;
    else            r1 = REG_BX;

    if (rm & 0b001) r2 = REG_DI;
    else            r2 = REG_SI;

    return RmUnion{
      .kind = RM_ADDR_ADD,
      .addr = RmAddr{
        .displacement = displacement,
        .r1 = r1,
        .r2 = r2,
      },
    };
  }
}

union RegBits {
  struct {
    u8 low_bits;
    u8 high_bits;
  };
  u16 all_bits;
};

enum ArithmaticFlags: u8 {
  AFLAG_ZERO = 1,
  AFLAG_SIGN = 1 << 1,
  AFLAG_PARITY = 1 << 2,
  AFLAG_COUNT = 3,
};

struct ExecutionState {
  Stream stream;
  RegBits reg_a;
  RegBits reg_c;
  RegBits reg_d;
  RegBits reg_b;
  u16 reg_sp;
  u16 reg_bp;
  u16 reg_si;
  u16 reg_di;
  u8 *memory;
  u32 mem_size;
  u32 end_of_instructions;
  u16 flags;
  bool do_execution;
  bool print_flag_updates;
  bool print_ip_updates;
  bool print_value_updates;
};

u16 get_ip(ExecutionState *state) {
  return (u16)state->stream.cursor;
}

#endif
