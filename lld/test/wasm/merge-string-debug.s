# RUN: llvm-mc -filetype=obj -triple=wasm32-unknown-unknown %s -o %t.o
# RUN: llvm-mc -filetype=obj -triple=wasm32-unknown-unknown %p/Inputs/merge-string-debug2.s -o %t2.o

# RUN: wasm-ld %t.o %t2.o -o %t.wasm --no-entry
# RUN: llvm-readobj -x .debug_str %t.wasm | FileCheck %s --check-prefixes CHECK,CHECK-O1

# RUN: wasm-ld -O0 %t.o %t2.o -o %tO0.wasm --no-entry
# RUN: llvm-readobj -x .debug_str %tO0.wasm | FileCheck %s --check-prefixes CHECK,CHECK-O0

.section .debug_str,"S",@
  .asciz "clang version 13.0.0"
  .asciz "foobar"

# CHECK: Hex dump of section '.debug_str':

# CHECK-O0: 0x00000000 636c616e 67207665 7273696f 6e203133 clang version 13
# CHECK-O0: 0x00000010 2e302e30 00666f6f 62617200 636c616e .0.0.foobar.clan
# CHECK-O0: 0x00000020 67207665 7273696f 6e203133 2e302e30 g version 13.0.0
# CHECK-O0: 0x00000030 00626172 00666f6f 00                .bar.foo.

# CHECK-O1: 0x00000000 666f6f62 61720066 6f6f0063 6c616e67 foobar.foo.clang
# CHECK-O1: 0x00000010 20766572 73696f6e 2031332e 302e3000  version 13.0.0.
