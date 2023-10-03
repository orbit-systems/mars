package deimos

import co "../common"

// mars compiler backend - codegen, register/instruction selector
// accepts abstract syntax tree from phobos and produces aphelion assembly

build_state : co.build_state_t