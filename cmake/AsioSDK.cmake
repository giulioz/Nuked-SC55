include(FetchContent)

option(CMAKE_TLS_VERIFY "Verify SSL certificates" ON)

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
cmake_policy(SET CMP0135 NEW)
endif()

function(download_file url hash)

FetchContent_Declare(download_${hash}
URL ${url}
URL_HASH SHA256=${hash}
)

if(NOT download_${hash}_POPULATED)
  FetchContent_Populate(download_${hash})
endif()

endfunction(download_file)

download_file(
  https://www.steinberg.net/asiosdk
  bc425d9b98701af74b43639798566c48bc005af7328a2251cff722c1885076b2
)
