

EXECUTABLE = ../dav
LIBKRELLM_ROOT := ../libkrellm
BUILDDIR = /tmp/build-dav

MODULES   := glcd utils

LIBKRELLM_DIRS=$(addprefix $(LIBKRELLM_ROOT)/,$(MODULES))
#$(info $(LIBKRELLM_DIRS))

INCLUDES  := $(addprefix -I,$(LIBKRELLM_DIRS))
#$(info $(INCLUDES))

MMAL_INCLUDE ?=	-I/opt/vc/include \
				-I/opt/vc/include/interface/vcos/pthreads \
				-I/opt/vc/include/interface/vmcs_host/linux

MMAL_LIB ?= -L/opt/vc/lib -lbcm_host -lvcos -lmmal -lmmal_core -lmmal_util \
				-lmmal_vc_client


FLAGS = -O2 -Wall $(MMAL_INCLUDE) $(INCLUDES)
LIBS = $(MMAL_LIB) -lm -lpthread -lasound -lmp3lame

LOCAL_SRC = dav.c mmalcam.c motion.c event.c display.c config.c \
			servo.c preset.c sunriset.c multicast.c audio.c \
			tcpserver.c tcpserver.c tcpserver_mjpeg.c

KRELLMLIB_SRC = $(wildcard $(addsuffix /*.c,$(LIBKRELLM_DIRS)))
SOURCES = $(LOCAL_SRC) $(KRELLMLIB_SRC)
#$(info $(SOURCES))

KRELLMLIB_DEPS = $(wildcard $(addsuffix /*.h,$(LIBKRELLM_DIRS)))
DEPS = dav.h $(KRELLMLIB_DEPS)
#$(info $(DEPS))

OBJECTS = $(addprefix $(BUILDDIR)/, $(notdir $(SOURCES:%.c=%.o)))
#$(info $(OBJECTS))


VPATH = $(LIBKRELLM_DIRS)
$(BUILDDIR)/%.o: %.c $(DEPS)
	$(CC) -c $(FLAGS) $< -o $@


all: dir $(EXECUTABLE)

dir:
	mkdir -p $(BUILDDIR)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $^ -o $(EXECUTABLE) $(LIBS)

clean:
	rm -f $(BUILDDIR)/*o $(EXECUTABLE)
