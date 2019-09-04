# 
#   Copyright(c) 2011-2016 Intel Corporation. All rights reserved.
# 
#   This library is free software; you can redistribute it and/or
#   modify it under the terms of the GNU Lesser General Public
#   License as published by the Free Software Foundation; either
#   version 2.1 of the License, or (at your option) any later version.
# 
#   This library is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   Lesser General Public License for more details.
# 
#   You should have received a copy of the GNU Lesser General Public
#   License along with this library.  If not, see <http://www.gnu.org/licenses/>.
# 
#   Contact Information:
#   Intel Corporation
# 
# 
#  version: SMTA.A.0.9.4-2
export CXX=g++
export CXXFLAGS=-Wall -fPIC  -fno-strict-overflow -fno-delete-null-pointer-checks -fwrapv -msse4 -std=c++11
export STATIC_LIBRARY_LINK=ar cr

export LIB_PREFIX=libxcode

ifeq ($(ver), debug)
	CXXFLAGS += -g
else
	CXXFLAGS += -DNDEBUG
endif

LIB_NAME=$(shell basename `pwd`)
STATIC_TARGET=../../dist/$(LIB_PREFIX)$(LIB_NAME).a
DYN_TARGET=../../dist/$(LIB_PREFIX)$(LIB_NAME).so

SRCEXT=.c .cpp
SRC=$(wildcard $(addprefix *, $(SRCEXT)))
SRC:=$(filter-out $(EXCLUDE_SRC) ,$(SRC))
SUBDIR:=$(filter-out $(OBJDIR),$(basename $(patsubst ./%,%,$(shell find . -maxdepth 1 -type d))))
SRC+=$(foreach dir,$(SUBDIR),$(shell ls $(dir)/*.cpp))
OBJDIR:=.objs
#OBJS:=$(patsubst %.cpp,$(OBJDIR)/%.o,$(notdir $(SRC)))
OBJS:=$(patsubst %.cpp,$(OBJDIR)/%.o,$(SRC))
INCLUDES_PATH+=-I. -I../

all: $(DYN_TARGET) $(STATIC_TARGET)
.PHONY:all

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)/$(SUBDIR)
	$(foreach dir,$(SUBDIR),$(shell mkdir -p $(OBJDIR)/$(dir)))
	$(CXX) $(CXXFLAGS) $(INCLUDES_PATH) $(EXTRA_FLAGS) -c $< -o $@

$(STATIC_TARGET): $(OBJS)
	$(STATIC_LIBRARY_LINK) $@ $(OBJS)

$(DYN_TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -shared $(OBJS) -o $@ $(LIBS)

clean:
	rm -rf $(STATIC_TARGET) $(OBJDIR) $(DYN_TARGET)
