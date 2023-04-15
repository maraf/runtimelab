// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// Adapted from:
// Generated by Fuzzlyn v1.5 on 2022-11-14 02:52:08
// Run on Arm64 Windows
// Seed: 11038715273855459808
// Reduced from 96.6 KiB to 0.9 KiB in 00:08:38
// Debug: Outputs 0
// Release: Outputs 1
using System;
using System.Runtime.CompilerServices;
using Xunit;

public struct S0
{
    public long F;
    public byte FB;
}

public struct S1
{
    public S0 S;
    public long LastField;
}

public class Runtime_78310
{
    [Fact]
    public static int TestEntryPoint()
    {
        S1 lhs = new S1 { S = { F = 1 }, LastField = 2 };
        S1 rhs = new S1 { S = { F = 3 }, LastField = 4 };
        Copy(true, ref lhs, rhs);

        int result = 100;
        if (lhs.S.F != 3 || lhs.LastField != 2)
        {
            Console.WriteLine("FAIL: After small copy is {0}, {1}", lhs.S.F, lhs.LastField);
            result = -1;
        }

        lhs = new S1 { S = { F = 5 }, LastField = 6 };
        rhs = new S1 { S = { F = 7 }, LastField = 8 };
        Copy(false, ref lhs, rhs);
        if (lhs.S.F != 7 || lhs.LastField != 8)
        {
            Console.WriteLine("FAIL: After large copy is {0}, {1}", lhs.S.F, lhs.LastField);
            result = -1;
        }

        return result;
    }

    [MethodImpl(MethodImplOptions.NoInlining)]
    internal static void Copy(bool small, ref S1 lhs, S1 rhs)
    {
        if (small)
        {
            lhs.S = rhs.S;
        }
        else
        {
            lhs = rhs;
        }

        Foo();
    }
    
    [MethodImpl(MethodImplOptions.NoInlining)]
    private static void Foo()
    {
    }
}
