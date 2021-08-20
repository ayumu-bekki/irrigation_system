#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := irrigation_system

GIT_VERSION := "$(shell git describe --dirty --always --tags)"
EXTRA_CPPFLAGS += -DGIT_VERSION=\"$(GIT_VERSION)\"

include $(IDF_PATH)/make/project.mk
