# abcc-driver
The Anybus CompactCom Driver is the capable and flexible foundation for your CompactCom host application.

*For a simpler API, visit [abcc-driver-api](https://github.com/hms-networks/abcc-driver-api).*

## Add as submodule (instead of cloning)

### Add this repo as a submodule

*It's suggested to add the repository as a submodule in your projects **lib/** folder.*
```
git submodule add https://github.com/hms-networks/abcc-driver.git lib/abcc-driver
```
This repository already contains another "nested" submodule ([abcc-abp/](https://github.com/hms-networks/abcc-abp)) that must be initialized. Therefore, go to the path of **abcc-driver/** and initialize **abcc-abp/**.
```
cd lib/abcc-driver
```
```
git submodule update --init --recursive
```
Go back to your git repository and stage the new submodules, otherwise they will only be linked locally on your machine.
```
cd ../../
```
```
git add .gitmodules lib/abcc-driver
```

## Cloning

### Use the flag

This repository contains a submodule ([abcc-abp/](https://github.com/hms-networks/abcc-abp)) that must be initialized. Therefore, pass the flag `--recurse-submodules` when cloning.

*It's suggested to clone the repository into your projects **lib/** folder.*
```
git clone --recurse-submodules https://github.com/hms-networks/abcc-driver.git lib/abcc-driver
```

#### (In case you missed it...)

If you did not pass the flag `--recurse-submodules` when cloning, the following command can be run:
```
git submodule update --init --recursive
```

## Building

### Configure

The Anybus CompactCom Driver shall **always** be configured by a file called **abcc_driver_config.h**, created by you, custom to your project. The file shall contain macro definitions to enable, disable, and set values of different features and funtionalities.

### Alternative 1: CMake

This repository can be included as a library into a CMake target by adding a few sections to your **CMakeLists.txt** file.

1. Add your file **abcc_hardware_abstraction.c** to your target.
```
target_sources(<your_target> PRIVATE <"path/to/abcc_hardware_abstraction.c">)
```

2. Create a variable called `ABCC_DRIVER_INCLUDE_DIRS` with directories containing **your** personal include (.h) files related to the CompactCom Driver, such as **abcc_driver_config.h**.
```
set(ABCC_DRIVER_INCLUDE_DIRS
    <${PROJECT_SOURCE_DIR}/path/to/include_directory>
	<...>
)
```

3. Create a variable called `ABCC_DRIVER_DIR` with the path to the directory where the CompactCom Driver repository was cloned.
```
set(ABCC_DRIVER_DIR <"${PROJECT_SOURCE_DIR}/path/to/abcc-driver">)
```

4. Includes the standard CMake file from the CompactCom Driver.
```
include(${ABCC_DRIVER_DIR}/abcc-driver.cmake)
```

5. Add directories containing include (.h) files related to the CompactCom Driver to your target.
```
target_include_directories(host_application_exec PRIVATE ${ABCC_DRIVER_INCLUDE_DIRS})
```

6. Add the CompactCom Driver library to the user host application executable target.
```
target_link_libraries(<your_target> abcc_driver)
```
### Alternative 2: Make

This repository's Makefile, **abcc-driver.mk**, can be included into a Make target by adding a few sections to your higher level Makefile.

1. Create `SRCS`, add your source files to it (optional), and create object files from the content of `SRCS`.
```
SRCS  = ./src/main.c
SRCS += ./src/abcc_adaptation/abcc_hardware_abstraction.c
...
```
2. Create `INCLUDES`, add your include paths to it (optional), and append the content as compiler flags when compiling. *Make sure to include the folder containing **abcc_driver_config.h** somehow, even if it's not in `INCLUDES` specifically.*.
```
INCLUDES = -I./src
INCLUDES = -I./src/abcc_adaptation
...
```
3. Create `ABCC_DRIVER_DIR` containing the path to the Anybus CompactCom Driver directory.
```
ABCC_DRIVER_DIR := ./path/to/abcc-driver
```
4. Include **abcc-driver.mk**.
```
include $(ABCC_DRIVER_DIR)/abcc-driver.mk
```
The CompactCom Driver should now compile together with your target!
