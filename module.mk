# Define the relative path to this source code, as seen by the main makefile,
# by calling the $(subdirectory) function from the main makefile. The
# local_root variable must then be used to prefix all paths in the rest
# of this makefile.
local_root := $(subdirectory)

# A list of directories that makes up the public API of this common object.
# Users of this common object can #include headers in these folders without any
# relative path to the files (#include "public.h"). In most cases this should
# only be $(local_root)/inc. Header files located in these folders must only
# #include header files from the public API of this or other common objects.
local_inc_dirs := $(local_root)/inc

# A list of directories containing the source code of this common object.
# They will contain both .c-files and object private .h-files. Relative
# paths must be used in #include directives. I.e. if a folder hierarchy is used
# to group code into sub modules, headers must be include like
# #include "submodule/subheader.h" or #include "../srcheader.h".
# Header files located in these folders must only #include header files from
# its own private source code or the public API of this or other common objects.

#local_src_dirs := $(local_root)/src \


#local_src_dirs := $(local_root)/src \
#                  $(local_root)/par \

#local_src_dirs := $(local_root)/src \
#                  $(local_root)/src/spi \

local_src_dirs := $(local_root)/src \
                  $(local_root)/src/par \
                  $(local_root)/src/spi \
                  $(local_root)/src/serial

# A list of all the .c-files that are to be compiled. Automatically traverses
# all folders set up in local_src and adds all .c-files to the list.
local_src := $(foreach var, $(local_src_dirs), $(wildcard $(var)/*.c) )

# Adds this common object's source code to the main makefile by calling the
# add-module function in it.
$(eval $(call add-module, $(local_src), $(local_inc_dirs), $(local_src_dirs)))

# Common object specific makefile targets and extension can also be added. The
# possibility to use a specific optimization level for this object or suppress
# all warnings it generates exists. Refer to the software documentation of the
# makefiles project for this.