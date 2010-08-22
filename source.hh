#ifndef SOURCE_HH
#define SOURCE_HH 1

#include <event.h>

extern "C" {
    void src_handler(int fd, short which, void *arg);
}

class Source {
public:
    Source() : fd(-1) {}
    virtual ~Source() {}

    // Return false when we're closed.
    virtual bool handle(short which);

    static Source* mk(const char *spec);
    static Source* mk(int srcfd);

protected:
    virtual void initialize(const char *spec) = 0;

    struct event ev;
    int fd;
};

class FileSource : public Source {
    void initialize(const char *spec);
};

class TcpSource : public Source {
public:

    bool handle(short which);

private:
    void initialize(const char *spec);
};

class FileDescriptorSource : public Source {
private:
    void initialize(const char *spec);
};

#endif /* SOURCE_HH */
