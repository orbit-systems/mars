#pragma once
#define DEIMOS_H

#include "mars.h"
#include "alloc.h"
#include "ir.h"
#include "phobos.h"
#include "pass.h"

char* random_string(int len);

AIR_Module* air_generate(mars_module* mod);

void deimos_run(mars_module* main_mod, AIR_Module* passthrough);