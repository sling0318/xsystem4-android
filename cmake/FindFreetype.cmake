if (NOT TARGET Freetype::Freetype)
  include(FetchContent)
  FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-3
  )
  if (IOS)
    # Without these freetype picks up the host's brotli/bzip2/harfbuzz and
    # leaves their symbols unresolved in the iOS archive.
    set(FT_DISABLE_BROTLI TRUE)
    set(FT_DISABLE_BZIP2 TRUE)
    set(FT_DISABLE_HARFBUZZ TRUE)
  endif()
  FetchContent_MakeAvailable(freetype)
  add_library(Freetype::Freetype ALIAS freetype)
  install(TARGETS freetype)
endif()
