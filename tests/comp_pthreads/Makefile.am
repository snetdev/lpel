noinst_PROGRAMS = \
	ringtest pthr_ringtest \
	pipetest pthr_pipetest

pthr_ringtest_SOURCES = pthr_ringtest.c pthr_streams.c error.c
pthr_ringtest_LDADD = $(top_builddir)/liblpel.la
pthr_pipetest_SOURCES = pthr_pipetest.c pthr_streams.c error.c
pthr_pipetest_LDADD = $(top_builddir)/liblpel.la
ringtest_SOURCES = ringtest.c
ringtest_LDADD = $(top_builddir)/liblpel.la
pipetest_SOURCES = pipetest.c
pipetest_LDADD = $(top_builddir)/liblpel.la
CPPFLAGS = -I$(top_srcdir)/include
