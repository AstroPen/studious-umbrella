
#include "types.h"

RmUnion process_rm_mod(Stream *stream, u8 rm, u8 mod) {
  s16 displacement;
  switch (mod) {
    case 0b00:
      if (rm == 0b110) return NewDirectAddr(pop_u16(stream));
      displacement = 0;
      break;
    case 0b01: displacement = pop_s8(stream); break;
    case 0b10: displacement = pop_s16(stream); break;
    case 0b11: return NewReg(rm);
  }

  return NewAddr(rm, displacement);
}

Instruction parse_cmd_jump(ExecutionState *state, AsmOperatorID cmd, u8 byte1) {
  u16 init_ip = get_ip(state) - 1;
  u8 offset = pop_byte(&state->stream);
  return Instruction{
    .cmd = cmd,
    .src = NewJumpOffset(bit_cast(s8, offset)),
    .initial_ip = init_ip,
    .encoded_size = u16(get_ip(state) - init_ip),
  };
}

Instruction parse_cmd_rm_with_reg(ExecutionState *state, AsmOperatorID cmd, u8 byte1) {
  u16 init_ip = get_ip(state) - 1;
  u8 byte2 = pop_byte(&state->stream);

  u8 mod = byte2 >> 6;
  u8 reg = (byte2 >> 3) & 0b111;
  u8 rm  = byte2 & 0b111;
  u8 dir = byte1 & 0b10;

  bool wide = byte1 & 0b01;

  auto data = Instruction{
    .cmd = cmd,
    .wide = wide,
    .initial_ip = init_ip,
  };

  RmUnion reg_data = NewReg(reg);
  RmUnion rm_data = process_rm_mod(&state->stream, rm, mod);

  if (dir) {
    data.dest = reg_data;
    data.src = rm_data;
  } else {
    data.dest = rm_data;
    data.src = reg_data;
  }

  data.encoded_size = u16(get_ip(state) - init_ip);
  return data;
}

Instruction parse_cmd_immediate_to_reg(ExecutionState *state, AsmOperatorID cmd, u8 byte1) {
  u16 init_ip = get_ip(state) - 1;
  bool wide = !!(byte1 & 0b1000);
  u8 reg  = byte1 & 0b111;

  auto data = Instruction{
    .cmd = cmd,
    .wide = wide,
    .initial_ip = init_ip,
  };

  u16 immediate;
  if (wide) immediate = pop_u16(&state->stream);
  else immediate = pop_byte(&state->stream);

  data.src = NewImmediate(immediate, false);
  data.dest = NewReg(reg);

  data.encoded_size = u16(get_ip(state) - init_ip);
  return data;
}

Instruction parse_cmd_immediate_to_rm(ExecutionState *state, u8 byte1) {
  u16 init_ip = get_ip(state) - 1;
  u8 byte2 = pop_byte(&state->stream);
  u8 mod = byte2 >> 6;
  u8 rm  = byte2 & 0b111;
  u8 reg = (byte2 >> 3) & 0b111;
  bool s = (byte1 >> 1) & 0b1;

  bool wide = byte1 & 0b1;
  bool immediate_wide = wide && !s;
  auto dest = process_rm_mod(&state->stream, rm, mod);

  u16 immediate;
  if (immediate_wide) immediate = pop_u16(&state->stream);
  else immediate = pop_byte(&state->stream);

  AsmOperatorID cmd = (AsmOperatorID) reg;

  return Instruction{
    .cmd = cmd,
    .wide = wide,
    .src = NewImmediate(immediate, dest.kind != RM_REG),
    .dest = dest,
    .initial_ip = init_ip,
    .encoded_size = u16(get_ip(state) - init_ip),
  };
}

Instruction parse_mov_immediate_to_rm(ExecutionState *state, u8 byte1) {
  u16 init_ip = get_ip(state) - 1;
  u8 byte2 = pop_byte(&state->stream);
  u8 mod = byte2 >> 6;
  u8 rm  = byte2 & 0b111;
  u8 reg = (byte2 >> 3) & 0b111;
  ASSERT_EQUAL(reg, 0);

  bool wide = byte1 & 0b1;
  auto dest = process_rm_mod(&state->stream, rm, mod);

  u16 immediate;
  if (wide) immediate = pop_u16(&state->stream);
  else immediate = pop_byte(&state->stream);

  return Instruction{
    .cmd = OP_MOV,
    .wide = wide,
    .src = NewImmediate(immediate, dest.kind != RM_REG),
    .dest = dest,
    .initial_ip = init_ip,
    .encoded_size = u16(get_ip(state) - init_ip),
  };
}

Instruction parse_mov_mem_to_or_from_accumulator(ExecutionState *state, u8 byte1) {
  u16 init_ip = get_ip(state) - 1;
  u16 direct_addr = pop_u16(&state->stream);

  auto data = Instruction{
    .cmd = OP_MOV,
    .wide = true,
    .initial_ip = init_ip,
  };

  auto mem_data = NewDirectAddr(direct_addr);
  auto acc_data = NewReg(REG_AL);

  if (byte1 & 0b10) {
    // Accumulator to memory
    data.src = acc_data;
    data.dest = mem_data;
  } else {
    // Memory to accumulator
    data.src = mem_data;
    data.dest = acc_data;
  }
  data.encoded_size = u16(get_ip(state) - init_ip);
  return data;
}

Instruction parse_cmd_immediate_with_accumulator(ExecutionState *state, AsmOperatorID cmd, u8 byte1) {
  u16 init_ip = get_ip(state) - 1;
  bool wide = byte1 & 0b1;
  u16 immediate;
  if (wide) immediate = pop_u16(&state->stream);
  else immediate = pop_byte(&state->stream);

  return Instruction{
    .cmd = cmd,
    .wide = wide,
    .src = NewImmediate(immediate, false),
    .dest = NewReg(REG_AL),
    .initial_ip = init_ip,
    .encoded_size = u16(get_ip(state) - init_ip),
  };
}

Instruction parse_cmd_rm_with_segment_reg(ExecutionState *state, AsmOperatorID cmd, u8 byte1) {
  u16 init_ip = get_ip(state) - 1;
  u8 byte2 = pop_byte(&state->stream);

  u8 mod = byte2 >> 6;
  u8 sr = (byte2 >> 3) & 0b11;
  u8 rm  = byte2 & 0b111;
  u8 dir = byte1 & 0b10;

  auto data = Instruction{
    .cmd = cmd,
    .wide = true,
    .initial_ip = init_ip,
  };

  RmUnion sr_data = NewSegmentReg(sr);
  RmUnion rm_data = process_rm_mod(&state->stream, rm, mod);

  if (dir) {
    data.dest = sr_data;
    data.src = rm_data;
  } else {
    data.dest = rm_data;
    data.src = sr_data;
  }
  data.encoded_size = u16(get_ip(state) - init_ip);
  return data;
}

Instruction parse_next_instruction(ExecutionState *state) {
  u8 byte1 = pop_byte(&state->stream);
  // First four bits of the opcode. Opcodes have varying length and the shortest is 3 bits.
  u8 opcode = byte1 >> 2;
  switch(opcode) {
    case 0b000000:
      // ADD Reg/memory with register to either
      return parse_cmd_rm_with_reg(state, OP_ADD, byte1);
    case 0b000001:
      return parse_cmd_immediate_with_accumulator(state, OP_ADD, byte1);
    case 0b000011:
    case 0b000101:
    case 0b000111:
      // PUSH / POP Segment Register
      FAILURE("Unhandled opcode: PUSH/POP segment reg");
    case 0b001010:
      return parse_cmd_rm_with_reg(state, OP_SUB, byte1);
    case 0b001011:
      return parse_cmd_immediate_with_accumulator(state, OP_SUB, byte1);
    case 0b001110:
      return parse_cmd_rm_with_reg(state, OP_CMP, byte1);
    case 0b001111:
      return parse_cmd_immediate_with_accumulator(state, OP_CMP, byte1);
    case 0b010100:
    case 0b010101:
      // PUSH Register
      FAILURE("Unhandled opcode: PUSH reg");
    case 0b010110:
    case 0b010111:
      // POP Register
      FAILURE("Unhandled opcode: POP reg");
    case 0b011100:
      switch(byte1 & 0b11) {
        // JO
        case 0b00: return parse_cmd_jump(state, OP_JO, byte1);
        // JNO
        case 0b01: return parse_cmd_jump(state, OP_JNO, byte1);
        // JB / JNAE
        case 0b10: return parse_cmd_jump(state, OP_JB, byte1);
        // JNB
        case 0b11: return parse_cmd_jump(state, OP_JNB, byte1);
      }
    case 0b011101:
      switch(byte1 & 0b11) {
        // JE / JZ
        case 0b00: return parse_cmd_jump(state, OP_JE, byte1);
        // JNE / JNZ
        case 0b01: return parse_cmd_jump(state, OP_JNE, byte1);
        // JBE / JNA
        case 0b10: return parse_cmd_jump(state, OP_JBE, byte1);
        // JNBE / JA
        case 0b11: return parse_cmd_jump(state, OP_JNBE, byte1);
      }
    case 0b011110:
      switch(byte1 & 0b11) {
        // JS
        case 0b00: return parse_cmd_jump(state, OP_JS, byte1);
        // JNS
        case 0b01: return parse_cmd_jump(state, OP_JNS, byte1);
        // JP
        case 0b10: return parse_cmd_jump(state, OP_JP, byte1);
        // JNP / JPO
        case 0b11: return parse_cmd_jump(state, OP_JNP, byte1);
      }
    case 0b011111:
      switch(byte1 & 0b11) {
        // JL / JNGE
        case 0b00: return parse_cmd_jump(state, OP_JL, byte1);
        // JNL / JGE
        case 0b01: return parse_cmd_jump(state, OP_JNL, byte1);
        // JLE / JNG
        case 0b10: return parse_cmd_jump(state, OP_JLE, byte1);
        // JNLE / JG
        case 0b11: return parse_cmd_jump(state, OP_JNLE, byte1);
      }
    case 0b100000:
      return parse_cmd_immediate_to_rm(state, byte1);
    case 0b100001:
      // XCHG Register/memory with register
      FAILURE("Unhandled opcode: XCHG rm with reg");
    case 0b100010:
      // MOV Register/memory to/from register
      return parse_cmd_rm_with_reg(state, OP_MOV, byte1);
    case 0b100011:
      // MOV Register/memory to segment register
      // MOV Segment register to register/memory
      if (byte1 & 1) {
        // POP Register/memory
        // LEA
        FAILURE("Unhandled opcode: LEA");
      } else {
        return parse_cmd_rm_with_segment_reg(state, OP_MOV, byte1);
      }
    case 0b100100:
    case 0b100101:
      // XCHG Register with accumulator
      FAILURE("Unhandled opcode: XCHG accumulator");
    case 0b100111:
      // LAHF / SAHF / PUSHF / POPF
      FAILURE("Unhandled opcode: Flags");
    case 0b101000:
      // MOV Memory to accumulator
      // MOV Accumulator to memory
      return parse_mov_mem_to_or_from_accumulator(state, byte1);
    case 0b101100:
    case 0b101101:
    case 0b101110:
    case 0b101111:
      // NOTE: The last two bits are not part of the op code.
      // MOV Immediate to register
      return parse_cmd_immediate_to_reg(state, OP_MOV, byte1);
    case 0b110001:
      switch (byte1 & 0b11) {
        case 0b00: // LES
          FAILURE("Unhandled opcode: LES");
        case 0b01: // LDS
          FAILURE("Unhandled opcode: LDS");
        default: return parse_mov_immediate_to_rm(state, byte1);
      }
    case 0b110101:
      // XLAT
      FAILURE("Unhandled opcode: XLAT");
    case 0b111000:
      switch(byte1 & 0b11) {
        // LOOPNZ / LOOPNE
        case 0b00: return parse_cmd_jump(state, OP_LOOPNZ, byte1);
        // LOOPZ / LOOPE
        case 0b01: return parse_cmd_jump(state, OP_LOOPZ, byte1);
        // LOOP
        case 0b10: return parse_cmd_jump(state, OP_LOOP, byte1);
        // JCXZ
        case 0b11: return parse_cmd_jump(state, OP_JCXZ, byte1);
      }
    case 0b111001:
      // IN / OUT Fixed port
      FAILURE("Unhandled opcode: IN / OUT Fixed port");
    case 0b111011:
      // IN / OUT Variable port
      FAILURE("Unhandled opcode: IN / OUT Variable port");
    case 0b111111:
      // PUSH Register/memory
      FAILURE("Unhandled opcode: PUSH rm");
    default:
      printf("; Unknown opcode: %x\n", opcode);
  }
  FAILURE("Failed to process instruction.", byte1);
  return Instruction{};
}

