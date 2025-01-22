function(make_includable input_file output_file)
  file(READ ${input_file} content)
  set(delim "~^~^~^~^")
  set(content "namespace Uhhyou {\nconst char* libraryLicenseText = R\"${delim}(${content})${delim}\";\n}")
  file(WRITE ${output_file} "${content}")
endfunction(make_includable)

make_includable(lib/README.md common/librarylicense.hpp)
