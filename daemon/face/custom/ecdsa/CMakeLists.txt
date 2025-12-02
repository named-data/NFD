
set(ecdsa_header
ecdsa_sig.hpp
)
set(ecdsa_src
ecdsa_sig.cpp
)
add_library(ecdsa STATIC ${ecdsa_src} ${ecdsa_header})
target_compile_options(ecdsa PUBLIC -I${mcl_header_dir})
