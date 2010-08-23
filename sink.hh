#ifndef SINK_HH
#define SINK_HH 1

#include <vector>
#include <queue>

#include <event.h>

class Sink;

class Destination {
public:

    Destination(Sink *s) : sink(s), fd(-1), flags(0), active(false) { }
    virtual ~Destination() {
        deactivate();
    }

    bool isActive() { return active; }

    virtual void activate();
    virtual void deactivate();
    virtual void update();
    virtual bool handle(short which);

    static Destination* mk(Sink *sink, const char *spec);
    static Destination* mk(Sink *sink, int srcfd);

protected:
    virtual void initialize(const char *spec) = 0;

    Sink *sink;
    struct event ev;
    int fd;
    int flags;
    bool active;
};

class FileDescriptorDestination : public Destination {
public:
    FileDescriptorDestination(Sink *s) : Destination(s) { }
protected:
    void initialize(const char *spec);
};

class Sink {
public:
    void receiveData(const char *data, size_t len);
    bool hasData() { return !data.empty(); }

    const std::string next() {
        const std::string rv = data.front();
        data.pop();
        return rv;
    }

    void addDestination(Destination* d) {
        d->activate();
        destinations.push_back(d);
    }

private:
    std::vector<Destination*> destinations;
    std::queue<std::string> data;
};

#endif /* SINK_HH */
