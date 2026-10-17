if (NOT TARGET Freetype::Freetype)
  include(FetchContent)
  FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-3
  )
  if (IOS)
    # Without these freetype picks up the host's brotli/bzip2/harfbuzz and
    # leaves their symbols unresolved in the iOS archive.  These are
    # cmake_dependent_option entries, so only a forced cache value works.
    set(FT_DISABLE_BROTLI TRUE CACHE BOOL "" FORCE)
    set(FT_DISABLE_BZIP2 TRUE CACHE BOOL "" FORCE)
    set(FT_DISABLE_HARFBUZZ TRUE CACHE BOOL "" FORCE)
  endif()
  FetchContent_MakeAvailable(freetype)
  add_library(Freetype::Freetype ALIAS freetype)
  install(TARGETS freetype)
endif()
