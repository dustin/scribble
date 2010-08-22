#include <iostream>
#include <cassert>

#include <stdio.h>
#include <unistd.h>
#include <sysexits.h>

#include <event.h>

#include <source.hh>

int main(int argc, char **argv) {

    assert(argc > 1);

    std::cout << "Going..." << std::endl;

    struct event_base *ev = event_init();
    assert(ev);

    Source *src = Source::mk(argv[1]);
    (void)src;

    event_dispatch();

    return 0;
}
