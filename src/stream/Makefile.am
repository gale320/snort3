## $Id
AUTOMAKE_OPTIONS=foreign no-dependencies

noinst_LIBRARIES = libstream.a

x_includedir = $(pkgincludedir)/stream

x_include_HEADERS = \
stream_api.h \
stream_splitter.h

libstream_a_SOURCES = \
flush_bucket.cc \
flush_bucket.h \
stream.h \
stream_api.cc \
stream_inspectors.cc \
stream_inspectors.h \
stream_splitter.cc

# FIXIT LIBADD here causes undefineds
#libstream_a_LIBADD = \
#base/libstream_base.a
#ip/libstream_ip.a
#icmp/libstream_icmp.a
#tcp/libstream_tcp.a
#udp/libstream_udp.a

SUBDIRS = \
base \
ip \
icmp \
tcp \
udp

AM_CXXFLAGS = @AM_CXXFLAGS@

