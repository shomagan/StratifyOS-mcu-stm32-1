file(GLOB STM32F4XX_SOURCES ${CMAKE_SOURCE_DIR}/src/stm32f4xx/*)


file(GLOB STM32_VARIANT_SOURCES "${CMAKE_SOURCE_DIR}/src/${STM32_VARIANT}/*.c")
file(GLOB STM32_VARIANT_HEADERS	${CMAKE_SOURCE_DIR}/include/*${STM32_VARIANT}.h ${CMAKE_SOURCE_DIR}/include/cmsis/${STM32_VARIANT}.h)
set(SOS_LIB_SOURCELIST ${SHARED_SOURCELIST} ${STM32F4XX_SOURCES} ${STM32_VARIANT_SOURCES} ${STM32_VARIANT_HEADERS})

set(SOS_LIB_OPTION mcu_${STM32_VARIANT})
set(SOS_LIB_DEFINITIONS __${STM32_VARIANT} ${STM32_VARIANT_DEFINE}=1)
set(SOS_LIB_ARCH armv7e-m)
set(SOS_LIB_TYPE release)
include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-lib.cmake)
set(SOS_LIB_TYPE debug)
include(${SOS_TOOLCHAIN_CMAKE_PATH}/sos-lib.cmake)
