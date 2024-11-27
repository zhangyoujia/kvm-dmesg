#!/usr/bin/env bash
#
# base.sh
#
# Copyright (C) 2024 Ray Lee <hburaylee@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

set -e

cd $(dirname $0)

if [ ! -d kvm-dmesg-ci ]; then
    git clone git@github.com:rayylee/kvm-dmesg-ci.git -b master ||
        git clone https://github.com/rayylee/kvm-dmesg-ci -b master
fi
python3 base.py

exit 0
