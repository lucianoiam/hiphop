# Filename: Makefile.plugins.mk
# Author:   oss@lucianoiam.com

# ------------------------------------------------------------------------------
# Configuration defaults

# Location for binaries
DPF_TARGET_DIR ?= bin
# Location for object files
DPF_BUILD_DIR ?= build
# Enable built-in websockets server and load content over HTTP
HIPHOP_NETWORK_UI ?= false
# (WIP) Enable HTTPS and secure WebSockets
HIPHOP_NETWORK_SSL ?= false
# Build a type of Variant backed by libbson
HIPHOP_SUPPORT_BSON ?= false
# Automatically inject dpf.js when loading content from file://
HIPHOP_INJECT_FRAMEWORK_JS ?= false
# Web view implementation on Linux [ gtk | cef ]
HIPHOP_LINUX_WEBVIEW ?= gtk
# WebAssembly runtime library [ wamr | wasmer ]
HIPHOP_WASM_RUNTIME ?= wamr
# WebAssembly execution mode - WAMR [ aot | interp ], Wasmer [ jit ]
HIPHOP_WASM_MODE ?= aot
# Universal build not available for Wasmer DSP
# Set to false for building current architecture only
HIPHOP_MACOS_UNIVERSAL ?= false
# Support macOS down to High Sierra when WKWebView was introduced. This setting
# must be enabled when compiling on newer systems otherwise plugins will crash.
HIPHOP_MACOS_OLD ?= false
# Build a rough and incomplete JACK application for development purposes
HIPHOP_MACOS_DEV_STANDALONE ?= false
# Target directory for dpf.js and libraries relative to plugin ui directory
HIPHOP_JS_LIB_TARGET_PATH ?=

ifeq ($(HIPHOP_PROJECT_VERSION),)
$(error HIPHOP_PROJECT_VERSION is not set)
endif

# ------------------------------------------------------------------------------
# Determine build environment

HIPHOP_ROOT_PATH := $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))
HIPHOP_INC_PATH  = $(HIPHOP_ROOT_PATH)/hiphop
HIPHOP_SRC_PATH  = $(HIPHOP_ROOT_PATH)/hiphop/src
HIPHOP_DEPS_PATH = $(HIPHOP_ROOT_PATH)/deps
DPF_PATH         = $(HIPHOP_ROOT_PATH)/dpf

ifneq ($(HIPHOP_AS_DSP_PATH),)
NPM_OPT_SET_PATH = true
WASM_DSP = true
endif

ifneq ($(HIPHOP_WEB_UI_PATH),)
WEB_UI = true
endif

TARGET_MACHINE := $(shell gcc -dumpmachine)

ifneq (,$(findstring linux,$(TARGET_MACHINE)))
LINUX = true
endif
ifneq (,$(findstring apple,$(TARGET_MACHINE)))
MACOS = true
endif
ifneq (,$(findstring mingw,$(TARGET_MACHINE)))
WINDOWS = true
endif
ifneq (,$(findstring MINGW,$(MSYSTEM)))
MSYS_MINGW = true
endif

ifeq ($(MSYS_MINGW),true)
ifeq ($(shell cmd /c "net.exe session 1>NUL 2>NUL || exit /b 1"; echo $$?),1)
# Run MSYS as administrator for real symlink support
MSYS_MINGW_SYMLINKS = :
else
MSYS_MINGW_SYMLINKS = export MSYS=winsymlinks:nativestrict
endif
endif

# ------------------------------------------------------------------------------
# Utility for determining plugin library path. Use the standard plugin resources
# path when available. User defined TARGETS variable becomes available only
# *after* inclusion of this Makefile hence the usage of the 'test' command.

TEST_LV2 = test -d $(DPF_TARGET_DIR)/$(NAME).lv2
TEST_CLAP_LINUX_OR_WINDOWS = test -f $(DPF_TARGET_DIR)/$(NAME).clap
TEST_CLAP_MACOS = test -d $(DPF_TARGET_DIR)/$(NAME).clap
TEST_VST3 = test -d $(DPF_TARGET_DIR)/$(NAME).vst3
TEST_VST2_LINUX = test -f $(DPF_TARGET_DIR)/$(NAME)-vst.so
TEST_VST2_MACOS = test -d $(DPF_TARGET_DIR)/$(NAME).vst
TEST_VST2_WINDOWS = test -f $(DPF_TARGET_DIR)/$(NAME)-vst.dll
TEST_JACK_LINUX_OR_MACOS = test -f $(DPF_TARGET_DIR)/$(NAME)
TEST_JACK_WINDOWS = test -f $(DPF_TARGET_DIR)/$(NAME).exe
TEST_NOBUNDLE = $(TEST_CLAP_LINUX_OR_WINDOWS) \
				|| $(TEST_VST2_WINDOWS) || $(TEST_VST2_LINUX) \
				|| $(TEST_JACK_LINUX_OR_MACOS) || $(TEST_JACK_WINDOWS)

LIB_DIR_LV2 = $(DPF_TARGET_DIR)/$(NAME).lv2/lib
LIB_DIR_CLAP_MACOS = $(DPF_TARGET_DIR)/$(NAME).clap/Contents/Resources
LIB_DIR_VST3 = $(DPF_TARGET_DIR)/$(NAME).vst3/Contents/Resources
LIB_DIR_VST2_MACOS = $(DPF_TARGET_DIR)/$(NAME).vst/Contents/Resources
LIB_DIR_NOBUNDLE = $(DPF_TARGET_DIR)/$(NAME)-lib

# ------------------------------------------------------------------------------
# Support some features missing from DPF like shared memory

HIPHOP_FILES_DSP = PluginEx.cpp
HIPHOP_FILES_UI  = UIEx.cpp

# ------------------------------------------------------------------------------
# Code shared by both UI and DSP
HIPHOP_FILES_SHARED += JSONVariant.cpp \
				       thirdparty/cJSON.c
ifeq ($(HIPHOP_SUPPORT_BSON),true)
HIPHOP_FILES_SHARED += BSONVariant.cpp
endif

# ------------------------------------------------------------------------------
# Optional support for web UI

ifeq ($(WEB_UI),true)
HIPHOP_FILES_UI += WebUIBase.cpp \
				   WebViewBase.cpp \
				   WebViewUI.cpp
ifeq ($(HIPHOP_NETWORK_UI),true)
HIPHOP_FILES_UI += NetworkUI.cpp \
				   WebServer.cpp
endif
ifeq ($(LINUX),true)
HIPHOP_FILES_UI += linux/LinuxWebViewUI.cpp \
				   linux/ChildProcessWebView.cpp \
				   linux/IpcChannel.cpp \
				   linux/ipc.c \
				   linux/scaling.c
endif
ifeq ($(MACOS),true)
HIPHOP_FILES_UI += macos/MacWebViewUI.mm \
				   macos/CocoaWebView.mm
endif
ifeq ($(WINDOWS),true)
HIPHOP_FILES_UI += windows/WindowsWebViewUI.cpp \
				   windows/EdgeWebView.cpp \
				   windows/WebView2EventHandler.cpp
endif
endif

FILES_UI += $(HIPHOP_FILES_UI:%=$(HIPHOP_SRC_PATH)/ui/%)
FILES_UI += $(HIPHOP_FILES_SHARED:%=$(HIPHOP_SRC_PATH)/%)

# ------------------------------------------------------------------------------
# Optional support for AssemblyScript DSP

ifeq ($(WASM_DSP),true)
HIPHOP_FILES_DSP += WasmPluginImpl.cpp \
					WasmRuntime.cpp
endif

FILES_DSP += $(HIPHOP_FILES_DSP:%=$(HIPHOP_SRC_PATH)/dsp/%)
FILES_DSP += $(HIPHOP_FILES_SHARED:%=$(HIPHOP_SRC_PATH)/%)

# ------------------------------------------------------------------------------
# Optional support for macOS universal binaries, keep this before DPF include.

ifeq ($(MACOS),true)
ifeq ($(HIPHOP_MACOS_UNIVERSAL),true)
ifeq ($(WASM_DSP),true)
  ifeq ($(HIPHOP_WASM_RUNTIME),wamr)
  $(error Universal build is currently unavailable for WAMR)
  endif
endif
# Non CPU-specific optimization flags, see DPF Makefile.base.mk
NOOPT = true
MACOS_UNIVERSAL_FLAGS = -arch x86_64 -arch arm64
CFLAGS += $(MACOS_UNIVERSAL_FLAGS)
CXXFLAGS += $(MACOS_UNIVERSAL_FLAGS)
LDFLAGS += $(MACOS_UNIVERSAL_FLAGS)
endif
endif

# ------------------------------------------------------------------------------
# Include DPF Makefile for plugins

# These commands cannot belong to a target because this Makefile relies on DPF's
ifeq (,$(wildcard $(DPF_PATH)/Makefile))
_ := $(shell git submodule update --init --recursive)
endif

ifeq ($(WEB_UI),true)
UI_TYPE = external
endif

include $(DPF_PATH)/Makefile.plugins.mk

# ------------------------------------------------------------------------------
# Add shared build flags

BASE_FLAGS += -I$(HIPHOP_INC_PATH) -I$(HIPHOP_SRC_PATH) -I$(DPF_PATH) \
			  -DHIPHOP_PLUGIN_BIN_BASENAME=$(NAME) \
			  -DHIPHOP_PROJECT_ID_HASH=$(shell echo $(NAME):$(HIPHOP_PROJECT_VERSION) \
				 | shasum -a 256 | head -c 8)
ifeq ($(LINUX),true)
BASE_FLAGS += -lrt
endif
ifeq ($(MACOS),true)
# Mute lots of warnings from DPF: 'vfork' is deprecated: Use posix_spawn or fork
BASE_FLAGS += -Wno-deprecated-declarations
ifeq ($(HIPHOP_MACOS_OLD),true)
# Warning: ...was built for newer macOS version (11.0) than being linked (10.13)
BASE_FLAGS += -mmacosx-version-min=10.13
endif
endif

# ------------------------------------------------------------------------------
# Add build flags for web UI dependencies

ifeq ($(WEB_UI),true)
  ifeq ($(HIPHOP_NETWORK_UI),true)
	BASE_FLAGS += -DHIPHOP_NETWORK_UI -I$(LWS_PATH)/include -I$(LWS_BUILD_PATH)
	LINK_FLAGS += -L$(LWS_BUILD_PATH)/lib
	ifeq ($(HIPHOP_INJECT_FRAMEWORK_JS),true)
	$(warning Network UI is enabled - disabling JavaScript framework injection)
	HIPHOP_INJECT_FRAMEWORK_JS = false
	endif
	ifeq ($(HIPHOP_NETWORK_SSL), true)
	BASE_FLAGS += -I$(MBEDTLS_PATH)/include -DHIPHOP_NETWORK_SSL
	LINK_FLAGS += -L$(MBEDTLS_BUILD_PATH) -lmbedtls -lmbedcrypto -lmbedx509
	endif
	ifeq ($(WINDOWS),true)
	LINK_FLAGS += -lwebsockets_static -lWs2_32
	else
	LINK_FLAGS += -lwebsockets
	endif
	ifeq ($(LINUX),true)
	# Add -lcap after -lwebsockets
	LINK_FLAGS += -lcap
	endif
	ifeq ($(MACOS),true)
	# Some lws-http.h constants clash with MacOSX.sdk/usr/include/cups/http.h
	BASE_FLAGS += -D_CUPS_CUPS_H_ -D_CUPS_HTTP_H_ -D_CUPS_PPD_H_
	endif
  endif
  ifeq ($(HIPHOP_INJECT_FRAMEWORK_JS),true)
  BASE_FLAGS += -DHIPHOP_INJECT_FRAMEWORK_JS
  endif
  ifeq ($(HIPHOP_SUPPORT_BSON), true)
  BASE_FLAGS += -DHIPHOP_SUPPORT_BSON -I$(LIBBSON_PATH)/src/libbson/src \
				-I$(LIBBSON_PATH)/build/src/libbson/src
  LINK_FLAGS += -L$(LIBBSON_BUILD_PATH)/src/libbson -lbson-static-1.0
  ifeq ($(WINDOWS),true)
	LINK_FLAGS += -lWs2_32
	endif
  endif
  ifeq ($(HIPHOP_PRINT_TRAFFIC),true)
  BASE_FLAGS += -DHIPHOP_PRINT_TRAFFIC
  endif
  ifeq ($(LINUX),true)
  LINK_FLAGS += -lpthread -ldl
  endif
  ifeq ($(MACOS),true)
  LINK_FLAGS += -framework WebKit 
  endif
  ifeq ($(WINDOWS),true)
  BASE_FLAGS += -I$(EDGE_WEBVIEW2_PATH)/build/native/include -Wno-unknown-pragmas
  LINK_FLAGS += -L$(EDGE_WEBVIEW2_PATH)/build/native/x64 \
				-lole32 -lShlwapi -lMfplat -lksuser -lmfuuid -lwmcodecdspuuid \
				-static-libgcc -static-libstdc++ -Wl,-Bstatic \
				-lstdc++ -lpthread
  endif
endif

# ------------------------------------------------------------------------------
# Add build flags for AssemblyScript DSP dependencies

ifeq ($(WASM_DSP),true)
  BASE_FLAGS += -DHIPHOP_SUPPORT_WASM
  WASM_BYTECODE_FILE = optimized.wasm
  ifeq ($(HIPHOP_WASM_RUNTIME),wamr)
	BASE_FLAGS += -DHIPHOP_WASM_RUNTIME_WAMR
	ifeq ($(HIPHOP_WASM_MODE),aot)
	  ifeq ($(CPU_I386_OR_X86_64),true)
	  WAMRC_TARGET = x86_64
	  endif
	  ifeq ($(CPU_ARM_OR_AARCH64),true)
	  WAMRC_TARGET = aarch64
	  endif
	  WASM_BINARY_FILE = $(WAMRC_TARGET).aot
	  BASE_FLAGS += -DHIPHOP_WASM_BINARY_COMPILED
	endif
	ifeq ($(HIPHOP_WASM_MODE),interp)
	WASM_BINARY_FILE = $(WASM_BYTECODE_FILE)
	endif
	ifeq ($(HIPHOP_WASM_MODE),jit)
	$(error JIT mode is not supported for WAMR)
	endif
  endif
  ifeq ($(HIPHOP_WASM_RUNTIME),wasmer)
	BASE_FLAGS += -DHIPHOP_WASM_RUNTIME_WASMER
	ifeq ($(HIPHOP_WASM_MODE),interp)
	# Both WAMR interp and Wasmer will load bytecode
	HIPHOP_WASM_MODE = jit
	endif
	ifeq ($(HIPHOP_WASM_MODE),jit)
	WASM_BINARY_FILE = $(WASM_BYTECODE_FILE)
	else
	$(error Only JIT mode is supported for Wasmer)
	endif
  endif
  ifeq ($(HIPHOP_WASM_RUNTIME),wamr)
	BASE_FLAGS += -I$(WAMR_PATH)/core/iwasm/include
	ifeq ($(LINUX_OR_MACOS),true)
	LINK_FLAGS += -L$(WAMR_BUILD_PATH) -lvmlib
	endif
	ifeq ($(LINUX),true)
	LINK_FLAGS += -lpthread
	endif
	ifeq ($(WINDOWS),true)
	  ifeq ($(HIPHOP_WASM_MODE),interp)
	  LINK_FLAGS += -L$(WAMR_BUILD_PATH) -lvmlib
	  LINK_FLAGS += -lWs2_32 -lShlwapi
	  endif
	endif
  endif
  ifeq ($(HIPHOP_WASM_RUNTIME),wasmer)
	BASE_FLAGS += -I$(WASMER_PATH)/include
	LINK_FLAGS += -L$(WASMER_PATH)/lib -lwasmer
	ifeq ($(LINUX),true)
	LINK_FLAGS += -lpthread -ldl
	endif
	ifeq ($(MACOS),true)
	LINK_FLAGS += -framework AppKit 
	endif
	ifeq ($(WINDOWS),true)
	LINK_FLAGS += -Wl,-Bstatic -lWs2_32 -lBcrypt -lUserenv -lShlwapi
	endif
  endif
endif

# ------------------------------------------------------------------------------
# Print some info

TARGETS += info

info:
	@echo "Building $(NAME)"

# ------------------------------------------------------------------------------
# Development - JACK-based application

ifeq ($(HIPHOP_MACOS_DEV_STANDALONE),true)
ifeq ($(MACOS),true)
ifeq ($(HAVE_JACK),true)
ifeq ($(HAVE_OPENGL),true)
TARGETS += jack
endif
endif
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Build DPF Graphics Library

ifneq ($(WEB_UI),true)
  LIBDGL_PATH = $(DPF_PATH)/build/libdgl.a
  TARGETS += $(LIBDGL_PATH)
  ifeq ($(HIPHOP_MACOS_UNIVERSAL),true)
  DGL_MAKE_FLAGS = dgl NOOPT=true DGL_FLAGS="$(MACOS_UNIVERSAL_FLAGS)" \
				 DGL_LIBS="$(MACOS_UNIVERSAL_FLAGS)"
  endif

  $(LIBDGL_PATH):
		make -C $(DPF_PATH) $(DGL_MAKE_FLAGS)
endif

# ------------------------------------------------------------------------------
# Dependency - Clone and build Mbed TLS

ifeq ($(WEB_UI),true)
ifeq ($(HIPHOP_NETWORK_UI),true)
ifeq ($(HIPHOP_NETWORK_SSL), true)
MBEDTLS_GIT_URL = https://github.com/ARMmbed/mbedtls
MBEDTLS_GIT_TAG = v3.5.2
MBEDTLS_PATH = $(HIPHOP_DEPS_PATH)/mbedtls
MBEDTLS_BUILD_PATH = ${MBEDTLS_PATH}/library
MBEDTLS_LIB_PATH = $(MBEDTLS_BUILD_PATH)/libmbedtls.a

ifeq ($(SKIP_STRIPPING),true)
MBEDTLS_MAKE_ARGS = DEBUG=1
endif

TARGETS += $(MBEDTLS_LIB_PATH)

$(MBEDTLS_LIB_PATH): $(MBEDTLS_PATH)
	@echo "Building Mbed TLS static library"
	@mkdir -p $(MBEDTLS_BUILD_PATH) && cd $(MBEDTLS_BUILD_PATH) && make

$(MBEDTLS_PATH):
	@mkdir -p $(HIPHOP_DEPS_PATH)
	@git -C $(HIPHOP_DEPS_PATH) clone --depth 1 --branch $(MBEDTLS_GIT_TAG) \
			$(MBEDTLS_GIT_URL)
endif
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Clone and build libwebsockets

ifeq ($(WEB_UI),true)
ifeq ($(HIPHOP_NETWORK_UI),true)
LWS_GIT_URL = https://github.com/warmcat/libwebsockets
LWS_GIT_TAG = v4.3.3
LWS_PATH = $(HIPHOP_DEPS_PATH)/libwebsockets
LWS_BUILD_PATH = ${LWS_PATH}/build
LWS_LIB_PATH = $(LWS_BUILD_PATH)/lib/libwebsockets.a

LWS_CMAKE_ARGS = -DLWS_WITH_SHARED=0 -DLWS_WITHOUT_TESTAPPS=1
ifeq ($(HIPHOP_NETWORK_SSL),true)
LWS_CMAKE_ARGS += -DLWS_WITH_SSL=1 -DLWS_WITH_MBEDTLS=1 \
				  -DLWS_MBEDTLS_INCLUDE_DIRS=../../mbedtls/include
else
LWS_CMAKE_ARGS += -DLWS_WITH_SSL=0
endif

ifeq ($(WINDOWS),true)
LWS_LIB_PATH = $(LWS_BUILD_PATH)/lib/libwebsockets_static.a
LWS_CMAKE_ARGS += -G"MSYS Makefiles"
endif

TARGETS += $(LWS_LIB_PATH)

LWS_CMAKE_ENV = true
ifeq ($(LINUX),true)
LWS_CMAKE_ENV = export CFLAGS=-fPIC
endif
ifeq ($(MACOS),true)
ifeq ($(HIPHOP_MACOS_UNIVERSAL),true)
LWS_CMAKE_ENV = export CMAKE_OSX_ARCHITECTURES="arm64;x86_64;"
endif
endif

$(LWS_LIB_PATH): $(LWS_PATH)
	@echo "Building libwebsockets static library"
	@mkdir -p $(LWS_BUILD_PATH) && cd $(LWS_BUILD_PATH) && $(LWS_CMAKE_ENV) \
		&& cmake .. $(LWS_CMAKE_ARGS) && cmake --build .

$(LWS_PATH):
	@mkdir -p $(HIPHOP_DEPS_PATH)
	@git -C $(HIPHOP_DEPS_PATH) clone --depth 1 --branch $(LWS_GIT_TAG) $(LWS_GIT_URL)
endif
endif

# ------------------------------------
# Dependency - Clone and build libbson

ifeq ($(HIPHOP_SUPPORT_BSON), true)
LIBBSON_GIT_URL = https://github.com/mongodb/mongo-c-driver
LIBBSON_GIT_TAG = 1.26.0
LIBBSON_PATH = $(HIPHOP_DEPS_PATH)/mongo-c-driver
LIBBSON_BUILD_PATH = ${LIBBSON_PATH}/build
LIBBSON_LIB_PATH = $(LIBBSON_BUILD_PATH)/src/libbson/libbson-static-1.0.a
LIBBSON_CMAKE_ARGS = -DENABLE_MONGOC=OFF -DENABLE_TESTS=OFF -DENABLE_EXAMPLES=OFF \
					 -DENABLE_BSON=ON -DENABLE_STATIC=ON -DCMAKE_BUILD_TYPE=Release

TARGETS += $(LIBBSON_LIB_PATH)

LIBBSON_CMAKE_ENV = true
ifeq ($(MACOS),true)
ifeq ($(HIPHOP_MACOS_UNIVERSAL),true)
LIBBSON_CMAKE_ENV = export CMAKE_OSX_ARCHITECTURES="arm64;x86_64;"
endif
endif
ifeq ($(WINDOWS),true)
# build/calc_release_version.py fails on MSYS due to git returning error
# "fatal: Needed a single revision". Pass version to CMake.
LIBBSON_CMAKE_ARGS += -G"MSYS Makefiles" -DBUILD_VERSION=$(LIBBSON_GIT_TAG)
# Prevent "error: implicit declaration of function 'aligned_alloc' [-Werror=implicit-function-declaration]"
LIBBSON_CMAKE_ENV = export CFLAGS=-std=c99
endif

$(LIBBSON_LIB_PATH): $(LIBBSON_PATH)
	@echo "Building libbson static library"
	@mkdir -p $(LIBBSON_BUILD_PATH) && cd $(LIBBSON_BUILD_PATH) && $(LIBBSON_CMAKE_ENV) \
		&& cmake .. $(LIBBSON_CMAKE_ARGS) && cmake --build .

$(LIBBSON_PATH):
	@mkdir -p $(HIPHOP_DEPS_PATH)
	@git -C $(HIPHOP_DEPS_PATH) clone --depth 1 --branch $(LIBBSON_GIT_TAG) $(LIBBSON_GIT_URL)
endif

# ------------------------------------------------------------------------------
# Dependency - Built-in JavaScript library include and polyfills

ifeq ($(WEB_UI),true)
ifeq ($(HIPHOP_INJECT_FRAMEWORK_JS),true)
FRAMEWORK_JS_PATH = $(HIPHOP_SRC_PATH)/ui/dpf.js
DPF_JS_INCLUDE_PATH = $(FRAMEWORK_JS_PATH).inc

TARGETS += $(DPF_JS_INCLUDE_PATH)

$(DPF_JS_INCLUDE_PATH): $(FRAMEWORK_JS_PATH)
	@echo 'R"JS(' > $(DPF_JS_INCLUDE_PATH)
	@cat $(FRAMEWORK_JS_PATH) >> $(DPF_JS_INCLUDE_PATH)
	@echo ')JS"' >> $(DPF_JS_INCLUDE_PATH)
endif

ifeq ($(MACOS),true)
POLYFILL_JS_PATH = $(HIPHOP_SRC_PATH)/ui/macos/polyfill.js
POLYFILL_JS_INCLUDE_PATH = $(POLYFILL_JS_PATH).inc

TARGETS += $(POLYFILL_JS_INCLUDE_PATH)

$(POLYFILL_JS_INCLUDE_PATH): $(POLYFILL_JS_PATH)
	@echo 'R"JS(' > $(POLYFILL_JS_INCLUDE_PATH)
	@cat $(POLYFILL_JS_PATH) >> $(POLYFILL_JS_INCLUDE_PATH)
	@echo ')JS"' >> $(POLYFILL_JS_INCLUDE_PATH)
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Download Edge WebView2

ifeq ($(WEB_UI),true)
ifeq ($(WINDOWS),true)
ifeq ($(MSYS_MINGW),true)
NUGET_URL = https://dist.nuget.org/win-x86-commandline/latest/nuget.exe
NUGET_BIN = /usr/bin/nuget.exe

TARGETS += $(NUGET_BIN)

$(NUGET_BIN):
	@echo Downloading NuGet
	@wget -4 -P /usr/bin $(NUGET_URL)
else
ifeq (,$(shell which nuget 2>/dev/null))
$(error NuGet not found, try sudo apt install nuget or the equivalent for your distro)
endif
endif

EDGE_WEBVIEW2_PATH = $(HIPHOP_DEPS_PATH)/Microsoft.Web.WebView2

TARGETS += $(EDGE_WEBVIEW2_PATH)

$(EDGE_WEBVIEW2_PATH):
	@echo Downloading Edge WebView2 SDK
	@mkdir -p $(HIPHOP_DEPS_PATH)
	@eval $(MSYS_MINGW_SYMLINKS)
	@nuget install Microsoft.Web.WebView2 -OutputDirectory $(HIPHOP_DEPS_PATH)
	@ln -rs $(EDGE_WEBVIEW2_PATH).* $(EDGE_WEBVIEW2_PATH)
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Clone and build WAMR

ifeq ($(WASM_DSP),true)
ifeq ($(HIPHOP_WASM_RUNTIME),wamr)
WAMR_GIT_URL = https://github.com/bytecodealliance/wasm-micro-runtime
WAMR_GIT_TAG = WAMR-1.3.2
WAMR_PATH = $(HIPHOP_DEPS_PATH)/wasm-micro-runtime
WAMR_BUILD_PATH = ${WAMR_PATH}/build-$(HIPHOP_WASM_MODE)
WAMR_LIB_PATH = $(WAMR_BUILD_PATH)/libvmlib.a
WAMR_LLVM_LIB_PATH = ${WAMR_PATH}/core/deps/llvm/build/lib/libLLVMCore.a
WAMRC_PATH = $(WAMR_PATH)/wamr-compiler
WAMRC_BUILD_PATH = $(WAMRC_PATH)/build
WAMRC_BIN_PATH = $(WAMRC_PATH)/build/wamrc

# Disable WASI feature because it is not exposed by the WAMR C API.
# HW_BOUND_CHECK not compiling on MinGW, leads to muted plugins on Linux+AOT and
# crashing on Mac+AOT https://github.com/bytecodealliance/wasm-micro-runtime/pull/1001
WAMR_CMAKE_ARGS = -DWAMR_BUILD_LIBC_WASI=0 -DWAMR_DISABLE_HW_BOUND_CHECK=1
ifeq ($(HIPHOP_WASM_MODE),aot)
WAMR_CMAKE_ARGS += -DWAMR_BUILD_AOT=1 -DWAMR_BUILD_INTERP=0
endif
ifeq ($(HIPHOP_WASM_MODE),interp)
WAMR_CMAKE_ARGS += -DWAMR_BUILD_AOT=0 -DWAMR_BUILD_INTERP=1
endif

ifeq ($(WINDOWS),true)
# Use the C version of invokeNative() instead of ASM until MinGW build is fixed.
WAMR_LLVM_LIB_PATH = ${WAMR_PATH}/core/deps/llvm/win32build/lib/libLLVMCore.a
WAMR_CMAKE_ARGS += -G"Unix Makefiles" -DWAMR_BUILD_INVOKE_NATIVE_GENERAL=1
WAMRC_BIN_PATH = $(WAMRC_PATH)/build/wamrc.exe
WAMRC_CMAKE_ARGS += -G"Unix Makefiles" -DWAMR_BUILD_INVOKE_NATIVE_GENERAL=1
endif

ifeq ($(SKIP_STRIPPING),true)
WAMR_BUILD_CONFIG = Debug
else
WAMR_BUILD_CONFIG = Release
endif

ifeq ($(LINUX_OR_MACOS),true)
TARGETS += $(WAMR_LIB_PATH)
endif
ifeq ($(WINDOWS),true)
ifeq ($(HIPHOP_WASM_MODE),interp)
# On Windows the WAMR static lib is only compiled for interp mode.
TARGETS += $(WAMR_LIB_PATH)
endif
ifeq ($(HIPHOP_WASM_MODE),aot)
# For AOT a MSVC DLL is used because the MinGW static lib is crashing.
TARGETS += $(WAMR_DLL_PATH)
endif
endif

ifeq ($(HIPHOP_WASM_MODE),aot)
TARGETS += $(WAMRC_BIN_PATH)
endif

WAMR_REPO = $(WAMR_PATH)/README.md

$(WAMR_LIB_PATH): $(WAMR_REPO)
	@echo "Building WAMR static library"
	@mkdir -p $(WAMR_BUILD_PATH) && cd $(WAMR_BUILD_PATH) \
		&& cmake .. $(WAMR_CMAKE_ARGS) && cmake --build . --config $(WAMR_BUILD_CONFIG)

$(WAMRC_BIN_PATH): $(WAMR_LLVM_LIB_PATH)
	@echo "Buliding WAMR compiler"
	@mkdir -p $(WAMRC_BUILD_PATH) && cd $(WAMRC_BUILD_PATH) \
		&& cmake .. $(WAMRC_CMAKE_ARGS) && cmake --build .

$(WAMR_LLVM_LIB_PATH): $(WAMR_REPO)
	@echo "Building LLVM"
	@$(WAMR_PATH)/build-scripts/build_llvm.py

$(WAMR_REPO):
	@mkdir -p $(HIPHOP_DEPS_PATH)
	@git -C $(HIPHOP_DEPS_PATH) clone $(WAMR_GIT_URL) --branch $(WAMR_GIT_TAG) --depth 1
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Download Wasmer static library

ifeq ($(WASM_DSP),true)
ifeq ($(HIPHOP_WASM_RUNTIME),wasmer)
WASMER_URL = https://github.com/wasmerio/wasmer/releases/download
WASMER_VERSION = 2.1.1
WASMER_PATH = $(HIPHOP_DEPS_PATH)/wasmer

TARGETS += $(WASMER_PATH)

ifeq ($(LINUX_OR_MACOS),true)
ifeq ($(LINUX),true)
WASMER_PKG_FILE_1 = wasmer-linux-amd64.tar.gz
endif
ifeq ($(MACOS),true)
# There is no macOS universal binary of Wasmer, download both architectures and combine.
WASMER_PKG_FILE_ARM = wasmer-darwin-arm64.tar.gz
WASMER_PKG_FILE_INTEL = wasmer-darwin-amd64.tar.gz
ifeq ($(CPU_ARM_OR_AARCH64),true)
WASMER_PKG_FILE_1 = $(WASMER_PKG_FILE_ARM)
WASMER_PKG_FILE_2 = $(WASMER_PKG_FILE_INTEL)
else
WASMER_PKG_FILE_1 = $(WASMER_PKG_FILE_INTEL)
WASMER_PKG_FILE_2 = $(WASMER_PKG_FILE_ARM)
endif
endif
WASMER_PKG_URL_1 = $(WASMER_URL)/$(WASMER_VERSION)/$(WASMER_PKG_FILE_1)
WASMER_PKG_URL_2 = $(WASMER_URL)/$(WASMER_VERSION)/$(WASMER_PKG_FILE_2)
endif
ifeq ($(WINDOWS),true)
# Wasmer official Windows binary distribution requires MSVC, download a custom build for MinGW.
WASMER_PKG_FILE_1 = wasmer-mingw-amd64-$(WASMER_VERSION).tar.gz
WASMER_PKG_URL_1 = https://github.com/lucianoiam/hiphop/files/7796845/$(WASMER_PKG_FILE_1)
endif

# https://stackoverflow.com/questions/37038472/osx-how-to-statically-link-a-library-and-dynamically-link-the-standard-library
$(WASMER_PATH):
	@mkdir -p $(WASMER_PATH)
	@wget -4 -O /tmp/$(WASMER_PKG_FILE_1) $(WASMER_PKG_URL_1)
	@tar xzf /tmp/$(WASMER_PKG_FILE_1) -C $(WASMER_PATH)
ifeq ($(LINUX),true)
	@mv $(WASMER_PATH)/lib/libwasmer.so $(WASMER_PATH)/lib/libwasmer.so.ignore
endif
ifeq ($(MACOS),true)
	@mv $(WASMER_PATH)/lib/libwasmer.dylib $(WASMER_PATH)/lib/libwasmer.dylib.ignore
endif
	@rm /tmp/$(WASMER_PKG_FILE_1)
ifeq ($(HIPHOP_MACOS_UNIVERSAL),true)
	@wget -4 -O /tmp/$(WASMER_PKG_FILE_2) $(WASMER_PKG_URL_2)
	@mkdir -p /tmp/wasmer-2
	@tar xzf /tmp/$(WASMER_PKG_FILE_2) -C /tmp/wasmer-2
	@lipo -create $(WASMER_PATH)/lib/libwasmer.a /tmp/wasmer-2/lib/libwasmer.a \
		-o $(WASMER_PATH)/lib/libwasmer-universal.a
	@rm $(WASMER_PATH)/lib/libwasmer.a
	@mv $(WASMER_PATH)/lib/libwasmer-universal.a $(WASMER_PATH)/lib/libwasmer.a
	@rm /tmp/$(WASMER_PKG_FILE_2)
	@rm -rf /tmp/wasmer-2
else
endif
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Download Node.js for MinGW

ifeq ($(WASM_DSP),true)
ifeq ($(MSYS_MINGW),true)
NPM_OPT_SET_PATH = export PATH=$$PATH:/opt/node && export NODE_SKIP_PLATFORM_CHECK=1
ifeq (,$(wildcard /opt/node))
NPM_VERSION = 19.2.0
NPM_FILENAME = node-v$(NPM_VERSION)-win-x64.zip
NPM_URL = https://nodejs.org/dist/v$(NPM_VERSION)/$(NPM_FILENAME)
NPM_BIN = /opt/node/npm

TARGETS += $(NPM_BIN)

$(NPM_BIN):
	@echo Downloading Node.js
	@wget -4 -P /tmp $(NPM_URL)
	@unzip -o /tmp/$(NPM_FILENAME) -d /opt
	@mv /opt/$(basename $(NPM_FILENAME)) /opt/node
	@rm /tmp/$(NPM_FILENAME)
endif
endif
endif

# ------------------------------------------------------------------------------
# Dependency - Download MSVC WAMR DLL for Windows

ifeq ($(WASM_DSP),true)
ifeq ($(WINDOWS),true)
ifeq ($(HIPHOP_WASM_RUNTIME),wamr)
ifeq ($(HIPHOP_WASM_MODE),aot)
WAMR_DLL_FILE = libiwasm.dll
WAMR_DLL_URL = https://github.com/lucianoiam/hiphop/files/8346015/$(WAMR_DLL_FILE).zip
WAMR_DLL_PATH = $(HIPHOP_DEPS_PATH)/$(WAMR_DLL_FILE)

BASE_FLAGS += -DHIPHOP_WASM_DLL=$(WAMR_DLL_FILE)

$(WAMR_DLL_PATH):
	@echo Downloading libiwasm.dll
	@wget -4 -P /tmp $(WAMR_DLL_URL)
	@unzip -o /tmp/$(WAMR_DLL_FILE).zip -d $(HIPHOP_DEPS_PATH)
	@rm /tmp/$(WAMR_DLL_FILE).zip
endif
endif
endif
endif

# ------------------------------------------------------------------------------
# Linux only - Build webview helper binary

ifeq ($(WEB_UI),true)
ifeq ($(LINUX),true)

ifeq ($(HIPHOP_LINUX_WEBVIEW),gtk)
BASE_FLAGS += -DHIPHOP_LINUX_WEBVIEW_GTK
endif
ifeq ($(HIPHOP_LINUX_WEBVIEW),cef)
BASE_FLAGS += -DHIPHOP_LINUX_WEBVIEW_CEF
endif

# See Makefile.cef.mk and Makefile.gtk.mk
HIPHOP_TARGET += lxhelper_bin

LXHELPER_NAME = ui-helper
LXHELPER_BUILD_PATH = $(BUILD_DIR)/helper

include $(HIPHOP_SRC_PATH)/ui/linux/Makefile.$(HIPHOP_LINUX_WEBVIEW).mk
endif
endif

# ------------------------------------------------------------------------------
# Mac only - Build Objective-C++ files

ifeq ($(MACOS),true)
$(BUILD_DIR)/%.mm.o: %.mm
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	@$(CXX) $< $(BUILD_CXX_FLAGS) -ObjC++ -c -o $@
endif

# ------------------------------------------------------------------------------
# Windows only - Build resource files

ifeq ($(WINDOWS),true)
$(BUILD_DIR)/%.rc.o: %.rc
	-@mkdir -p "$(shell dirname $(BUILD_DIR)/$<)"
	@echo "Compiling $<"
	@windres --input $< --output $@ --output-format=coff
endif

# ------------------------------------------------------------------------------
# Post build - Copy Linux helper files

ifeq ($(WEB_UI),true)
ifeq ($(LINUX),true)
HIPHOP_TARGET += lxhelper_res

lxhelper_res:
	@echo "Copying UI helper files"
	@($(TEST_LV2) \
		&& mkdir -p $(LIB_DIR_LV2) \
		&& cp -ru $(LXHELPER_FILES) $(LIB_DIR_LV2) \
		) || true
	@($(TEST_VST3) \
		&& mkdir -p $(LIB_DIR_VST3) \
		&& cp -ru $(LXHELPER_FILES) $(LIB_DIR_VST3) \
		) || true
	@($(TEST_NOBUNDLE) \
		&& mkdir -p $(LIB_DIR_NOBUNDLE) \
		&& cp -ru $(LXHELPER_FILES) $(LIB_DIR_NOBUNDLE) \
		) || true
endif
endif

# ------------------------------------------------------------------------------
# Post build - Always copy web UI files

ifeq ($(WEB_UI),true)
HIPHOP_TARGET += lib_ui_plugin
LIB_UI_DIR = ui
ifneq ($(HIPHOP_INJECT_FRAMEWORK_JS),true)
HIPHOP_TARGET += lib_ui_framework
LIB_JS_PATH = $(LIB_UI_DIR)/$(HIPHOP_JS_LIB_TARGET_PATH)
LIB_JS_FILES = $(HIPHOP_SRC_PATH)/ui/dpf.js
ifeq ($(HIPHOP_SUPPORT_BSON),true)
LIB_JS_FILES += $(HIPHOP_SRC_PATH)/thirdparty/bson.min.js
endif
endif
# https://unix.stackexchange.com/questions/178235/how-is-cp-f-different-from-cp-remove-destination
CP_JS_ARGS = -f
ifeq ($(LINUX),true)
CP_JS_ARGS += --remove-destination
endif

lib_ui_plugin:
	@echo "Copying plugin web UI files"
	@($(TEST_LV2) \
		&& mkdir -p $(LIB_DIR_LV2)/$(LIB_UI_DIR) \
		&& cp -r $(HIPHOP_WEB_UI_PATH)/* $(LIB_DIR_LV2)/$(LIB_UI_DIR) \
		) || true
	@($(TEST_CLAP_MACOS) \
		&& mkdir -p $(LIB_DIR_CLAP_MACOS)/$(LIB_UI_DIR) \
		&& cp -r $(HIPHOP_WEB_UI_PATH)/* $(LIB_DIR_CLAP_MACOS)/$(LIB_UI_DIR) \
		) || true
	@($(TEST_VST3) \
		&& mkdir -p $(LIB_DIR_VST3)/$(LIB_UI_DIR) \
		&& cp -r $(HIPHOP_WEB_UI_PATH)/* $(LIB_DIR_VST3)/$(LIB_UI_DIR) \
		) || true
	@($(TEST_VST2_MACOS) \
		&& mkdir -p $(LIB_DIR_VST2_MACOS)/$(LIB_UI_DIR) \
		&& cp -r $(HIPHOP_WEB_UI_PATH)/* $(LIB_DIR_VST2_MACOS)/$(LIB_UI_DIR) \
		) || true
	@($(TEST_NOBUNDLE) \
		&& mkdir -p $(LIB_DIR_NOBUNDLE)/$(LIB_UI_DIR) \
		&& cp -r $(HIPHOP_WEB_UI_PATH)/* $(LIB_DIR_NOBUNDLE)/$(LIB_UI_DIR) \
		) || true

lib_ui_framework:
	@echo "Copying framework web UI files"
	@for LIB_JS_FILE in $(LIB_JS_FILES) ; do \
		($(TEST_LV2) \
			&& cp $(CP_JS_ARGS) $$LIB_JS_FILE $(LIB_DIR_LV2)/$(LIB_JS_PATH) \
			) || true ; \
		($(TEST_CLAP_MACOS) \
			&& cp $(CP_JS_ARGS) $$LIB_JS_FILE $(LIB_DIR_CLAP_MACOS)/$(LIB_JS_PATH) \
			) || true ; \
		($(TEST_VST3) \
			&& cp $(CP_JS_ARGS) $$LIB_JS_FILE $(LIB_DIR_VST3)/$(LIB_JS_PATH) \
			) || true ; \
		($(TEST_VST2_MACOS) \
			&& cp $(CP_JS_ARGS) $$LIB_JS_FILE $(LIB_DIR_VST2_MACOS)/$(LIB_JS_PATH) \
			) || true ; \
		($(TEST_NOBUNDLE) \
			&& cp $(CP_JS_ARGS) $$LIB_JS_FILE $(LIB_DIR_NOBUNDLE)/$(LIB_JS_PATH) \
			) || true ; \
	done

clean: clean_lib

clean_lib:
	@rm -rf $(LIB_DIR_NOBUNDLE)
endif

# ------------------------------------------------------------------------------
# Post build - Copy Windows Edge WebView2 DLL, currently only 64-bit is supported

ifeq ($(WEB_UI),true)
ifeq ($(WINDOWS),true)
HIPHOP_TARGET += edge_dll
WEBVIEW_DLL = $(EDGE_WEBVIEW2_PATH)/runtimes/win-x64/native/WebView2Loader.dll

edge_dll:
	@($(TEST_LV2) \
		&& mkdir -p $(LIB_DIR_LV2) \
		&& cp $(WEBVIEW_DLL) $(LIB_DIR_LV2) \
		) || true
	@($(TEST_VST3) \
		&& mkdir -p $(LIB_DIR_VST3) \
		&& cp $(WEBVIEW_DLL) $(LIB_DIR_VST3) \
		) || true
	@($(TEST_NOBUNDLE) \
		&& mkdir -p $(LIB_DIR_NOBUNDLE) \
		&& cp $(WEBVIEW_DLL) $(LIB_DIR_NOBUNDLE) \
		) || true
endif
endif

# ------------------------------------------------------------------------------
# Post build - Compile AssemblyScript project

ifneq ($(WASM_DSP),)
ifneq ($(HIPHOP_AS_SKIP_FRAMEWORK_FILES),true)
HIPHOP_TARGET += framework_as

AS_ASSEMBLY_PATH = $(HIPHOP_AS_DSP_PATH)/assembly

framework_as:
	@test -f $(AS_ASSEMBLY_PATH)/index.ts \
		|| ln -s $(abspath $(HIPHOP_SRC_PATH)/dsp/index.ts) $(AS_ASSEMBLY_PATH)
	@test -f $(AS_ASSEMBLY_PATH)/dpf.ts \
		|| ln -s $(abspath $(HIPHOP_SRC_PATH)/dsp/dpf.ts) $(AS_ASSEMBLY_PATH)
endif

AS_BUILD_PATH = $(HIPHOP_AS_DSP_PATH)/build
WASM_BYTECODE_PATH = $(AS_BUILD_PATH)/$(WASM_BYTECODE_FILE)
WASM_BINARY_PATH = $(AS_BUILD_PATH)/$(WASM_BINARY_FILE)

HIPHOP_TARGET += $(WASM_BYTECODE_PATH)

$(WASM_BYTECODE_PATH): $(AS_ASSEMBLY_PATH)/plugin.ts
	@echo "Building AssemblyScript project"
	@# npm --prefix fails on MinGW due to paths mixing \ and /
	@test -d $(HIPHOP_AS_DSP_PATH)/node_modules \
		|| (cd $(HIPHOP_AS_DSP_PATH) && $(NPM_OPT_SET_PATH) && npm install)
	@cd $(HIPHOP_AS_DSP_PATH) && $(NPM_OPT_SET_PATH) && npm run asbuild

ifeq ($(HIPHOP_WASM_RUNTIME),wamr)
ifeq ($(HIPHOP_WASM_MODE),aot)
HIPHOP_TARGET += $(WASM_BINARY_PATH)

ifeq ($(CPU_I386_OR_X86_64),true)
# https://github.com/bytecodealliance/wasm-micro-runtime/issues/1022
WAMRC_ARGS = --cpu=sandybridge
endif

$(WASM_BINARY_PATH): $(WASM_BYTECODE_PATH)
	@echo "Compiling WASM AOT module"
	@$(WAMRC_BIN_PATH) --target=$(WAMRC_TARGET) -o $(WASM_BINARY_PATH) $(WAMRC_ARGS) \
		$(WASM_BYTECODE_PATH)
endif
endif
endif

# ------------------------------------------------------------------------------
# Post build - Always copy AssemblyScript DSP binary

ifneq ($(WASM_DSP),)
HIPHOP_TARGET += lib_dsp

lib_dsp:
	@echo "Copying WebAssembly DSP binary"
	@($(TEST_LV2) \
		&& mkdir -p $(LIB_DIR_LV2)/dsp \
		&& cp -r $(WASM_BINARY_PATH) $(LIB_DIR_LV2)/dsp/$(WASM_BINARY_FILE) \
		) || true
	@($(TEST_CLAP_MACOS) \
		&& mkdir -p $(LIB_DIR_CLAP_MACOS)/dsp \
		&& cp -r $(WASM_BINARY_PATH) $(LIB_DIR_CLAP_MACOS)/dsp/$(WASM_BINARY_FILE) \
		) || true
	@($(TEST_VST3) \
		&& mkdir -p $(LIB_DIR_VST3)/dsp \
		&& cp -r $(WASM_BINARY_PATH) $(LIB_DIR_VST3)/dsp/$(WASM_BINARY_FILE) \
		) || true
	@($(TEST_VST2_MACOS) \
		&& mkdir -p $(LIB_DIR_VST2_MACOS)/dsp \
		&& cp -r $(WASM_BINARY_PATH) $(LIB_DIR_VST2_MACOS)/dsp/$(WASM_BINARY_FILE) \
		) || true
	@($(TEST_NOBUNDLE) \
		&& mkdir -p $(LIB_DIR_NOBUNDLE)/dsp \
		&& cp -r $(WASM_BINARY_PATH) $(LIB_DIR_NOBUNDLE)/dsp/$(WASM_BINARY_FILE) \
		) || true
endif

# ------------------------------------------------------------------------------
# Post build - Copy Windows WAMR DLL, currently only 64-bit is supported

ifeq ($(WASM_DSP),true)
ifeq ($(WINDOWS),true)
ifeq ($(HIPHOP_WASM_RUNTIME),wamr)
ifeq ($(HIPHOP_WASM_MODE),aot)
HIPHOP_TARGET += wamr_dll

wamr_dll:
	@($(TEST_LV2) \
		&& mkdir -p $(LIB_DIR_LV2) \
		&& cp $(WAMR_DLL_PATH) $(LIB_DIR_LV2) \
		) || true
	@($(TEST_VST3) \
		&& mkdir -p $(LIB_DIR_VST3) \
		&& cp $(WAMR_DLL_PATH) $(LIB_DIR_VST3) \
		) || true
	@($(TEST_NOBUNDLE) \
		&& mkdir -p $(LIB_DIR_NOBUNDLE) \
		&& cp $(WAMR_DLL_PATH) $(LIB_DIR_NOBUNDLE) \
		) || true
endif
endif
endif
endif

# ------------------------------------------------------------------------------
# Post build - Create LV2 manifest files

ifneq ($(CROSS_COMPILING),true)
CAN_GENERATE_TTL = true
else ifneq ($(EXE_WRAPPER),)
CAN_GENERATE_TTL = true
endif

ifeq ($(CAN_GENERATE_TTL),true)
HIPHOP_TARGET += lv2ttl

lv2ttl: $(DPF_PATH)/utils/lv2_ttl_generator
	@# generate-ttl.sh expects hardcoded directory bin/
	@cd $(DPF_TARGET_DIR)/.. && $(abspath $(DPF_PATH))/utils/generate-ttl.sh

$(DPF_PATH)/utils/lv2_ttl_generator:
	$(MAKE) -C $(DPF_PATH)/utils/lv2-ttl-generator
endif

# ------------------------------------------------------------------------------
# Extend DPF clean target

ifeq ($(WASM_DSP),true)
clean: clean_wasm

clean_wasm:
	rm -rf $(HIPHOP_AS_DSP_PATH)/build
endif
