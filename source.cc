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
#include <sysexits.h>

#include <event.h>

#include <source.hh>

extern "C" {
    void src_handler(int fd, short which, void *arg) {
        (void)fd;
        Source *src = static_cast<Source *>(arg);
        if (!src->handle(which)) {
            delete src;
        }
     }
}

Source* Source::mk(const char *spec) {
    Source *rv(NULL);
    const char *downSpec(spec);
    if (strncmp(spec, "file://", 7) == 0) {
        downSpec = spec + 7;
        rv = new FileSource;
    } else if (strncmp(spec, "tcp://", 6) == 0) {
        downSpec = spec + 6;
        rv = new TcpSource;
    } else if (strcmp(spec, "-") == 0) {
        rv = new FileDescriptorSource;
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

Source* Source::mk(int srcfd) {
    FileDescriptorSource *rv = new FileDescriptorSource;
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
        write(STDOUT_FILENO, buf, bytesread);
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

    (void)Source::mk(newsock);
    return true;
}

void TcpSource::initialize(const char *spec) {
    (void)spec;
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(EX_OSERR);
    }

    std::string sspec(spec);
    size_t colon = sspec.find(":");
    if (colon == std::string::npos) {
        std::cerr << "tcp spec format error (wants tcp://i.p.ad.dr:port)" << std::endl;
        exit(EX_USAGE);
    }

    std::string hosts = sspec.substr(0, colon);
    std::string ports = sspec.substr(colon + 1);

    int port = atoi(ports.c_str());

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    switch (inet_pton(AF_INET, hosts.c_str(), &addr.sin_addr)) {
    case 0:
        std::cerr << "Invalid format for host addr." << std::endl;
        exit(EX_USAGE);
        break;
    case -1:
        perror("inet_pton");
        exit(EX_OSERR);
    }

    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&reuse, sizeof(int)) != 0) {
        perror("setsockopt(SO_REUSEADDR)");
        exit(EX_OSERR);
    }

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("bind");
        exit(EX_OSERR);
    }

    if (listen(fd, 16) != 0) {
        perror("listen");
        exit(EX_OSERR);
    }
}
