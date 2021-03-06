// Copyright 2013 the V8 project authors. All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//     * Neither the name of Google Inc. nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#include <stdlib.h>

#include <limits>

#include "v8.h"

#include "cctest.h"
#include "code-stubs.h"
#include "test-code-stubs.h"
#include "factory.h"
#include "macro-assembler.h"
#include "platform.h"

using namespace v8::internal;

#define __ assm.

ConvertDToIFunc MakeConvertDToIFuncTrampoline(Isolate* isolate,
                                              Register source_reg,
                                              Register destination_reg) {
  // Allocate an executable page of memory.
  size_t actual_size;
  byte* buffer = static_cast<byte*>(OS::Allocate(Assembler::kMinimalBufferSize,
                                                   &actual_size,
                                                   true));
  CHECK(buffer);
  HandleScope handles(isolate);
  MacroAssembler assm(isolate, buffer, static_cast<int>(actual_size));
  assm.set_allow_stub_calls(false);
  int offset =
    source_reg.is(esp) ? 0 : (HeapNumber::kValueOffset - kSmiTagSize);
  DoubleToIStub stub(source_reg, destination_reg, offset, true);
  byte* start = stub.GetCode(isolate)->instruction_start();

  __ push(ebx);
  __ push(ecx);
  __ push(edx);
  __ push(esi);
  __ push(edi);

  if (!source_reg.is(esp)) {
    __ lea(source_reg, MemOperand(esp, 6 * kPointerSize - offset));
  }

  int param_offset = 7 * kPointerSize;
  // Save registers make sure they don't get clobbered.
  int reg_num = 0;
  for (;reg_num < Register::NumAllocatableRegisters(); ++reg_num) {
    Register reg = Register::from_code(reg_num);
    if (!reg.is(esp) && !reg.is(ebp) && !reg.is(destination_reg)) {
      __ push(reg);
      param_offset += kPointerSize;
    }
  }

  // Re-push the double argument
  __ push(MemOperand(esp, param_offset));
  __ push(MemOperand(esp, param_offset));

  // Call through to the actual stub
  __ call(start, RelocInfo::EXTERNAL_REFERENCE);

  __ add(esp, Immediate(kDoubleSize));

  // Make sure no registers have been unexpectedly clobbered
  for (--reg_num; reg_num >= 0; --reg_num) {
    Register reg = Register::from_code(reg_num);
    if (!reg.is(esp) && !reg.is(ebp) && !reg.is(destination_reg)) {
      __ cmp(reg, MemOperand(esp, 0));
      __ Assert(equal, "register was clobbered");
      __ add(esp, Immediate(kPointerSize));
    }
  }

  __ mov(eax, destination_reg);

  __ pop(edi);
  __ pop(esi);
  __ pop(edx);
  __ pop(ecx);
  __ pop(ebx);

  __ ret(kDoubleSize);

  CodeDesc desc;
  assm.GetCode(&desc);
  return reinterpret_cast<ConvertDToIFunc>(
      reinterpret_cast<intptr_t>(buffer));
}

#undef __


static Isolate* GetIsolateFrom(LocalContext* context) {
  return reinterpret_cast<Isolate*>((*context)->GetIsolate());
}


TEST(ConvertDToI) {
  CcTest::InitializeVM();
  LocalContext context;
  Isolate* isolate = GetIsolateFrom(&context);
  HandleScope scope(isolate);

#if DEBUG
  // Verify that the tests actually work with the C version. In the release
  // code, the compiler optimizes it away because it's all constant, but does it
  // wrong, triggering an assert on gcc.
  RunAllTruncationTests(&ConvertDToICVersion);
#endif

  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esp, eax));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esp, ebx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esp, ecx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esp, edx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esp, edi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esp, esi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, eax, eax));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, eax, ebx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, eax, ecx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, eax, edx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, eax, edi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, eax, esi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ebx, eax));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ebx, ebx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ebx, ecx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ebx, edx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ebx, edi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ebx, esi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ecx, eax));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ecx, ebx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ecx, ecx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ecx, edx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ecx, edi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, ecx, esi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edx, eax));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edx, ebx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edx, ecx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edx, edx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edx, edi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edx, esi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esi, eax));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esi, ebx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esi, ecx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esi, edx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esi, edi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, esi, esi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edi, eax));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edi, ebx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edi, ecx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edi, edx));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edi, edi));
  RunAllTruncationTests(MakeConvertDToIFuncTrampoline(isolate, edi, esi));
}
