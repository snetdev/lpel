SUBDIRS = include . tests

lib_LTLIBRARIES = liblpel.la liblpel_mon.la

liblpel_la_SOURCES = \
	src/arch/atomic-pthread.h \
	src/arch/atomic-builtin.h \
	src/arch/atomic-x86.h \
	src/arch/atomic.h \
	src/arch/sysdep.h \
	src/arch/mctx.h \
	src/arch/mctx-pcl.h \
	src/arch/mctx-setjmp.h \
	src/arch/mctx-x86.h \
	src/arch/mctx-ucontext.h \
	src/mailbox.c \
	src/mailbox.h \
	src/buffer.c \
	src/buffer.h \
	src/scheduler.c \
	src/scheduler.h \
        src/spmdext.c \
        src/spmdext.h \
	src/stream.c \
	src/stream.h \
	src/streamset.c \
	src/task.c \
	src/task.h \
	src/taskqueue.c \
	src/taskqueue.h \
	src/timing.c \
	src/lpelcfg.c \
	src/lpelcfg.h \
	src/lpel_main.c \
	src/lpel_main.h \
	src/placementscheduler.c \
	src/placementscheduler.h \
	src/taskiterator.h \
	src/taskiterator.c \
	src/malloc.c \
	src/worker.c \
	src/worker.h \
	src/workermsg.h
liblpel_la_CPPFLAGS = -I$(top_srcdir)/include

liblpel_mon_la_SOURCES = \
	modimpl/monitoring.c \
	modimpl/monitoring.h
liblpel_mon_la_CPPFLAGS = -I$(top_srcdir)/include


if USE_MCTX_PCL
liblpel_la_LIBADD = $(LIBPCL_LA)
liblpel_la_CPPFLAGS += -DUSE_MTX_PCL
endif
if USE_MCTX_X86
liblpel_la_SOURCES += src/ctx/ctx_i386.S
endif
if USE_MCTX_X86_64
liblpel_la_SOURCES += src/ctx/ctx_amd64.S
endif
if USE_MCTX_X86_64_MEM
liblpel_la_SOURCES += src/ctx/ctx_amd64-mem.S
endif

ACLOCAL_AMFLAGS = -I m4
