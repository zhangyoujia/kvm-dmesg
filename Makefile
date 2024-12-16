TARGET := kvm-dmesg
Q := @
CC := $(CROSS_COMPILE)gcc
CFLAGS := -std=gnu99 -Wall -Wextra -O2
LDFLAGS := -ldl

ifeq ($(STATIC), y)
	LDFLAGS += -static
endif

SRC = main.c \
	  log.c \
    kernel.c \
	  version.c \
	  global_data.c \
	  symbols.c \
	  libvirt_client.c \
	  printk.c \
	  xutil.c \
	  qmp_client.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(Q) echo "  LD      " $@
	$(Q) $(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(Q) echo "  CC      " $@
	$(Q) $(CC) -c $< -o $@ $(CFLAGS)

test: $(TARGET)
	$Q bash tests/base.sh

clean:
	$(Q) $(RM) $(OBJ) $(TARGET) tags

tags:
	$(Q) echo "  GEN" $@
	$(Q) rm -f tags
	$(Q) find . -name '*.[hc]' -print | xargs ctags -a

.PHONY: all clean tags
