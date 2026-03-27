#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # Done: Add your kernel build steps here
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all -j4
    # skip building the modules
    # make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs 
fi

echo "Adding the Image in outdir"
cp "${OUTDIR}/linux-stable/arch/arm64/boot/Image" "${OUTDIR}"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# Done: Create necessary base directoriesi
mkdir -p "${OUTDIR}/rootfs"
cd "${OUTDIR}/rootfs"
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # Done:  Configure busybox
    make distclean
    make defconfig
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu-     
else
    cd busybox
fi

# Done: Make and install busybox
make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- CONFIG_PREFIX="${OUTDIR}/rootfs" install

cd "${OUTDIR}/rootfs"

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# Done: Add library dependencies to rootfs
  # Using sysroot makes sure that the dependencies are found both when running
  # manual and when using the git-runner.
  # See  https://www.coursera.org/learn/linux-system-programming-introduction-to-buildroot/discussions/weeks/2/threads/ulHq06OREe67HQrbxnFcjw
  # Adapted the code inspired by:
  # https://github.com/cu-ecen-aeld/assignments-3-and-later-siva7699/commit/3a10a72b75c74f55d9c4338291e83019a394c1b9#diff-e01e2146bf6f13636a134d3ce13983df0085950949a7b0a3868c3a8d9e0f8eafR87
  SYS_ROOT=$(aarch64-none-linux-gnu-gcc -print-sysroot)
  cp ${SYS_ROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
  cp ${SYS_ROOT}/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64/
  cp ${SYS_ROOT}/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64/
  cp ${SYS_ROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64/

# Done: Make device nodes
sudo  mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo  mknod -m 666 ${OUTDIR}/rootfs/dev/console c 5 1

# Done: Clean and build the writer utility
cd "${FINDER_APP_DIR}"
make clean
make CROSS_COMPILE=aarch64-none-linux-gnu- all

# Done: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp ${FINDER_APP_DIR}/finder.sh "${OUTDIR}/rootfs/home" 
cp ${FINDER_APP_DIR}/finder-test.sh "${OUTDIR}/rootfs/home"
cp -r $(realpath ${FINDER_APP_DIR}/conf) "${OUTDIR}/rootfs/home"
cp ${FINDER_APP_DIR}/writer "${OUTDIR}/rootfs/home" 
cp ${FINDER_APP_DIR}/autorun-qemu.sh "${OUTDIR}/rootfs/home" 

# Correct the conf/assignment.txt path inside the destination file
$(sed -i 's/\.\.\/conf\/assignment.txt/conf\/assignment.txt/g' "${OUTDIR}/rootfs/home/finder-test.sh")

# Done: Chown the root directory
cd "${OUTDIR}/rootfs"
sudo chown -R root $OUTDIR/rootfs

# Done: Create initramfs.cpio.gz
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio

