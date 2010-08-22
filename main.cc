#include <iostream>
#include <cassert>

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sysexits.h>

#include <event.h>

extern "C" {
    static void src_handler(int fd, short which, void *arg) {
        assert(which == EV_READ);
        struct event *ev = static_cast<struct event*>(arg);

        char buf[8192];
        ssize_t bytesread = read(fd, buf, sizeof(buf));
        switch(bytesread) {
        case -1:
            perror("read");
            // FALLTHROUGH
        case 0:
            std::cerr << "Closing " << fd << std::endl;
            if (event_del(ev) != 0) {
                std::cerr << "Failed to remove an event." << std::endl;
                abort();
            }
            free(ev);
            if (close(fd) != 0) {
                perror("close");
            }
            break;
        default:
            write(STDOUT_FILENO, buf, bytesread);
        }
    }

    static void conn_handler(int fd, short which, void *arg) {
        assert(which == EV_READ);
        (void)arg;

        std::cout << "Accepting a connection." << std::endl;

        struct sockaddr_in addr;
        socklen_t addrlen;
        int newsock;
        if ((newsock = accept(fd, (sockaddr*)&addr, &addrlen)) < 0) {
            perror("accept");
            exit(EX_OSERR);
        }

        struct event *ev = static_cast<struct event*>(calloc(sizeof(struct event), 1));
        event_set(ev, newsock, EV_READ|EV_PERSIST, src_handler, ev);
        event_add(ev, 0);
    }
}

static int openFileSrc(const char *path) {
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("open");
        exit(EX_OSFILE);
    }

    return fd;
}

static int openTcpSrc(const char *spec) {
    (void)spec;
    int fd = socket(PF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(EX_OSERR);
    }

    int port = 6789;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

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

    return fd;
}

static void initSrc(const char *path) {

    int fd(-1);
    struct event *src_ev(NULL);

    if (strncmp(path, "file://", 7) == 0) {
        fd = openFileSrc(path + 7);
    } else if (strncmp(path, "tcp://", 6) == 0) {
        fd = openTcpSrc(path + 6);
        src_ev = static_cast<struct event*>(calloc(sizeof(struct event), 1));
        assert(src_ev);
        event_set(src_ev, fd, EV_READ|EV_PERSIST, conn_handler, NULL);
    } else if (strcmp(path, "-") == 0) {
        fd = STDIN_FILENO;
    } else {
        std::cerr << "Unhandled src type:  " << path << std::endl;
        exit(EX_USAGE);
    }
    assert(fd >= 0);

    if (!src_ev) {
        src_ev = static_cast<struct event*>(calloc(sizeof(struct event), 1));
        event_set(src_ev, fd, EV_READ|EV_PERSIST, src_handler, src_ev);
    }
    event_add(src_ev, 0);
}

int main(int argc, char **argv) {

    assert(argc > 1);

    std::cout << "Going..." << std::endl;

    struct event_base *ev = event_init();
    assert(ev);

    initSrc(argv[1]);

    event_dispatch();

    return 0;
}
