#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = astone

# --------------------------------------------------------------
# Project version for generating unique symbol names

HIPHOP_PROJECT_VERSION = 1

# --------------------------------------------------------------
# Enable AssemblyScript by setting DSP files location

HIPHOP_AS_DSP_PATH = dsp

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
    ASToneExamplePlugin.cpp

FILES_UI  = \
    ASToneExampleUI.cpp \
    ui/Blendish.cpp

# --------------------------------------------------------------
# Do some magic

DPF_TARGET_DIR = ../../bin
DPF_BUILD_DIR = ../../build/astone/1/2

include ../../Makefile.plugins.mk

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_OPENGL),true)
TARGETS += lv2_sep
endif

TARGETS += clap vst3 vst

all: $(TARGETS) $(HIPHOP_TARGET)

# --------------------------------------------------------------
