// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.Diagnostics;
using Internal.TypeSystem;

#nullable enable

namespace ILCompiler.Dataflow
{
    // This represents a field which has been generated by the compiler as the
    // storage location for a hoisted local (a local variable which is lifted to a
    // field on a state machine type, or to a field on a closure accessed by lambdas
    // or local functions).
    public readonly struct HoistedLocalKey : IEquatable<HoistedLocalKey>
    {
        readonly FieldDesc Field;

        public HoistedLocalKey(FieldDesc field)
        {
            Debug.Assert(CompilerGeneratedState.IsHoistedLocal(field));
            Field = field;
        }

        public bool Equals(HoistedLocalKey other) => Field.Equals(other.Field);

        public override bool Equals(object? obj) => obj is HoistedLocalKey other && Equals(other);

        public override int GetHashCode() => Field.GetHashCode();
    }
}
