# kvm-dmesg

`kvm-dmesg` is a tool designed to retrieve kernel logs from a guest virtual machine running under KVM, similar to the `dmesg` tool but for virtualized environments. This tool allows system administrators and developers to easily access the kernel logs of guest VMs directly from the host.

## Features

- **Guest Kernel Log Retrieval**: `kvm-dmesg` enables you to fetch kernel logs from KVM-based virtual machines.
- **Two Connection Methods**: It supports two different interfaces for log retrieval:
  - **libvirt Interface**: Fetch logs from VMs managed through the libvirt virtualization API.
  - **QMP Socket**: Connect directly to the QEMU monitor protocol (QMP) socket for communication.
- **x86_64 Linux VM Support**: This tool currently supports only x86_64 Linux virtual machines.
- **System.map Symbol Table**: `kvm-dmesg` requires access to the guest VM's `System.map` symbol table to map kernel log addresses to human-readable symbols.

## Compilation

To compile the tool, simply run:

```bash
make
```

## Usage

1. **Using libvirt**:
   ```bash
   ./kvm-dmesg <domain_name> <system.map_path>
   ```

2. **Using QMP Socket**:
   ```bash
   ./kvm-dmesg <socket_path> <system.map_path>
   ```

   In both commands, replace `<domain_name>` with the name of the virtual machine, `<socket_path>` with the path to the QMP socket, and `<system.map_path>` with the path to the `System.map` file for the guest kernel.


## Requirements

- Linux-based host with KVM support.
- Libvirt or access to the QMP socket for the virtual machine.
- The tool currently only supports x86_64 Linux guests.
- The guest's `System.map` file must be available for symbol resolution.

## Acknowledgments

This tool is inspired by the functionality provided by the [Crash Utility](https://github.com/crash-utility/crash). Special thanks to the authors of this project for providing a robust set of tools for kernel crash analysis and debugging.

---

Let me know if you'd like to add or modify anything else!
