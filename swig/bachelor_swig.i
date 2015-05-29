/* -*- c++ -*- */

#define BACHELOR_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "bachelor_swig_doc.i"

%{
#include "bachelor/skynet_analyze.h"
#include "bachelor/skynet_wireshark_sink.h"
#include "bachelor/skynet_extended_analyze.h"
%}

%include "bachelor/skynet_analyze.h"
GR_SWIG_BLOCK_MAGIC2(bachelor, skynet_analyze);

%include "bachelor/skynet_wireshark_sink.h"
GR_SWIG_BLOCK_MAGIC2(bachelor, skynet_wireshark_sink);

%include "bachelor/skynet_extended_analyze.h"
GR_SWIG_BLOCK_MAGIC2(bachelor, skynet_extended_analyze);
