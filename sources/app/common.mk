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
export CXXFLAGS+=-Wall -fPIC -fno-strict-overflow -fno-delete-null-pointer-checks -fwrapv -msse4 -std=c++11

ifeq ($(ver), debug)
	CXXFLAGS += -g
else
	CXXFLAGS += -DNDEBUG
endif

SRCEXT=.c .cpp
SRC=$(wildcard $(addprefix *, $(SRCEXT)))
#OBJDIR:=.objs
#OBJS:=$(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRC)))
OBJDIR?=../../objs/$(notdir $(CURDIR))
OBJS+=$(patsubst %.cpp, $(OBJDIR)/%.o, $(notdir $(SRC)))
INCLUDES_PATH+=-I. -I../../libxcode/
LIBS+= ../../dist/libxcodeos.a -lpthread

all: $(DYN_TARGET) $(STATIC_TARGET) install
.PHONY:all install

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $(INCLUDES_PATH) $(CUSTOM_DEFINES) $(EXTRA_FLAGS) -c $< -o $@

$(DYN_TARGET): $(OBJS)
	@echo "Building dyn_target $@"
	$(CXX) -o $@ $^ $(SHARED_LIBS) $(LIBS)

$(STATIC_TARGET): $(OBJS)
	@echo "Building static target $@"
	$(CXX) -o $@ $^ $(STATIC_LIBS) $(LIBS)

clean:
	-rm -rf $(OBJDIR) $(DYN_TARGET) $(STATIC_TARGET)
