cmake_minimum_required(VERSION 3.24)
project(WhatsAppWrapper LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  WebView2
  URL https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(WebView2)

add_executable(WhatsAppWrapper WIN32
  src/main.cpp
  src/window.cpp
  src/window.h
)

target_include_directories(WhatsAppWrapper PRIVATE
  "${webview2_SOURCE_DIR}/build/native/include"
)

target_link_libraries(WhatsAppWrapper PRIVATE
  "${webview2_SOURCE_DIR}/build/native/x64/WebView2LoaderStatic.lib"
  shlwapi
  shell32
  ole32
  user32
  gdi32
)

target_compile_definitions(WhatsAppWrapper PRIVATE UNICODE _UNICODE)
