project('kvm-dmesg', 'c', default_options : ['c_std=gnu99'])

# Compiler options
cflags = ['-Wall', '-Wextra', '-O2']

# Linker options
ldflags = ['-ldl']

# Sources
sources = [
  'main.c',
  'log.c',
  'version.c',
  'kernel.c',
  'global_data.c',
  'symbols.c',
  'libvirt_client.c',
  'printk.c',
  'xutil.c',
  'qmp_client.c',
]

# Build executable
executable('kvm-dmesg',
  sources,
  c_args       : cflags,
  link_args    : ldflags
)
