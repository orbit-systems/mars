#pragma once
#define ALLOC_H

#include <stdlib.h>
#include <string.h>

void* mars_alloc(size_t size);
void mars_free(void* ptr);
void* mars_realloc(void* ptr, size_t new_size);