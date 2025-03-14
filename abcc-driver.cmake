# The CMake command include_guard is compatible with CMake version 3.10 and greater.
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
# Preventing this file to be included more than once.
   include_guard(GLOBAL)
endif()

# Complete list of source (.c) files inside the Anybus CompactCom Driver. 
set(abcc_driver_SRCS
   ${ABCC_DRIVER_DIR}/src/abcc_command_sequencer.c
   ${ABCC_DRIVER_DIR}/src/abcc_copy.c
   ${ABCC_DRIVER_DIR}/src/abcc_handler.c
   ${ABCC_DRIVER_DIR}/src/abcc_link.c
   ${ABCC_DRIVER_DIR}/src/abcc_log.c
   ${ABCC_DRIVER_DIR}/src/abcc_memory.c
   ${ABCC_DRIVER_DIR}/src/abcc_remap.c
   ${ABCC_DRIVER_DIR}/src/abcc_segmentation.c
   ${ABCC_DRIVER_DIR}/src/abcc_setup.c
   ${ABCC_DRIVER_DIR}/src/abcc_timer.c
   ${ABCC_DRIVER_DIR}/src/par/abcc_handler_parallel.c
   ${ABCC_DRIVER_DIR}/src/par/abcc_parallel_driver.c
   ${ABCC_DRIVER_DIR}/src/serial/abcc_crc16.c
   ${ABCC_DRIVER_DIR}/src/serial/abcc_handler_serial.c
   ${ABCC_DRIVER_DIR}/src/serial/abcc_serial_driver.c
   ${ABCC_DRIVER_DIR}/src/spi/abcc_crc32.c
   ${ABCC_DRIVER_DIR}/src/spi/abcc_handler_spi.c
   ${ABCC_DRIVER_DIR}/src/spi/abcc_spi_driver.c
)

# Complete list of header (.h) files inside the Anybus CompactCom Driver. This is
# only used to arrange the files correctly for display in IDEs.
set(abcc_driver_INCS
   ${ABCC_DRIVER_DIR}/inc/abcc.h
   ${ABCC_DRIVER_DIR}/inc/abcc_application_data_interface.h
   ${ABCC_DRIVER_DIR}/inc/abcc_command_sequencer_interface.h
   ${ABCC_DRIVER_DIR}/inc/abcc_config.h
   ${ABCC_DRIVER_DIR}/inc/abcc_error_codes.h
   ${ABCC_DRIVER_DIR}/inc/abcc_hardware_abstraction.h
   ${ABCC_DRIVER_DIR}/inc/abcc_hardware_abstraction_parallel.h
   ${ABCC_DRIVER_DIR}/inc/abcc_hardware_abstraction_serial.h
   ${ABCC_DRIVER_DIR}/inc/abcc_hardware_abstraction_spi.h
   ${ABCC_DRIVER_DIR}/inc/abcc_log.h
   ${ABCC_DRIVER_DIR}/inc/abcc_message.h
   ${ABCC_DRIVER_DIR}/inc/abcc_port.h
   ${ABCC_DRIVER_DIR}/src/abcc_command_sequencer.h
   ${ABCC_DRIVER_DIR}/src/abcc_driver_interface.h
   ${ABCC_DRIVER_DIR}/src/abcc_handler.h
   ${ABCC_DRIVER_DIR}/src/abcc_link.h
   ${ABCC_DRIVER_DIR}/src/abcc_memory.h
   ${ABCC_DRIVER_DIR}/src/abcc_segmentation.h
   ${ABCC_DRIVER_DIR}/src/abcc_setup.h
   ${ABCC_DRIVER_DIR}/src/abcc_timer.h
   ${ABCC_DRIVER_DIR}/src/par/abcc_driver_parallel_interface.h
   ${ABCC_DRIVER_DIR}/src/spi/abcc_crc32.h
   ${ABCC_DRIVER_DIR}/src/spi/abcc_driver_spi_interface.h
   ${ABCC_DRIVER_DIR}/src/serial/abcc_crc16.h
   ${ABCC_DRIVER_DIR}/src/serial/abcc_driver_serial_interface.h
)

# Creating a library target containing the Anybus CompactCom Driver.
# The header files are added only to keep the file and directory tree structure.
add_library(abcc_driver STATIC 
   ${abcc_driver_SRCS}
   ${abcc_driver_INCS}
)

# Keeping the file and directory tree structure of the Anybus CompactCom Driver when 
# generating IDE projects.
source_group(TREE ${ABCC_DRIVER_DIR} PREFIX "abcc-driver" FILES 
   ${abcc_driver_SRCS}
   ${abcc_driver_INCS}
)

# The directory containing the Anybus CompactCom Driver repository.
set(ABCC_ABP_DIR ${ABCC_DRIVER_DIR}/abcc-abp)

# Including the Anybus CompactCom Driver's CMake module file.
include(${ABCC_ABP_DIR}/abcc-abp.cmake)

# Directories inside the Anybus CompactCom Driver containing header (.h) files to be 
# externally accessible is appended to the list ABCC_DRIVER_INCLUDE_DIRS.
list(APPEND ABCC_DRIVER_INCLUDE_DIRS
   ${ABCC_ABP_INCLUDE_DIRS}
   ${ABCC_DRIVER_DIR}/inc
)

# Adding all the Anybus CompactCom Driver related header (.h) directories to the
# Anybus CompactCom Driver library target.
target_include_directories(abcc_driver PRIVATE ${ABCC_DRIVER_INCLUDE_DIRS})

# Link the Anybus CompactCom Driver library to the Anybus CompactCom API library.
target_link_libraries(abcc_driver abcc_abp)