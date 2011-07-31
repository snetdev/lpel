noinst_PROGRAMS = lpel lpel2

lpel_SOURCES = check_lpel.c
lpel2_SOURCES = check_lpel2.c

CPPFLAGS = -I$(top_srcdir)/include
LDADD = $(top_builddir)/liblpel.la $(top_builddir)/liblpel_mon.la

SUBDIRS = comp_pthreads
