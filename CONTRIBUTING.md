# Contributing Guideline
- Keep The Programming Language Syntax As Easier
- Use Optimized Runtime Instead Of Standard Runtime On Transpiler, And Sandbox
  But Runtime Is Located At `lib/runtime.asm`

## Contribute
Required Dependencies:
- `cmake`: Version 3.28 or higher (for configuration)
- `python3`: For higher-level build automation
- `qt5`: Qt5::Core and Qt5::Network modules (for argument parsing and HTTP requests)
- `llvm`: Version 18.1 or higher (for assembling/compiling), but lower than LLVM 19
- `clang` development headers: `libclang-18-dev` (for C/C++ AST parsing)
- `lldb` development headers: `liblldb-18-dev` (for debugger integration)
- `nasm`: Netwide Assembler (for own runtime assembly)
- `pulseaudio`: For playing successul/failure sounds
- `zlib`: Compression library
- `seccomp`: Linux secure computing mode library
- `lldb`: LLDB API for debugging integration
- `sphinx`: For Documentation Generation
- `insset`: For Instruction Set Simulation (git Submodule)

Required Files Structure
```bash
root
|---docs                            # Documentation Directory
|---include                         # Header Directory
    |------definations.h            # Global Header
    |------lexer.h                  # Header Of Lexer
    |------parser.h                 # Header Of Parser
    |------miniaudio.h              # CLI Audio Player Header
    |------preprocessor.h           # Header Of Preprocessor/Optimizer
    |------runtime.h                # Header Of Optimized Runtime
    |------transpiler.h             # Header Of Transpiler/Translator
    |------ai.h                     # AI Integration Header
    |------cparser.h                # C/C++ Parser Header (Not confusion with parser.cpp)
    |------debugger.h               # Debugger Wrapper Header
    |------emulatorUtil.h           # Isolated Emulator Wrapper Header
    |------dynvar.h                 # Dynamic Variable Header
|---lib                             # External Libraries
|---media                           # Audio Directory
|---sample                          # Sample Programs
    |------helloWorld.br            # Hello World Sample
    |------calculator.br            # Calculator Sample
    |------functions.br             # Functions Sample
    |------guessTheNumber.br        # Guess The Number Sample
    |------commenting.br            # Commenting Sample
    |------macro.br                 # Macro Sample
    |------dtype.br                 # Data Type Sample
    |------ifElse.br                # If-Else Sample
|---scripts                         # Higher Level Scripts To Run
    |------generateAudioHeaders.py  # Audio Header Generation Script
|---src                             # Main Source Directory
    |--main.cpp                     # Main Executeable Source Code
    |--lexer.cpp                    # Lexer
    |--parser.cpp                   # Parser
    |--preprocessor.cpp             # Preprocessor/Optimizer
    |--transpiler.cpp               # Transpiler/Translator
    |--ai.cpp                       # AI Integration
    |--cparser.cpp                  # C/C++ Parser (Not confusion with parser.cpp)
    |--debugger.cpp                 # Debugger Wrapper
    |--emulatorUtil.c               # Isolated Emulator Wrapper
```

Building The Compiler:
```bash
git submodule update --init --recursive --remote
cmake -B build
cd build
make -j $(nproc)
make docs
```
Note: You need to have `sphinx` installed to build the documentation. and builded documentation is generated in `build/docs` directory after building. otherwise documentation source code is located on `docs` directory.
