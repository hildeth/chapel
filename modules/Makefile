# Copyright 2004-2015 Cray Inc.
# Other additional copyright holders may be indicated within.
# 
# The entirety of this work is licensed under the Apache License,
# Version 2.0 (the "License"); you may not use this file except
# in compliance with the License.
# 
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

ifndef CHPL_MAKE_HOME
export CHPL_MAKE_HOME=$(shell pwd)/..
endif

CHPL_MAKE_HOST_TARGET = --target
include $(CHPL_MAKE_HOME)/make/Makefile.base

SYS_MODULES_DIR=standard/gen/$(CHPL_MAKE_PLATFORM)-$(CHPL_MAKE_COMPILER)
SYS_CTYPES_MODULE_DOC=standard/gen/doc/SysCTypes.chpl
SYS_CTYPES_MODULE=$(SYS_MODULES_DIR)/SysCTypes.chpl

default: all

all: $(SYS_CTYPES_MODULE)
	-@($(CHPL_MAKE_HOME)/util/chpltags -r . > /dev/null 2>&1 && echo "Updating TAGS...")

clean: FORCE

cleanall: FORCE

clobber: FORCE clean-documentation
	rm -rf standard/gen

depend:

MAKE_SYS_BASIC_TYPES=$(CHPL_MAKE_HOME)/util/config/make_sys_basic_types.py

$(SYS_CTYPES_MODULE): $(MAKE_SYS_BASIC_TYPES)
	mkdir -p $(@D)
	cd $(@D) && $(MAKE_SYS_BASIC_TYPES) $(@F)

$(SYS_CTYPES_MODULE_DOC): $(MAKE_SYS_BASIC_TYPES)
	mkdir -p $(@D)
	cd $(@D) && $(MAKE_SYS_BASIC_TYPES) --doc $(@F)


MODULES_TO_DOCUMENT = \
	standard/AdvancedIters.chpl \
	standard/Assert.chpl \
	standard/AutoPtr.chpl \
	standard/BitOps.chpl \
	standard/Buffers.chpl \
	standard/CommDiagnostics.chpl \
	standard/Curl.chpl \
	standard/FFTW.chpl \
	standard/FFTW_MT.chpl \
	standard/FileSystem.chpl \
	standard/GMP.chpl \
	standard/HDFS.chpl \
	standard/HDFSiterator.chpl \
	standard/Help.chpl \
	standard/IO.chpl \
	standard/List.chpl \
	standard/Math.chpl \
	standard/Memory.chpl \
	standard/Norm.chpl \
	standard/Path.chpl \
	standard/Random.chpl \
	standard/Regexp.chpl \
	standard/Search.chpl \
	standard/Sort.chpl \
	standard/Sys.chpl \
	standard/Types.chpl \
	standard/UtilReplicatedVar.chpl \
	$(SYS_CTYPES_MODULE_DOC)

documentation: $(SYS_CTYPES_MODULE_DOC)
	export CHPLDOC_AUTHOR='Cray Inc' && \
	$(CHPLDOC) $(MODULES_TO_DOCUMENT)

clean-documentation:
	rm -rf ./docs

FORCE:

