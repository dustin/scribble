#ifndef SINK_HH
#define SINK_HH 1

#include <vector>

class Sink {
public:
    void receiveData(const char *data, size_t len);
};

#endif /* SINK_HH */
