#include <string>

#include <stdio.h>

#include <sink.hh>

void Sink::receiveData(const char *data, size_t len) {
    write(STDOUT_FILENO, data, len);
}
