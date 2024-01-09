#include "orbit.h"
#include "dynarr.h"

#include "phobos/phobos.h"
#include "phobos/lexer.h"
#include "phobos/parser.h"
#include "phobos/ast.h"
#include "phobos/type.h"

// stuff all your dynarr code here! its jank, 
// but it prevents code duplication and declutters the place.
// yes yes crucify me for this whatever i dont gibve a shit

dynarr_lib(arena)
dynarr_lib(mars_type)
dynarr_lib(size_t)
dynarr_lib(string)
dynarr_lib(AST)
dynarr_lib(token)
dynarr_lib(lexer)
dynarr_lib(parser)
dynarr_lib(mars_file)