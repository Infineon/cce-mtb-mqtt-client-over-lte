################################################################################
# \file Makefile
# \version 1.0
#
# \brief
# Top-level application make file.
#
################################################################################
# \copyright
# $ Copyright 2018-YEAR Cypress Semiconductor Apache2 $
################################################################################


################################################################################
# Basic Configuration
################################################################################

# Target board/hardware (BSP).
# To change the target, it is recommended to use the Library manager
# ('make modlibs' from command line), which will also update Eclipse IDE launch
# configurations. If TARGET is manually edited, ensure TARGET_<BSP>.mtb with a
# valid URL exists in the application, run 'make getlibs' to fetch BSP contents
# and update or regenerate launch configurations for your IDE.
TARGET=CY8CKIT-062-WIFI-BT

# Name of application (used to derive name of final linked file).
#
# If APPNAME is edited, ensure to update or regenerate launch
# configurations for your IDE.
APPNAME=cce-mtb-mqtt-client-over-LTE

# Name of toolchain to use. Options include:
#
# GCC_ARM -- GCC provided with ModusToolbox IDE
# ARM     -- ARM Compiler (must be installed separately)
# IAR     -- IAR Compiler (must be installed separately)
#
# See also: CY_COMPILER_PATH below
TOOLCHAIN=GCC_ARM

# Default build configuration. Options include:
#
# Debug -- build with minimal optimizations, focus on debugging.
# Release -- build with full optimizations
# Custom -- build with custom configuration, set the optimization flag in CFLAGS
#
# If CONFIG is manually edited, ensure to update or regenerate launch configurations
# for your IDE.
CONFIG=Debug

# If set to "true" or "1", display full command-lines when building.
VERBOSE=

################################################################################
# Advanced Configuration
################################################################################

# Enable optional code that is ordinarily disabled by default.
#
# Available components depend on the specific targeted hardware and firmware
# in use. In general, if you have
#
#    COMPONENTS=foo bar
#
# ... then code in directories named COMPONENT_foo and COMPONENT_bar will be
# added to the build
#
COMPONENTS=FREERTOS LWIP MBEDTLS SECURE_SOCKETS RTOS_AWARE 4343W MURATA-1DX BSP_DESIGN_MODUS
#ifeq ($(TARGET), CY8CKIT-062-WIFI-BT)
#COMPONENTS+=UDB_SDIO_P2 CY8CKIT_062_WIFI_BT
#endif 

# Like COMPONENTS, but disable optional code that was enabled by default.
DISABLE_COMPONENTS=

# By default the build system automatically looks in the Makefile's directory
# tree for source code and builds it. The SOURCES variable can be used to
# manually add source code to the build process from a location not searched
# by default, or otherwise not found by the build system.
SOURCES=

# Like SOURCES, but for include directories. Value should be paths to
# directories (without a leading -I).
INCLUDES=./configs

# Custom configuration of mbedtls library.
MBEDTLSFLAGS = MBEDTLS_USER_CONFIG_FILE='"mbedtls_user_config.h"'

# Add additional defines to the build process (without a leading -D).
DEFINES=$(MBEDTLSFLAGS) CYBSP_PPP_CAPABLE CY_RETARGET_IO_CONVERT_LF_TO_CRLF CY_SERIAL_FLASH_QSPI_THREAD_SAFE

# CY8CPROTO-062-4343W board shares the same GPIO for the user button (USER BTN1)
# and the CYW4343W host wake up pin. Since this example uses the GPIO for  
# interfacing with the user button, the SDIO interrupt to wake up the host is
# disabled by setting CY_WIFI_HOST_WAKE_SW_FORCE to '0'.
ifeq ($(TARGET), CY8CPROTO-062-4343W)
DEFINES+=CY_WIFI_HOST_WAKE_SW_FORCE=0
endif

# Select softfp or hardfp floating point. Default is softfp.
VFP_SELECT=

# Additional / custom C compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CFLAGS=

# Additional / custom C++ compiler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
CXXFLAGS=

# Additional / custom assembler flags.
#
# NOTE: Includes and defines should use the INCLUDES and DEFINES variable
# above.
ASFLAGS=

# Additional / custom linker flags.
LDFLAGS=

# Additional / custom libraries to link in to the application.
LDLIBS=

# Path to the linker script to use (if empty, use the default linker script).
LINKER_SCRIPT=

# Custom pre-build commands to run.
PREBUILD=

# Custom post-build commands to run.
POSTBUILD=

# Place the Wi-Fi firmware into external flash
ifeq ($(TARGET), CY8CPROTO-062S3-4343W)
DEFINES+=CY_ENABLE_XIP_PROGRAM
DEFINES+=CY_STORAGE_WIFI_DATA=\".cy_xip\"
endif
################################################################################
# Paths
################################################################################

# Relative path to the project directory (default is the Makefile's directory).
#
# This controls where automatic source code discovery looks for code.
CY_APP_PATH=

# Relative path to the shared repo location.
#
# All .mtb files have the format, <URI>#<COMMIT>#<LOCATION>. If the <LOCATION> field
# begins with $$ASSET_REPO$$, then the repo is deposited in the path specified by
# the CY_GETLIBS_SHARED_PATH variable. The default location is one directory level
# above the current app directory.
# This is used with CY_GETLIBS_SHARED_NAME variable, which specifies the directory name.
CY_GETLIBS_SHARED_PATH=../

# Directory name of the shared repo location.
#
CY_GETLIBS_SHARED_NAME=mtb_shared

# Absolute path to the compiler's "bin" directory.
#
# The default depends on the selected TOOLCHAIN (GCC_ARM uses the ModusToolbox
# IDE provided compiler by default).
CY_COMPILER_PATH=



CY_IGNORE+=../mtb_shared/wifi-connection-manager
CY_IGNORE+=../mtb_shared/wifi-host-driver
CY_IGNORE+=../mtb_shared/whd-bsp-integration
CY_IGNORE+=../mtb_shared/wpa3-external-supplicant
CY_IGNORE+=../mtb_shared/wifi-core-freertos-lwip-mbedtls
CY_IGNORE+=../mtb_shared/connectivity-utilities
CY_IGNORE+=../mtb_shared/wifi-mw-core/release-v3.4.0/lwip-whd-port
# issue with ignoring .cyignore in wifi bundle
CY_IGNORE+=../mtb_shared/lwip-network-interface-integration/release-v1.0.0
CY_IGNORE+=../mtb_shared/secure-sockets/release-v2.5.0
CY_IGNORE+=../mtb_shared/lwip-network-interface-integration/release-v1/doxygen
CY_IGNORE+=../mtb_shared/lwip-network-interface-integration/release-v1/test
CY_IGNORE+=../mtb_shared/secure-sockets/release-v3/doxygen
CY_IGNORE+=../mtb_shared/secure-sockets/release-v3/test


# Locate ModusToolbox IDE helper tools folders in default installation
# locations for Windows, Linux, and macOS.
CY_WIN_HOME=$(subst \,/,$(USERPROFILE))
CY_TOOLS_PATHS ?= $(wildcard \
    $(CY_WIN_HOME)/ModusToolbox/tools_2.4 \
    $(HOME)/ModusToolbox/tools_2.4 \
    /Applications/ModusToolbox/tools_2.4)

# If you install ModusToolbox IDE in a custom location, add the path to its
# "tools_X.Y" folder (where X and Y are the version number of the tools
# folder). Make sure you use forward slashes.
CY_TOOLS_PATHS+=

# Default to the newest installed tools folder, or the users override (if it's
# found).
CY_TOOLS_DIR=$(lastword $(sort $(wildcard $(CY_TOOLS_PATHS))))

ifeq ($(CY_TOOLS_DIR),)
$(error Unable to find any of the available CY_TOOLS_PATHS -- $(CY_TOOLS_PATHS). On Windows, use forward slashes.)
endif

$(info Tools Directory: $(CY_TOOLS_DIR))

include $(CY_TOOLS_DIR)/make/start.mk
