#include <iostream>
#include <string>
#include <cassert>

#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sysexits.h>

#include <event.h>

#include <source.hh>
#include <sink.hh>

extern "C" {
    void src_handler(int fd, short which, void *arg) {
        (void)fd;
        Source *src = static_cast<Source *>(arg);
        if (!src->handle(which)) {
            delete src;
        }
     }
}

Source* Source::mk(Sink *sink, const char *spec) {
    Source *rv(NULL);
    const char *downSpec(spec);
    if (strncmp(spec, "file://", 7) == 0) {
        downSpec = spec + 7;
        rv = new FileSource(sink);
    } else if (strncmp(spec, "tcp://", 6) == 0) {
        downSpec = spec + 6;
        rv = new TcpSource(sink);
    } else if (strcmp(spec, "-") == 0) {
        rv = new FileDescriptorSource(sink);
        rv->fd = STDIN_FILENO;
    } else {
        std::cerr << "Unhandled src type:  " << spec << std::endl;
        exit(EX_USAGE);
    }
    rv->initialize(downSpec);
    assert(rv->fd >= 0);

    event_set(&rv->ev, rv->fd, EV_READ|EV_PERSIST, src_handler, rv);
    event_add(&rv->ev, 0);

    return rv;
}

Source* Source::mk(Sink *sink, int srcfd) {
    FileDescriptorSource *rv = new FileDescriptorSource(sink);
    rv->fd = srcfd;

    event_set(&rv->ev, rv->fd, EV_READ|EV_PERSIST, src_handler, rv);
    event_add(&rv->ev, 0);

    return rv;
}

bool Source::handle(short which) {
    assert(which == EV_READ);

    char buf[8192];
    ssize_t bytesread = read(fd, buf, sizeof(buf));
    switch(bytesread) {
    case -1:
        perror("read");
        // FALLTHROUGH
    case 0:
        if (event_del(&ev) != 0) {
            std::cerr << "Failed to remove an event." << std::endl;
            abort();
        }
        if (close(fd) != 0) {
            perror("close");
        }
        return false;
        break;
    default:
        sink->receiveData(buf, bytesread);
    }

    return true;
}

void FileSource::initialize(const char *spec) {
    fd = open(spec, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        exit(EX_OSFILE);
    }
}

void FileDescriptorSource::initialize(const char *spec) {
    (void)spec;
}

bool TcpSource::handle(short which) {
    assert(which == EV_READ);

    struct sockaddr_in addr;
    socklen_t addrlen;
    int newsock;
    if ((newsock = accept(fd, (sockaddr*)&addr, &addrlen)) < 0) {
        perror("accept");
        exit(EX_OSERR);
    }

    (void)Source::mk(sink, newsock);
    return true;
}

void TcpSource::initialize(const char *spec) {
    std::string sspec(spec);
    size_t colon = sspec.find(":");
    if (colon == std::string::npos) {
        std::cerr << "tcp spec format error (wants tcp://i.p.ad.dr:port)" << std::endl;
        exit(EX_USAGE);
    }

    std::string hosts = sspec.substr(0, colon);
    std::string ports = sspec.substr(colon + 1);

    int error = 0;
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((error = getaddrinfo(hosts.c_str(), ports.c_str(), &hints, &res)) != 0) {
        std::cerr << "Failed to set up binding: " << gai_strerror(error) << std::endl;
        exit(EX_USAGE);
    }

    (void)spec;
    fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) {
        perror("socket");
        exit(EX_OSERR);
    }

    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&reuse, sizeof(int)) != 0) {
        perror("setsockopt(SO_REUSEADDR)");
        exit(EX_OSERR);
    }

    if (bind(fd, res->ai_addr, res->ai_addrlen) != 0) {
        perror("bind");
        exit(EX_OSERR);
    }

    if (listen(fd, 16) != 0) {
        perror("listen");
        exit(EX_OSERR);
    }

    freeaddrinfo(res);
}
