#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

typedef int64_t i64;

char* i64_to_str_I(i64 number, char* buf) {
    if (number == 0) return buf;
    char* newbuf = i64_to_str_I(number/10, buf);
    *newbuf = '0' + number % 10;
    return newbuf + 1;
}

void i64_to_str(i64 number, char* buf) {
    if (number < 0) {
        *buf = '-';
        buf++;
        number = -number;
    }
    if (number == 0) *buf = '0';
    i64_to_str_I(number, buf);
}

char buffer[1000] = {0};

int main() {
    i64_to_str(-502, buffer);
    puts(buffer);
}