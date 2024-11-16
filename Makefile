TARGET = kvm-dmesg
SRC = main.c \
	  log.c \
	  version.c \
	  global_data.c \
	  symbols.c libvirt_client.c \
	  printk.c \
	  xutil.c \
	  qmp_client.c

all: $(TARGET)

$(TARGET): $(SRC)
	gcc -o $@ $^ `pkg-config --cflags --libs libvirt libvirt-qemu` -std=gnu99

tags:
	ctags -R

clean:
	rm -f tags
	rm -f $(TARGET)
