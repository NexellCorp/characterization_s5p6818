########################################################################
# Get Linux	Build Enviornment:
ifndef TOPDIR
TOPDIR := ../..
endif

include	../../env.mk
ARCH           := S5P6818
######################################################################
# Build	options
INCLUDE	+= -I./	-I./khronos/ 
LIBRARY	+= -L./lib/ -lMali -lEGL -lGLESv1_CM  -ldl -lm
LIBRARY	+= -lstdc++	-lpthread

######################################################################
# Target
COBJS	:=
CPPOBJS	:= OGLEarthSphere.o ES_texture.o V6_sphere_980.o V6_sphere_9680.o Fake_OS.o GLFont.o __font_modify_tga.o main.o
TARGET	:= asb_graphic

#CPPFLAGS += -g

######################################################################
# Build
OBJS	:= $(COBJS)	$(CPPOBJS)

all: $(TARGET)

$(TARGET): depend $(OBJS)
	$(CC) $(CPPFLAGS) $(OBJS) -o $@	$(LIBRARY)
	#cp -f $(TARGET)	../../release/
	#sudo cp -f $(TARGET)	../../../../../../devel/nfs/kernel_rootfs-$(ARCHNAME)/test/
	cp $(TARGET) $(TOPDIR)/bin
	
clean:
	@find $(OBJTREE) -type f \
		\( -name '*.o'	-o -name $(TARGET) -o -name	'*.map'	\) -print \
		| xargs	rm -f

#########################################################################
# Dependency
ifeq (dependency_list_$(TARGET),$(wildcard dependency_list_$(TARGET)))
include	dependency_list_$(TARGET)
endif

SRCS :=	$(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS :=	$(INCLUDE)
depend dep:
	$(CC)  -M  $(CPPFLAGS) $(CFLAGS) $(INCS) $(INCLUDE)	$(SRCS)	> dependency_list_$(TARGET)



