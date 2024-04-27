#pragma once
#define DEIMOS_H

#include "mars.h"
#include "ir.h"
#include "phobos.h"

char* random_string(int len);

IR_Module* ir_generate(mars_module* mod);