/* -*- c++ -*- */

#define SVL_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "svl_swig_doc.i"

%{
#include "svl/svl_sink.h"
#include "svl/svl_source.h"
%}


%include "svl/svl_sink.h"
GR_SWIG_BLOCK_MAGIC2(svl, svl_sink);
%include "svl/svl_source.h"
GR_SWIG_BLOCK_MAGIC2(svl, svl_source);
