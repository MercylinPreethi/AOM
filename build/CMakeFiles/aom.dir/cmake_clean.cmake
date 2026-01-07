file(REMOVE_RECURSE
  "aom.lib"
  "aom.pdb"
)

# Per-language clean rules from dependency scanning.
foreach(lang ASM_NASM C)
  include(CMakeFiles/aom.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
