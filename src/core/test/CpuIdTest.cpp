/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "core/CpuId.h"
#include <gtest/gtest.h>
#include <iostream>

using namespace std;


template <typename Stream>
static void OutputCpuid(Stream& out)
{
    CpuId cpuid;
    out << "Cpuid: " << endl;
#define FN(a, x)    a.x()
#define OUTPUT(x)   out << "\t" << #x << ": " << FN(cpuid, x) << endl
    OUTPUT(sse3);
    OUTPUT(pclmuldq);
    OUTPUT(dtes64);
    OUTPUT(monitor);
    OUTPUT(dscpl);
    OUTPUT(vmx);
    OUTPUT(smx);
    OUTPUT(eist);
    OUTPUT(tm2);
    OUTPUT(ssse3);
    OUTPUT(fma);
    OUTPUT(cx16);
    OUTPUT(xtpr);
    OUTPUT(pdcm);
    OUTPUT(pcid);
    OUTPUT(sse41);
    OUTPUT(sse42);
    OUTPUT(x2apic);
    OUTPUT(movbe);
    OUTPUT(popcnt);
    OUTPUT(tscdeadline);
    OUTPUT(aes);
    OUTPUT(xsave);
    OUTPUT(osxsave);
    OUTPUT(avx);
    OUTPUT(osxsave);
    OUTPUT(f16c);
    OUTPUT(rdrand);
    OUTPUT(fpu);
    OUTPUT(vme);
    OUTPUT(de);
    OUTPUT(pse);
    OUTPUT(tsc);
    OUTPUT(msr);
    OUTPUT(pae);
    OUTPUT(mce);
    OUTPUT(cx8);
    OUTPUT(apic);
    OUTPUT(sep);
    OUTPUT(apic);
    OUTPUT(mtrr);
    OUTPUT(apic);
    OUTPUT(pge);
    OUTPUT(mca);
    OUTPUT(cmov);
    OUTPUT(pat);
    OUTPUT(pse36);
    OUTPUT(psn);
    OUTPUT(clfsh);
    OUTPUT(ds);
    OUTPUT(acpi);
    OUTPUT(mmx);
    OUTPUT(fxsr);
    OUTPUT(sse);
    OUTPUT(sse2);
    OUTPUT(ss);
    OUTPUT(htt);
    OUTPUT(tm);
    OUTPUT(pbe);
#undef OUTPUT
#undef FN    
}


TEST(CpuId, Simple)
{
    // All CPUs should support MMX
    CpuId id;
    EXPECT_TRUE(id.mmx());

    //OutputCpuid(cout);
}
