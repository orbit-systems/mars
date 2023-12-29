package deimos

TAC :: struct {
	type: TAC_type,
	arguments: TAC_arg,
}

TAC_type :: enum {
	INVALID,

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
		load_b8,
		load_b16,
		load_b32,
		load_b64, 

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
}

TAC_arg :: struct {
	type: TAC_arg_type,
	value: string,
}

TAC_arg_type :: enum {
	INVALID,
	label,
	literal_int,
	literal_float,
}

/*TODO:
	add deref


*/

//INSERT_SYMBOLS "neptune"

//FUNCTION main
//LOAD_I64 t0, 4
//PASS t0
//LABEL_ADDRESS t1, test
//CALL t1
//RETURN

//FUNCTION test
//LABEL_ADDRESS t2, add
//ARGUMENT t3, 0 //get argument 0 from the stack
//PASS t3
//LOAD_I64 t4, 3
//PASS t4
//CALL t2
//LOAD_RETURN t8, 0 //gets return value 0 from the stack
//LOAD_I64 t9, 5
//BRANCH_BLE test_1, t8, t9 //flip this to be less than or equal to, as opposed to greater than
//FUNCTION test_0
//add(a + 3) > 5 succeeded
//TABLE_ADDRESS t10, neptune
//TABLE_OFFSET t11, t10, printf
//LOAD_DATA t12, 0 //load data value 0
//PASS t12
//CALL t11 //call nsys.printf
//FUNCTION test_1
//RETURN

//FUNCTION add
//ARGUMENT t5, 0
//ARGUMENT t6, 0
//ADD t7, t6, t5
//PASS_RETURN t7
//RETURN

//DATA
//"we ballin"
//