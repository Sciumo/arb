# Uncomment one section, depending on your machine type and compiler options

DEBUG = 1# don't set to 0 (comment out instead!)
# SEER = 1

#********* LINUX / gcc g++ / static ********
MACH = LINUX# Linus g++  and gcc
LINUX = 1
ECGS = 1# for newer Linux-versions

#********* SUN4 / acc CC / dynamic ********

# MACH = SUN4#
# SUN4 = 1#                              # Exportable Version

# MACH = SUN5#
# SUN5 = 1
# SUN_WS_50 = 1 # compile with Workshop 5.0

#********* HP ****************
# HPCC = 1
# HPGCC = 1
# MACH = HP#

#*******  SGI  ***********
# SGI = 1
# MACH = SGI#

#*******  DIGITAL OSF  ***********
# DIGITAL = 1
# MACH = DIGITAL#
