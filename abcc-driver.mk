# Makefile for the Anybus CompactCom Driver
#
# How to use:
#  - Create ABCC_DRIVER_DIR containing the path to the
#    Anybus CompactCom Driver directory.
#  - Create SRCS, add your source files to it (optional),
#    and create object files from the content of SRCS.
#  - Create INCLUDES, add your include paths to it
#    (optional), and append the content as compiler flags
#    when compiling.

# store the path to the Anybus CompactCom Protocol
ABCC_ABP_DIR := $(ABCC_DRIVER_DIR)/abcc-abp

# include the Anybus Protocol
include $(ABCC_ABP_DIR)/abcc-abp.mk

# add the Anybus CompactCom Driver source files
SRCS += $(ABCC_DRIVER_DIR)/src/abcc_command_sequencer.c
SRCS += $(ABCC_DRIVER_DIR)/src/abcc_copy.c
SRCS += $(ABCC_DRIVER_DIR)/src/abcc_handler.c
SRCS += $(ABCC_DRIVER_DIR)/src/abcc_link.c
SRCS += $(ABCC_DRIVER_DIR)/src/abcc_log.c
SRCS += $(ABCC_DRIVER_DIR)/src/abcc_memory.c
SRCS += $(ABCC_DRIVER_DIR)/src/abcc_remap.c
SRCS += $(ABCC_DRIVER_DIR)/src/abcc_segmentation.c
SRCS += $(ABCC_DRIVER_DIR)/src/abcc_setup.c
SRCS += $(ABCC_DRIVER_DIR)/src/abcc_timer.c
SRCS += $(ABCC_DRIVER_DIR)/src/par/abcc_handler_parallel.c
SRCS += $(ABCC_DRIVER_DIR)/src/par/abcc_parallel_driver.c
SRCS += $(ABCC_DRIVER_DIR)/src/serial/abcc_crc16.c
SRCS += $(ABCC_DRIVER_DIR)/src/serial/abcc_handler_serial.c
SRCS += $(ABCC_DRIVER_DIR)/src/serial/abcc_serial_driver.c
SRCS += $(ABCC_DRIVER_DIR)/src/spi/abcc_crc32.c
SRCS += $(ABCC_DRIVER_DIR)/src/spi/abcc_handler_spi.c
SRCS += $(ABCC_DRIVER_DIR)/src/spi/abcc_spi_driver.c

# add the Anybus CompactCom Driver include directory
INCLUDES += -I$(ABCC_DRIVER_DIR)/inc