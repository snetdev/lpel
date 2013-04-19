SUBDIRS = include . tests

lib_LTLIBRARIES = liblpel.la liblpel_mon.la liblpel_hrc.la

liblpel_la_SOURCES = \
	src/mailbox.c \
	src/streamset.c \
	src/timing.c \
	src/lpelcfg.c \
	src/lpel_main.c \
	src/buffer/bounded.c \
	src/sched/common/task.c \
	src/sched/common/stream.c \
	src/sched/decentralised/decen_scheduler.c \
	src/sched/decentralised/decen_scheduler.h \
	src/sched/decentralised/spmdext.c \
	src/sched/decentralised/spmdext.h \
	src/sched/decentralised/decen_task.c \
	src/sched/decentralised/task_migration.h \
	src/sched/decentralised/task_migration.c \
	src/sched/decentralised/decen_worker.c \
	src/sched/decentralised/decen_worker.h \
	src/sched/decentralised/decen_taskqueue.c \
	src/sched/decentralised/decen_taskqueue.h \
	src/sched/decentralised/decen_stream.c

liblpel_la_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/include

liblpel_hrc_la_SOURCES = \
	src/mailbox.c \
	src/streamset.c \
	src/timing.c \
	src/lpelcfg.c \
	src/lpel_main.c \
	src/buffer/unbounded_ll.c \
	src/sched/common/task.c \
	src/sched/common/stream.c \
	src/sched/hierarchy/hrc_task.c \
	src/sched/hierarchy/hrc_task.h \
	src/sched/hierarchy/hrc_worker.c \
	src/sched/hierarchy/hrc_worker.h \
	src/sched/hierarchy/hrc_taskqueue.c \
	src/sched/hierarchy/taskpriority.h \
	src/sched/hierarchy/taskpriority.c \
	src/sched/hierarchy/hrc_stream.c

liblpel_hrc_la_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src/include


liblpel_mon_la_SOURCES = \
	modimpl/monitoring.c \
	modimpl/monitoring.h
liblpel_mon_la_CPPFLAGS = -I$(top_srcdir)/include


if USE_MCTX_PCL
liblpel_la_LIBADD = $(LIBPCL_LA)
liblpel_la_CPPFLAGS += -DUSE_MTX_PCL
liblpel_hrc_la_LIBADD = $(LIBPCL_LA)
liblpel_hrc_la_CPPFLAGS += -DUSE_MTX_PCL
endif
if USE_MCTX_X86
liblpel_la_SOURCES += src/ctx/ctx_i386.S
liblpel_hrc_la_SOURCES += src/ctx/ctx_i386.S
endif
if USE_MCTX_X86_64
liblpel_la_SOURCES += src/ctx/ctx_amd64.S
liblpel_hrc_la_SOURCES += src/ctx/ctx_amd64.S
endif
if USE_MCTX_X86_64_MEM
liblpel_la_SOURCES += src/ctx/ctx_amd64-mem.S
liblpel_hrc_la_SOURCES += src/ctx/ctx_amd64-mem.S
endif

ACLOCAL_AMFLAGS = -I m4
