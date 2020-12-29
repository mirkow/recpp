# recpp
C++ Refactoring tool using libclang


## FAQ

### Standard headers are not found
Depending on which compiler you use in your cmake project, you might need to add aditional includes to the tool via "--extra-arg=-I/your/compiler/includes" since the compiler uses implicit includes that are unknown to clang. For gcc you can find out which includes are used by adding -v to "CMAKE_CXX_FLAGS" and compile the file with which the tool has problems. This way, recpp will also print which includes it uses to build the AST. Or try "gcc -xc++ -E -v -".
