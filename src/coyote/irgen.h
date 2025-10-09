#ifndef IRGEN_H
#define IRGEN_H

#include "lex.h"
#include "parse.h"

#include "iron/iron.h"

FeModule* irgen(CompilationUnit* cu);

#endif // IRGEN_H
