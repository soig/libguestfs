# libguestfs Python bindings
# Copyright (C) 2016 Red Hat Inc.
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
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# Utilities for the tests of the Python bindings.

import sys
import unittest


if sys.version_info >= (3, 0):
    int_type = int
else:
    int_type = long


def skipUnlessLibvirtHasCPointer ():
    """
    Skip the current class/method if:
      (a) libvirt cannot be imported (e.g. not installed)
      (b) connection objects don't have the c_pointer() method (too old)
    """
    try:
        import libvirt
    except:
        return unittest.skip ("could not import libvirt")
    # Check we're using the version of libvirt-python that has c_pointer() methods.
    if not "c_pointer" in dir (libvirt.open (None)):
        return unittest.skip ("libvirt-python doesn't support c_pointer()")
    return lambda func: func


def skipUnlessGuestfsBackendIs (wanted):
    """
    Skip the current class/method if the default backend of libguestfs
    is not 'wanted'.
    """
    import guestfs
    backend = guestfs.GuestFS ().get_backend ()
    # Match both "backend" and "backend:etc"
    if not (backend == wanted or backend.startswith (wanted + ":")):
        return unittest.skip ("the current backend is not %s" % wanted)
    return lambda func: func


def skipUnlessArchMatches (arch_re):
    """
    Skip the current class/method if the current architecture does not match
    the regexp specified.
    """
    import platform
    import re
    machine = platform.machine ()
    rex = re.compile (arch_re)
    if not rex.match (machine):
        return unittest.skip ("the current architecture (%s) does not match '%s'" % (machine, arch_re))
    return lambda func: func
