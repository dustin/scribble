#ifndef SOURCE_HH
#define SOURCE_HH 1

#include <event.h>

extern "C" {
    void src_handler(int fd, short which, void *arg);
}

class Sink;

class Source {
public:
    Source(Sink *s) : sink(s), fd(-1) {}
    virtual ~Source() {}

    // Return false when we're closed.
    virtual bool handle(short which);

    static Source* mk(Sink *sink, const char *spec);
    static Source* mk(Sink *sink, int srcfd);

protected:
    virtual void initialize(const char *spec) = 0;

    Sink *sink;
    struct event ev;
    int fd;
};

class FileSource : public Source {
public:
    FileSource(Sink *s) : Source(s) {}
private:
    void initialize(const char *spec);
};

class TcpSource : public Source {
public:
    TcpSource(Sink *s) : Source(s) {}
    bool handle(short which);

private:
    void initialize(const char *spec);
};

class FileDescriptorSource : public Source {
public:
    FileDescriptorSource(Sink *s) : Source(s) {}
private:
    void initialize(const char *spec);
};

#endif /* SOURCE_HH */
