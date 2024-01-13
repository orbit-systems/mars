#pragma once
#define DEIMOS_TAC_H

#include "orbit.h"
#include "dynarr.h"

void testTAC();
void processTAC();

typedef u8 TAC_type; enum {
	INVALID_TAC_type = 0,

	meta_special_begin,
		special_raw_asm,
		special_insert_symbols,
	meta_special_end,

	meta_operations_begin,
		operator_add,
		operator_sub,
		operator_mul,
		operator_div,
	meta_operations_end,

	meta_label_begin,
		label_function,
		label_label_address,
		label_table_address,
		label_table_offset,
	meta_label_end,

	meta_function_calls_start,
		function_pass,
		function_call,
		function_return,
		function_load_rv, //load return value
		function_pass_rv, //pass return value
		function_argument,
	meta_function_calls_end,

	meta_function_load_start,
		load_f16,
		load_f32,
		load_f64,

		load_i8,
		load_i16,
		load_i32,
		load_i64,

		load_u8,
		load_u16,
		load_u32,
		load_u64,

		load_data,
	meta_function_load_end,

	meta_branch_start,
		branch_ble,
	meta_branch_end,
};


typedef u8 TAC_arg_type; enum {
	INVALID_arg_type = 0,
	label,
	literal_int,
	literal_float,
	tac_var,
};

typedef struct {
	TAC_arg_type type;
	string value;
} TAC_arg;

typedef struct {
	TAC_type type;
	int arg_len;
	TAC_arg  arg[3];
} TAC_element;

dynarr_lib_h(TAC_element);
typedef u8 TAC_allocation; enum {
	NONE = 0,
	TAC_STACK,
	TAC_REGISTER,
};

dynarr_lib_h(TAC_element);
