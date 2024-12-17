#!/usr/bin/env bash

cd $(dirname $0)

VERSION_FILE="../version.h"
ARCH=$(uname -m 2>/dev/null)

MAJOR_VERSION=$(grep -Po '#define MAJOR_VERSION \K\d+' "$VERSION_FILE")
MINOR_VERSION=$(grep -Po '#define MINOR_VERSION \K\d+' "$VERSION_FILE")
BUGFIX_VERSION=$(grep -Po '#define BUGFIX_VERSION \K\d+' "$VERSION_FILE")
EXTRA_VERSION=$(grep -Po '#define EXTRA_VERSION "\K[^"]+' "$VERSION_FILE")

VERSION_STRING="${MAJOR_VERSION}.${MINOR_VERSION}.${BUGFIX_VERSION}"
if [ -n "$EXTRA_VERSION" ]; then
    VERSION_STRING="${VERSION_STRING}.${EXTRA_VERSION}"
fi

release() {
    local pkg="$1"
    local bin="$2"

    [ -d ${pkg} ] && rm -rf ${pkg}
    [ -d ${pkg}.tar.gz ] && rm -rf ${pkg}.tar.gz

    mkdir -p ${pkg}
    cp -f ${bin} ${pkg}/
    chmod +x ${pkg}/kvm-dmesg*
    strip ${pkg}/kvm-dmesg*

    tar -czf ${pkg}.tar.gz ${pkg}
    [ -d ${pkg} ] && rm -rf ${pkg}
}

pushd ../
    make clean && make
popd
release kvm-dmesg-${VERSION_STRING}-${ARCH} ../kvm-dmesg

pushd ../
    make clean && CROSS_COMPILE=x86_64-linux-musl- STATIC=y make
popd
release kvm-dmesg-${VERSION_STRING}-${ARCH}-static ../kvm-dmesg

exit 0
