#include "deimos_util.h"

char* random_string(int len) {
	char* str = malloc(len + 1);
	if (str == NULL) general_error("Failed to allocate %d bytes for random_string", len + 1);
	FOR_URANGE(i, 0, len) {
		str[i] = '!' + rand() % ('~' - '!'); //generates random ascii from 0x21 (!) to 0x7E (~)
	}
	str[len] = '\0';
	return str;
}