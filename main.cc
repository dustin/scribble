#include <iostream>
#include <cassert>

#include <stdio.h>
#include <unistd.h>
#include <sysexits.h>

#include <event.h>

#include <source.hh>
#include <sink.hh>

int main(int argc, char **argv) {

    assert(argc > 1);

    struct event_base *ev = event_init();
    assert(ev);

    Sink sink;
    (void)Source::mk(&sink, argv[1]);

    event_dispatch();

    return 0;
}
