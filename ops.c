#include "ops.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int MAX_STACK_DEPTH = 5;
const memory_addr_t SFR_ADDR_START = 0xf0;
const memory_addr_t SFR_ADDR_END = 0x100;

const char* const reg_names[16] = {
	"R0", "R1", "R2", "R3", "R4", 
	"R5", "R6", "R7", "R8", "R9", 
	"OUT", "IN", "JSR", "PCL", "PCM", "PCH"
};

void get_info_rx(const struct vm_instruction *instr, char *out)
{
	snprintf(out, INFO_SIZE, "%s", reg_names[instr->nibble2]);
}

void get_info_ry(const struct vm_instruction *instr, char *out)
{
	snprintf(out, INFO_SIZE, "%s", reg_names[instr->nibble3]);
}

void get_info_rg(const struct vm_instruction *instr, char *out)
{
	uint8_t rg = instr->nibble3 >> 2;
	if (rg < 0x3) {
		snprintf(out, INFO_SIZE, "%s", reg_names[rg]);
	} else {
		snprintf(out, INFO_SIZE, "RS");
	}
}

void get_info_r0(const struct vm_instruction *instr, char *out)
{
	snprintf(out, INFO_SIZE, "R0");
}

void get_info_pc(const struct vm_instruction *instr, char *out)
{
	snprintf(out, INFO_SIZE, "PC");
}

void get_info_pointer(const struct vm_instruction *instr, char *out)
{
	snprintf(out, INFO_SIZE, "[%#02hhx]", (instr->nibble2 << 4) | instr->nibble3);
}

void get_info_indirect(const struct vm_instruction *instr, char *out)
{
	snprintf(out, INFO_SIZE, "[%s:%s]",
		reg_names[instr->nibble2],
		reg_names[instr->nibble3]);
}

void get_info_literal(const struct vm_instruction *instr, char *out)
{
	snprintf(out, INFO_SIZE, "%#02hhx", instr->nibble3);
}

void get_info_byte_literal(const struct vm_instruction *instr, char *out)
{
	snprintf(out, INFO_SIZE, "%#02x", (instr->nibble2 << 4) | instr->nibble3);
}

void get_info_crumb_literal(const struct vm_instruction *instr, char *out)
{
	snprintf(out, INFO_SIZE, "%#02x", instr->nibble3 & 0x3);
}

const char *CONDITIONS[] = {"C", "NC", "Z", "NZ"};

void get_info_condition_flag(const struct vm_instruction *instr, char *out)
{
	snprintf(out, INFO_SIZE, "%#02x", instr->nibble3 >> 2);
}

memory_addr_t get_reg_addr(const memory_word_t *reg, const struct vm_state *vm) {
	return (memory_addr_t) (reg - &vm->user_mem[0]);
}

memory_addr_t get_addr_rx(const struct vm_instruction *instr, const struct vm_state *vm) {
	return get_reg_addr(&vm->main_regs_page[instr->nibble2], vm);
}

memory_addr_t get_addr_ry(const struct vm_instruction *instr, const struct vm_state *vm) {
	return get_reg_addr(&vm->main_regs_page[instr->nibble3], vm);
}

memory_addr_t get_addr_rg_in(const struct vm_instruction *instr, const struct vm_state *vm) {
	uint8_t rg = instr->nibble3 >> 2;
	if (rg < 0x3) {
		return get_reg_addr(&vm->main_regs_page[rg], vm);
	} else if (vm->reg_wr_flags & WR_FLAG_IN_OUT_POS) {
		return get_reg_addr(&vm->reg_in_b, vm);
	} else {
		return get_reg_addr(&vm->reg_in, vm);
	}
}

memory_addr_t get_addr_rg_out(const struct vm_instruction *instr, const struct vm_state *vm) {
	uint8_t rg = instr->nibble3 >> 2;
	if (rg < 0x3) {
		return get_reg_addr(&vm->main_regs_page[rg], vm);
	} else if (vm->reg_wr_flags & WR_FLAG_IN_OUT_POS) {
		return get_reg_addr(&vm->reg_out_b, vm);
	} else {
		return get_reg_addr(&vm->reg_out, vm);
	}
}

memory_addr_t get_addr_r0(const struct vm_instruction *instr, const struct vm_state *vm) {
	return get_reg_addr(&vm->reg_r0, vm);
}

memory_addr_t get_addr_pc(const struct vm_instruction *instr, const struct vm_state *vm) {
	return get_reg_addr(&vm->reg_pcm, vm);
}

memory_addr_t get_addr_pointer(const struct vm_instruction *instr, const struct vm_state *vm) {
	return (instr->nibble2 << 4) | instr->nibble3;
}

memory_addr_t get_addr_indirect(const struct vm_instruction *instr, const struct vm_state *vm) {
	return (vm->main_regs_page[instr->nibble2] << 4) | vm->main_regs_page[instr->nibble3];
}

const struct operand_dst DST_RX  = {.mnemnonic = "RX",   .get_addr = get_addr_rx,       .get_info = get_info_rx};
const struct operand_dst DST_RY  = {.mnemnonic = "RY",   .get_addr = get_addr_ry,       .get_info = get_info_ry};
const struct operand_dst DST_RGI = {.mnemnonic = "RG",   .get_addr = get_addr_rg_in,    .get_info = get_info_rg};
const struct operand_dst DST_RGO = {.mnemnonic = "RG",   .get_addr = get_addr_rg_out,   .get_info = get_info_rg};
const struct operand_dst DST_R0  = {.mnemnonic = "R0",   .get_addr = get_addr_r0,       .get_info = get_info_r0};
const struct operand_dst DST_PC  = {.mnemnonic = "PC",   .get_addr = get_addr_pc,       .get_info = get_info_pc};
const struct operand_dst DST_PTR = {.mnemnonic = "[NN]", .get_addr = get_addr_pointer,  .get_info = get_info_pointer};
const struct operand_dst DST_IND = {.mnemnonic = "[XY]", .get_addr = get_addr_indirect, .get_info = get_info_indirect};

uint8_t get_condition_flag(const struct vm_instruction *instr, const struct vm_state *vm)
{
	return instr->nibble3 >> 2;
}

const struct operand_cnd CND_FLG = {.mnemnonic = "F",    .get_val = get_condition_flag, .get_info = get_info_condition_flag};

uint8_t get_val_ry(const struct vm_instruction *instr, const struct vm_state *vm)
{
	return vm->main_regs_page[instr->nibble3];
}

uint8_t get_val_r0(const struct vm_instruction *instr, const struct vm_state *vm)
{
	return vm->reg_r0;
}

uint8_t get_val_pointer(const struct vm_instruction *instr, const struct vm_state *vm)
{
	memory_addr_t addr = (instr->nibble2 << 4) | instr->nibble3;
	return vm->user_mem[addr];
}

uint8_t get_val_indirect(const struct vm_instruction *instr, const struct vm_state *vm)
{
	memory_addr_t addr = (vm->main_regs_page[instr->nibble2] << 4) | vm->main_regs_page[instr->nibble3];
	return vm->user_mem[addr];
}

uint8_t get_val_literal(const struct vm_instruction *instr, const struct vm_state *vm)
{
	return instr->nibble3;
}

uint8_t get_val_byte_literal(const struct vm_instruction *instr, const struct vm_state *vm)
{
	return (instr->nibble2 << 4) | instr->nibble3;
}

uint8_t get_val_crumb_literal(const struct vm_instruction *instr, const struct vm_state *vm)
{
	return instr->nibble3 & 0x3;
}

const struct operand_src SRC_RY  = {.mnemnonic = "RY",   .get_val = get_val_ry,            .get_info = get_info_ry};
const struct operand_src SRC_R0  = {.mnemnonic = "R0",   .get_val = get_val_r0,            .get_info = get_info_r0};
const struct operand_src SRC_PTR = {.mnemnonic = "[NN]", .get_val = get_val_pointer,       .get_info = get_info_pointer};
const struct operand_src SRC_IND = {.mnemnonic = "[XY]", .get_val = get_val_indirect,      .get_info = get_info_indirect};
const struct operand_src SRC_N   = {.mnemnonic = "N",    .get_val = get_val_literal,       .get_info = get_info_literal};
const struct operand_src SRC_NN  = {.mnemnonic = "NN",   .get_val = get_val_byte_literal,  .get_info = get_info_byte_literal};
const struct operand_src SRC_M   = {.mnemnonic = "M",    .get_val = get_val_crumb_literal, .get_info = get_info_crumb_literal};

void update_flags(uint8_t result, uint8_t mask, struct vm_state *vm)
{
	uint8_t new_flags = 0;
	if (result & 0x10) {
		new_flags |= FLAG_CARRY;
	}
	if (!(result & 0xf)) {
		new_flags |= FLAG_ZERO;
	}
	/* TODO(octav): Implement overflow logic. */
	if (0) {
		new_flags |= FLAG_OVERFLOW;
	}
	vm->reg_flags = (vm->reg_flags & ~mask) | (new_flags & mask);
	if (mask & FLAG_OVERFLOW) {
		if (vm->reg_flags & FLAG_OVERFLOW) {
			vm->reg_rd_flags |= RD_FLAG_V_FLAG;
		} else {
			vm->reg_rd_flags &= ~RD_FLAG_V_FLAG;
		}
	}
}

void maybe_call_or_jump(memory_addr_t dst, struct vm_state *vm)
{
	if (dst == SFR_JSR) {
		if (vm->reg_sp == MAX_STACK_DEPTH) {
			fprintf(stderr, "Stack overflow.\n");
			exit(1);
		}
		vm->stack[vm->reg_sp * 3] = vm->reg_pc & 0xf;
		vm->stack[vm->reg_sp * 3 + 1] = (vm->reg_pc >> 4) & 0xf;
		vm->stack[vm->reg_sp * 3 + 2] = vm->reg_pc >> 8;
		vm->reg_sp++;
		vm->reg_pc = (vm->reg_pch << 8) | (vm->reg_pcm << 4) | vm->reg_jsr;
		return;
	}

	if (dst == SFR_PCL) {
		vm->reg_pc = (vm->reg_pch << 8) | (vm->reg_pcm << 4) | vm->reg_pcl;
		return;
	}
}

bool is_sfr_address(memory_addr_t addr, const struct vm_state *vm)
{
	return addr >= SFR_FIRST && addr <= SFR_LAST;
}

bool maybe_handle_sfr_read(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t addr = get_addr_pointer(instr, vm);
	if (!is_sfr_address(addr, vm)) {
		return false;
	}

	/* TODO(octav): Handle reads from special regs. */
	switch (addr) {
	case SFR_RD_FLAGS:
		vm->reg_r0 = vm->reg_rd_flags;
		vm->reg_rd_flags &= ~RD_FLAG_USER_SYNC;
		break;
	case SFR_RANDOM:
		vm->reg_r0 = vm->reg_random;
		vm->reg_random = next_rng(&vm->rng) & 0xf;
		break;
	default:
		vm->reg_r0 = vm->user_mem[addr];
		break;
	}

	return true;
}

bool maybe_handle_sfr_write(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t addr = get_addr_pointer(instr, vm);
	if (!is_sfr_address(addr, vm)) {
		return false;
	}

	/* TODO(octav): Handle writes to special regs. */
	switch (addr) {
	case SFR_RANDOM:
		if (vm->reg_r0 == 0xf) {
			init_rng(&vm->rng);
			vm->reg_random = next_rng(&vm->rng) & 0xf;
		} else {
			uint32_t seed = vm->reg_r0;
			vm->reg_random = seed;
			seed = (seed << 4) | seed;
			seed = (seed << 8) | seed;
			set_rng_seed(&vm->rng, seed);
		}
		break;
	default:
		vm->user_mem[addr] = vm->reg_r0;
		break;
	}

	return true;
}

void op_add(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result += descr->src->get_val(instr, vm);
	vm->user_mem[dst] = result & 0xf;
	update_flags(result, FLAG_CARRY | FLAG_ZERO | FLAG_OVERFLOW, vm);
}

void op_adc(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result += descr->src->get_val(instr, vm);
	if (vm->reg_flags & FLAG_CARRY) {
		result++;
	}
	vm->user_mem[dst] = result & 0xf;
	update_flags(result, FLAG_CARRY | FLAG_ZERO | FLAG_OVERFLOW, vm);
}

void op_sub(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result -= descr->src->get_val(instr, vm);
	vm->user_mem[dst] = result & 0xf;
	update_flags(result, FLAG_CARRY | FLAG_ZERO | FLAG_OVERFLOW, vm);
}

void op_sbb(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result -= descr->src->get_val(instr, vm);
	if (!(vm->reg_flags & FLAG_CARRY)) {
		result--;
	}
	vm->user_mem[dst] = result & 0xf;
	update_flags(result, FLAG_CARRY | FLAG_ZERO | FLAG_OVERFLOW, vm);
}

void op_or(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result |= descr->src->get_val(instr, vm);
	vm->user_mem[dst] = result;
	if (descr->flg & OP_FLAG_UPDATE_CARRY) {
		vm->reg_flags |= FLAG_CARRY;
	}
}

void op_and(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result &= descr->src->get_val(instr, vm);
	vm->user_mem[dst] = result;
	if (descr->flg & OP_FLAG_UPDATE_CARRY) {
		vm->reg_flags &= ~FLAG_CARRY;
	}
}

void op_xor(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result ^= descr->src->get_val(instr, vm);
	vm->user_mem[dst] = result;
	if (descr->flg & OP_FLAG_UPDATE_CARRY) {
		vm->reg_flags ^= FLAG_CARRY;
	}
}

void op_mov(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	if ((descr->flg & OP_FLAG_CAN_RD_SFR) && maybe_handle_sfr_read(instr, descr, vm)) {
		return;
	}
	if ((descr->flg & OP_FLAG_CAN_WR_SFR) && maybe_handle_sfr_write(instr, descr, vm)) {
		return;
	}
	uint8_t src = descr->src->get_val(instr, vm);
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	if (descr->flg & OP_FLAG_DST_BYTE) {
		vm->user_mem[dst] = src & 0xf;
		vm->user_mem[dst + 1] = src >> 4;
	} else {
		vm->user_mem[dst] = src;
	}
	if (descr->flg & OP_FLAG_CAN_JUMP) {
		maybe_call_or_jump(dst, vm);
	}
}

void op_jr(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	vm->reg_pc += (int8_t) descr->src->get_val(instr, vm);
}

void op_cp(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result -= descr->src->get_val(instr, vm);
	update_flags(result, FLAG_CARRY | FLAG_ZERO, vm);
}

void op_inc(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result++;
	vm->user_mem[dst] = result & 0xf;
	update_flags(result, FLAG_CARRY | FLAG_ZERO, vm);
	maybe_call_or_jump(dst, vm);
}

void op_dec(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result--;
	vm->user_mem[dst] = result & 0xf;
	update_flags(result, FLAG_CARRY | FLAG_ZERO, vm);
	maybe_call_or_jump(dst, vm);
}

void op_dsz(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	result--;
	result &= 0xf;
	vm->user_mem[dst] = result;
	if (!result) {
		vm->reg_pc++;
	}
}

void op_exr(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	uint8_t n = descr->src->get_val(instr, vm);
	if (!n) {
		n = 0x10;
	}
	memory_word_t buf[n];
	size_t size = n * sizeof(memory_word_t);
	memcpy(buf, vm->main_regs_page, size);
	memcpy(vm->main_regs_page, vm->alt_regs_page, size);
	memcpy(vm->alt_regs_page, buf, size);
}

void op_bit(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t m = descr->src->get_val(instr, vm);
	uint8_t result = vm->user_mem[dst] & (1 << m);
	update_flags(result, FLAG_ZERO, vm);
}

void op_bset(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t m = descr->src->get_val(instr, vm);
	vm->user_mem[dst] |= 1 << m;
}

void op_bclr(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t m = descr->src->get_val(instr, vm);
	vm->user_mem[dst] &= ~(1 << m);
}

void op_btg(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t m = descr->src->get_val(instr, vm);
	vm->user_mem[dst] ^= 1 << m;
}

void op_rrc(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	memory_addr_t dst = descr->dst->get_addr(instr, vm);
	uint8_t result = vm->user_mem[dst];
	bool carry = vm->reg_flags & FLAG_CARRY;
	if (result & 0x1) {
		vm->reg_flags |= FLAG_CARRY;
	} else {
		vm->reg_flags &= ~FLAG_CARRY;
	}
	result >>= 1;
	if (carry) {
		result |= 0x8;
	}
	vm->user_mem[dst] = result;
	update_flags(result, FLAG_ZERO, vm);
}

void op_ret(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	if (!vm->reg_sp) {
		fprintf(stderr, "Stack underflow.\n");
		exit(1);
	}
	uint8_t n = descr->src->get_val(instr, vm);
	vm->reg_r0 = n;
	vm->reg_sp--;
	memory_word_t ret_ptr = vm->reg_sp * 3;
	vm->reg_pc = vm->stack[ret_ptr] | (vm->stack[ret_ptr + 1] << 4) | (vm->stack[ret_ptr + 2] << 8);
}

void op_skip(const struct vm_instruction *instr, const struct instruction_descriptor *descr, struct vm_state *vm)
{
	uint8_t cnd_flg = descr->cnd->get_val(instr, vm);
	uint8_t m = descr->src->get_val(instr, vm);
	if (!m) {
		m = 4;
	}
	bool result = false;
	switch (cnd_flg) {
	case 0:
		result = vm->reg_flags & FLAG_CARRY;
		break;
	case 1:
		result = !(vm->reg_flags & FLAG_CARRY);
		break;
	case 2:
		result = vm->reg_flags & FLAG_ZERO;
		break;
	case 3:
		result = !(vm->reg_flags & FLAG_ZERO);
		break;
	}
	if (result) {
		vm->reg_pc += m;
	}
}

const struct operation OP_ADD  = {.mnemnonic = "ADD",  .op_fn = op_add};
const struct operation OP_ADC  = {.mnemnonic = "ADC",  .op_fn = op_adc};
const struct operation OP_SUB  = {.mnemnonic = "SUB",  .op_fn = op_sub};
const struct operation OP_SBB  = {.mnemnonic = "SBB",  .op_fn = op_sbb};
const struct operation OP_OR   = {.mnemnonic = "OR",   .op_fn = op_or};
const struct operation OP_AND  = {.mnemnonic = "AND",  .op_fn = op_and};
const struct operation OP_XOR  = {.mnemnonic = "XOR",  .op_fn = op_xor};
const struct operation OP_MOV  = {.mnemnonic = "MOV",  .op_fn = op_mov};
const struct operation OP_JR   = {.mnemnonic = "JR",   .op_fn = op_jr};
const struct operation OP_CP   = {.mnemnonic = "CP",   .op_fn = op_cp};
const struct operation OP_INC  = {.mnemnonic = "INC",  .op_fn = op_inc};
const struct operation OP_DEC  = {.mnemnonic = "DEC",  .op_fn = op_dec};
const struct operation OP_DSZ  = {.mnemnonic = "DSZ",  .op_fn = op_dsz};
const struct operation OP_EXR  = {.mnemnonic = "EXR",  .op_fn = op_exr};
const struct operation OP_BIT  = {.mnemnonic = "BIT",  .op_fn = op_bit};
const struct operation OP_BSET = {.mnemnonic = "BSET", .op_fn = op_bset};
const struct operation OP_BCLR = {.mnemnonic = "BCLR", .op_fn = op_bclr};
const struct operation OP_BTG  = {.mnemnonic = "BTG",  .op_fn = op_btg};
const struct operation OP_RRC  = {.mnemnonic = "RRC",  .op_fn = op_rrc};
const struct operation OP_RET  = {.mnemnonic = "RET",  .op_fn = op_ret};
const struct operation OP_SKIP = {.mnemnonic = "SKIP", .op_fn = op_skip};

/* Single nibble opcodes. */
const struct instruction_descriptor INSTRUCTIONS[] = {
	{},  /* Not an instruction as it indicates a wide opcode. */
	{.op = &OP_ADD,  .dst = &DST_RX,  .src = &SRC_RY},
	{.op = &OP_ADC,  .dst = &DST_RX,  .src = &SRC_RY},
	{.op = &OP_SUB,  .dst = &DST_RX,  .src = &SRC_RY},
	{.op = &OP_SBB,  .dst = &DST_RX,  .src = &SRC_RY},
	{.op = &OP_OR,   .dst = &DST_RX,  .src = &SRC_RY},
	{.op = &OP_AND,  .dst = &DST_RX,  .src = &SRC_RY},
	{.op = &OP_XOR,  .dst = &DST_RX,  .src = &SRC_RY},
	{.op = &OP_MOV,  .dst = &DST_RX,  .src = &SRC_RY,  .flg = OP_FLAG_CAN_JUMP},
	{.op = &OP_MOV,  .dst = &DST_RX,  .src = &SRC_N,   .flg = OP_FLAG_CAN_JUMP},
	{.op = &OP_MOV,  .dst = &DST_IND, .src = &SRC_R0},
	{.op = &OP_MOV,  .dst = &DST_R0,  .src = &SRC_IND},
	{.op = &OP_MOV,  .dst = &DST_PTR, .src = &SRC_R0,  .flg = OP_FLAG_CAN_WR_SFR},
	{.op = &OP_MOV,  .dst = &DST_R0,  .src = &SRC_PTR, .flg = OP_FLAG_CAN_RD_SFR},
	{.op = &OP_MOV,  .dst = &DST_PC,  .src = &SRC_NN,  .flg = OP_FLAG_DST_BYTE},
	{.op = &OP_JR,                    .src = &SRC_NN},
};

/* Double nibble opcodes (indexed by second nibble; first nibble is zero). */
const struct instruction_descriptor INSTRUCTIONS_WIDE[] = {
	{.op = &OP_CP,   .dst = &DST_R0,  .src = &SRC_N},
	{.op = &OP_ADD,  .dst = &DST_R0,  .src = &SRC_N},
	{.op = &OP_INC,  .dst = &DST_RY,                   .flg = OP_FLAG_CAN_JUMP},
	{.op = &OP_DEC,  .dst = &DST_RY,                   .flg = OP_FLAG_CAN_JUMP},
	{.op = &OP_DSZ,  .dst = &DST_RY},
	{.op = &OP_OR,   .dst = &DST_R0,  .src = &SRC_N,   .flg = OP_FLAG_UPDATE_CARRY},
	{.op = &OP_AND,  .dst = &DST_R0,  .src = &SRC_N,   .flg = OP_FLAG_UPDATE_CARRY},
	{.op = &OP_XOR,  .dst = &DST_R0,  .src = &SRC_N,   .flg = OP_FLAG_UPDATE_CARRY},
	{.op = &OP_EXR,                   .src = &SRC_N},
	{.op = &OP_BIT,  .dst = &DST_RGI, .src = &SRC_M},
	{.op = &OP_BSET, .dst = &DST_RGO, .src = &SRC_M},
	{.op = &OP_BCLR, .dst = &DST_RGO, .src = &SRC_M},
	{.op = &OP_BTG,  .dst = &DST_RGO, .src = &SRC_M},
	{.op = &OP_RRC,  .dst = &DST_RY},
	{.op = &OP_RET,  .dst = &DST_R0,  .src = &SRC_N},
	{.op = &OP_SKIP, .cnd = &CND_FLG, .src = &SRC_M},
};

void decode_instruction(program_word_t pi, struct vm_instruction *vmi)
{
	vmi->nibble1 = (pi >> 8) & 0xf;
	vmi->nibble2 = (pi >> 4) & 0xf;
	vmi->nibble3 = pi & 0xf;
}

const struct instruction_descriptor *get_instruction_descriptor(const struct vm_instruction *vmi)
{
	if (vmi->nibble1) {
		return &INSTRUCTIONS[vmi->nibble1];
	} else {
		return &INSTRUCTIONS_WIDE[vmi->nibble2];
	}
}

void disassemble_instruction(const struct vm_instruction *vmi, const struct instruction_descriptor *descr, char *out, size_t size)
{
	int count;
	count = snprintf(out, size, "%-4s ", descr->op->mnemnonic);
	out += count;
	size -= count;
	int i = 0;
	char info[INFO_SIZE];
	if (descr->dst != NULL) {
		descr->dst->get_info(vmi, info);
		count = snprintf(out, size, "%s", info);
		out += count;
		size -= count;
		i++;
	}
	if (descr->cnd!= NULL) {
		descr->cnd->get_info(vmi, info);
		if (i && size > 1) {
			*out++ = ',';
			*out = '\0';
			count++;
		}
		count = snprintf(out, size, "%s", info);
		out += count;
		size -= count;
		i++;
	}
	if (descr->src != NULL) {
		descr->src->get_info(vmi, info);
		if (i && size > 1) {
			*out++ = ',';
			*out = '\0';
			count++;
		}
		count = snprintf(out, size, "%s", info);
		out += count;
		size -= count;
		i++;
	}
}
