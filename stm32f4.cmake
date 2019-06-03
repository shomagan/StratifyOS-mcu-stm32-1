set(STM32_FAMILY stm32f4xx)
set(SOS_INCLUDE_DIRECTORIES include src ${STM32_FAMILY} include/cmsis)

file(GLOB STM32_FAMILY_SOURCES ${CMAKE_SOURCE_DIR}/src/${STM32_FAMILY}/*)
file(GLOB STM32_VARIANT_SOURCES "${CMAKE_SOURCE_DIR}/src/${STM32_VARIANT}/*.c")
file(GLOB STM32_VARIANT_HEADERS	${CMAKE_SOURCE_DIR}/include/*${STM32_VARIANT}.h ${CMAKE_SOURCE_DIR}/include/cmsis/${STM32_FAMILY}.h ${CMAKE_SOURCE_DIR}/include/cmsis/${STM32_VARIANT}.h)
file(GLOB STM32_LDSCRIPT_SOURCES "${CMAKE_SOURCE_DIR}/ldscript/stm32f4*" "${CMAKE_SOURCE_DIR}/ldscript/stm32-common*")
set(SOS_SOURCELIST ${SHARED_SOURCELIST} ${STM32_FAMILY_SOURCES} ${STM32_VARIANT_SOURCES} ${STM32_VARIANT_HEADERS} ${STM32_LDSCRIPT_SOURCES})

set(SOS_OPTION mcu_${STM32_VARIANT})
set(SOS_DEFINITIONS __${STM32_VARIANT} ${STM32_VARIANT_DEFINE}=1)
set(SOS_ARCH v7em_f4sh)
set(SOS_CONFIG release)
include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-lib.cmake)
set(SOS_CONFIG debug)
include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-lib.cmake)
