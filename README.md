Illinix391-SaenaiOS
===================

ECE 391 SP18 MP3 by Group 39 "Blessing Hardware".

*&copy; 2018 by Fang Lu, Xutao Jiang, Xi Chen, Yiyi Wang*

Illinix391-SaenaiOS is a UNIX-like operating system from the UIUC ECE 391 8-week
course project. We got 1st place in the Spring 2018 ECE 391 Design Competition.
The system features partial POSIX-compliant system calls.

*The project has ended and is no longer maintained. You are welcome to discuss
or fork the project, but the developers make no promise to any responses or
bug fixes.*

Compiling
---------

To build the kernel, invoke `make` in the `student-distrib` folder. You will
need a i386-elf-gcc toolchain to do that. (The gcc from the ECE391 devel VM
will suffice. You just need to make a symlink or modify the `CC` variable in the
Makefile).

If the previous step complains about no rule to make libext4.a, you can get one
by invoking `make` in the `fuse-lwext4/lwext4/src` folder.

To create the bootable image, invoke `makeos.sh` in the root directory. To do
this, your system must have a `mount` utility that supports ext2, and you will
need root privileges to perform that `mount`.

If the previous step complains about missing `mp3.img`, you can retrieve a
template from ECE391 Staff by invoking `git checkout initial mp3.img` under the
`student-distrib` directory. Note that the template image contains GRUB, and
will make the OS GPL licensed.

Running
-------

The resulting mp3.img is a raw disk image. To run in QEMU, invoke

```
qemu-system-i386 -hda "student-distrib/mp3.img" -m 256 -name SaenaiOS
```

To run in VMWare, you can convert the image to VMDK with QEMU-img:

```
qemu-img convert -f raw -O vmdk mp3.img mp3.vmdk
```

The img file could also be burnt into a USB stick for booting in a physical
BIOS-capable machine. However, ATA disk driver will not work unless you burnt
the image into an actual hard disk connected on an IDE bus.

Documentation
-------------

The documentation of the project can be generated using doxygen. To do that,
invoke `doxygen` under `student-distrib` and `libc`.

A copy of the HTML doxygen output is hosted here:
[kernel](https://fanglu2.web.engr.illinois.edu/ece391/kernel/files.html),
[libc](https://fanglu2.web.engr.illinois.edu/ece391/libc/files.html)

License
-------

The kernel of SaenaiOS is licensed under the MIT License. However, components in
some submodules (like gcc and newlib) are licensed under the GPL. If you choose
to distribute the operating system with these toolchains bundled, the OS would
be licensed under the GPL. Specifically, the `mp3.img` disk image contains a
copy of GRUB, which is GPLv3. Thus using the `debug.sh` script will make the
OS bootable image GPL. Please refer to the `LICENSE` file of this repo and its
submodules for detailed license text.

**Additional Notes to Future ECE391 Students:** Please refer to the Student Code
for academic integrity requirements. You may use code from this project freely
under the license terms, but it is your responsibility to ensure that such use
meets the originality requirements from your course and department.
