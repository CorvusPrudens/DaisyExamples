# Project Name
TARGET = Spectral

# Sources
CPP_SOURCES = Spectral.cpp

# Library Locations
LIBDAISY_DIR = ../../libdaisy
# DAISYSP_DIR = ../../DaisySP
DAICSP_DIR = ../../DaisySP-LGPL

OPT = -O3

C_INCLUDES += -I../../DaisySP-LGPL/Source
LIBS += -ldaicsp
LIBDIR += -L../../DaisySP-LGPL/build

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
