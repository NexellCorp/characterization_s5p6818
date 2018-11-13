######################################################################
SUB_DIRS	:=
SUB_DIRS	+= main
SUB_DIRS	+= vpu_test
SUB_DIRS	+= vr_test

all: compile_subdirs 

compile_subdirs:
	@for dir in $(SUB_DIRS); do \
	make -C $$dir || exit $?;	\
	done

clean:
	@for dir in $(SUB_DIRS); do \
	make -C $$dir clean;		\
	done
