include(FetchContent)

option(CMAKE_TLS_VERIFY "Verify SSL certificates" ON)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(asio_sdk_download
  URL https://www.steinberg.net/asiosdk
  URL_HASH SHA256=bc425d9b98701af74b43639798566c48bc005af7328a2251cff722c1885076b2
)

FetchContent_MakeAvailable(asio_sdk_download)

add_library(
  AsioSDK STATIC EXCLUDE_FROM_ALL
  ${asio_sdk_download_SOURCE_DIR}/host/asiodrivers.cpp
  ${asio_sdk_download_SOURCE_DIR}/host/pc/asiolist.cpp
)

target_include_directories(
  AsioSDK PUBLIC
  ${asio_sdk_download_SOURCE_DIR}/common
  ${asio_sdk_download_SOURCE_DIR}/host
  ${asio_sdk_download_SOURCE_DIR}/host/pc
)
