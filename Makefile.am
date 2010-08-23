ACLOCAL_AMFLAGS = -I m4 --force

bin_PROGRAMS = scribble

scribble_SOURCES = main.cc source.hh source.cc sink.hh sink.cc
scribble_LDADD = ${LTLIBEVENT}

check_PROGRAMS=
TESTS=${check_PROGRAMS}

test: check-TESTS
