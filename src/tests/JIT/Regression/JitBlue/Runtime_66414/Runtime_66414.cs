// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// Generated by Fuzzlyn v1.5 on 2022-03-09 14:59:40
// Run on X64 Windows
// Seed: 1682944285549106524
// Reduced from 43.0 KiB to 0.5 KiB in 00:00:58
// Debug: Outputs 0
// Release: Outputs 255
using System.Runtime.CompilerServices;
using Xunit;

public struct S0
{
    public bool F0;
    public short F1;
    public ushort F2;
    public byte F4;
    public sbyte F5;
    public short F6;
    
    [MethodImpl(MethodImplOptions.NoInlining)]
    public int M6()
    {
        return (int)(Runtime_66414.s_3[0] ^ this.F5--);
    }
}

public class Runtime_66414
{
    public static long[] s_3 = new long[]{0};
    [Fact]
    public static int TestEntryPoint()
    {
        var vr1 = new S0();
        return M5(vr1) == 0 ? 100 : -1;
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    public static int M5(S0 arg1)
    {
        arg1.F4 = (byte)(arg1.F5 - (uint)arg1.M6());
        return arg1.F4;
    }
}