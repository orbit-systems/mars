#pragma once
#define DEIMOS_H

#include "mars.h"
#include "ir.h"
#include "phobos.h"

char* random_string(int len);

IR_Module* generate_ir(mars_module* mod);