#include <iostream>
#include <string>
#include <algorithm>
#include <cassert>

#include <stdio.h>
#include <sysexits.h>

#include <sink.hh>

extern "C" {
    void dest_handler(int fd, short which, void *arg) {
        (void)fd;
        Destination *dest = static_cast<Destination *>(arg);
        if (!dest->handle(which)) {
            delete dest;
        }
     }
}

void Sink::receiveData(const char *buf, size_t len) {
    std::string s(buf, len);
    data.push(s);
    std::for_each(destinations.begin(), destinations.end(),
                  std::mem_fun(&Destination::update));
}

bool Destination::handle(short which) {
    (void)which;
    bool rv(true);

    if (sink->hasData()) {
        std::string s = sink->next();
        size_t written = write(STDOUT_FILENO, s.data(), s.size());
        assert(written == s.size());
    }

    return rv;
}

Destination* Destination::mk(Sink *sink, const char *spec) {
    Destination *rv(NULL);
    const char *downSpec(spec);
    if (strncmp(spec, "-", 1) == 0) {
        rv = new FileDescriptorDestination(sink);
        rv->fd = STDIN_FILENO;
    } else {
        std::cerr << "Unhandled dest type:  " << spec << std::endl;
        exit(EX_USAGE);
    }
    rv->initialize(downSpec);
    return rv;
}

Destination* Destination::mk(Sink *sink, int srcfd) {
    FileDescriptorDestination *rv = new FileDescriptorDestination(sink);
    rv->fd = srcfd;
    return rv;
}

void Destination::activate() {
    if (!active) {
        active = true;
        update();
    }
}

void Destination::deactivate() {
    if (flags != 0) {
        event_del(&ev);
        flags = 0;
        active = false;
    }
}

 void Destination::update() {
     int desired_flags = EV_WRITE|EV_PERSIST;
     if (active && sink->hasData() && flags != desired_flags) {
         flags = desired_flags;
         event_set(&ev, fd, flags, dest_handler, this);
         event_add(&ev, 0);
     }
 }

void FileDescriptorDestination::initialize(const char *spec) {
    (void)spec;
}
