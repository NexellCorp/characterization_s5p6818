ifndef TOPDIR
TOPDIR := ../..
endif

include ./env.mk

FRAMEWORKDIR	:= framework
PROTOTYPEDIR	:= prototype
######################################################################
# Build options
INCLUDE += -I./
INCLUDE += -I$(FRAMEWORKDIR)/include
INCLUDE += -I$(FRAMEWORKDIR)/config
INCLUDE += -I$(PROTOTYPEDIR)/nx_base
INCLUDE += -I$(PROTOTYPEDIR)/module

LIBRARY	+= -L./prototype -lprototype
CFLAGS	+= -D__LINUX__ -DNX_RELEASE
CFLAGS  += -static
######################################################################
# Target
COBJS  	:= main.o map.o
TARGET 	:= mipi_test
SUB_DIR	:= $(PROTOTYPEDIR)

BUILD_NUMBER_FILE=build_number_$(TARGET)
######################################################################
# Build
OBJS	:= $(COBJS)

all: $(TARGET)

build_subdir:
	@for dir in $(SUB_DIR); do \
	make -C $$dir || exit $?;	\
	done

$(TARGET): depend $(OBJS) build_subdir $(BUILD_NUMBER_FILE).o
	$(CC) $(CFLAGS) $(OBJS) $(BUILD_NUMBER_FILE).o -o $@ $(LIBRARY)
	cp -f $(TARGET) $(TOPDIR)/bin/

clean:
	@rm -rf $(BUILD_NUMBER_FILE).c
	@rm -rf $(BUILD_NUMBER_FILE).o
	@find $(OBJTREE) -type f \
		\( -name '*.o'	-o -name $(TARGET) -o -name '*.map' -o -name '*.a' -o -name '*.so.*' \) \
		-print | xargs rm -f
    

#########################################################################
APPNAME:= $(TARGET)
MAJOR  := 1
MINOR  := 0
$(BUILD_NUMBER_FILE).o: $(OBJS)
	@if [ -e $(BUILD_NUMBER_FILE).txt ]; then \
	echo $$(($$(cat $(BUILD_NUMBER_FILE).txt) + 1)) > $(BUILD_NUMBER_FILE).txt ;\
	else echo 1 > $(BUILD_NUMBER_FILE).txt ; fi;
	@echo
	@echo "Update Build Number: $(APPNAME) version $(MAJOR).$(MINOR).$$(cat $(BUILD_NUMBER_FILE).txt) (built by $$(whoami) at $$(date) )"
	@echo
	@echo "const char* __$(APPNAME)_VERSION_STRING__ = \"$(APPNAME) version $(MAJOR).$(MINOR).$$(cat $(BUILD_NUMBER_FILE).txt) (built by $$(whoami) at \" __DATE__ \",\" __TIME__ \")\";" > $(BUILD_NUMBER_FILE).c
	$(CC) -c $(BUILD_NUMBER_FILE).c -o $(BUILD_NUMBER_FILE).o

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif
SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	$(CC)  -M  $(CFLAGS)   $(INCS) $(SRCS) > .depend
