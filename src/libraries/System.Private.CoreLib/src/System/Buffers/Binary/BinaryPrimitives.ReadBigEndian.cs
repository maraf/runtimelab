// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace System.Buffers.Binary
{
    public static partial class BinaryPrimitives
    {
        /// <summary>
        /// Reads a <see cref="double" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <param name="source">The read-only span to read.</param>
        /// <returns>The big endian value.</returns>
        /// <remarks>Reads exactly 8 bytes from the beginning of the span.</remarks>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <paramref name="source"/> is too small to contain a <see cref="double" />.
        /// </exception>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static double ReadDoubleBigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                BitConverter.Int64BitsToDouble(ReverseEndianness(MemoryMarshal.Read<long>(source))) :
                MemoryMarshal.Read<double>(source);
        }

        /// <summary>
        /// Reads a <see cref="Half" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <param name="source">The read-only span to read.</param>
        /// <returns>The big endian value.</returns>
        /// <remarks>Reads exactly 2 bytes from the beginning of the span.</remarks>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <paramref name="source"/> is too small to contain a <see cref="Half" />.
        /// </exception>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Half ReadHalfBigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                BitConverter.Int16BitsToHalf(ReverseEndianness(MemoryMarshal.Read<short>(source))) :
                MemoryMarshal.Read<Half>(source);
        }

        /// <summary>
        /// Reads a <see cref="short" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static short ReadInt16BigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                ReverseEndianness(MemoryMarshal.Read<short>(source)) :
                MemoryMarshal.Read<short>(source);
        }

        /// <summary>
        /// Reads a <see cref="int" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static int ReadInt32BigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                ReverseEndianness(MemoryMarshal.Read<int>(source)) :
                MemoryMarshal.Read<int>(source);
        }

        /// <summary>
        /// Reads a <see cref="long" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static long ReadInt64BigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                ReverseEndianness(MemoryMarshal.Read<long>(source)) :
                MemoryMarshal.Read<long>(source);
        }

        /// <summary>
        /// Reads a <see cref="Int128" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static Int128 ReadInt128BigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                ReverseEndianness(MemoryMarshal.Read<Int128>(source)) :
                MemoryMarshal.Read<Int128>(source);
        }

        /// <summary>
        /// Reads a <see cref="nint" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static nint ReadIntPtrBigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                ReverseEndianness(MemoryMarshal.Read<nint>(source)) :
                MemoryMarshal.Read<nint>(source);
        }

        /// <summary>
        /// Reads a <see cref="float" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <param name="source">The read-only span to read.</param>
        /// <returns>The big endian value.</returns>
        /// <remarks>Reads exactly 4 bytes from the beginning of the span.</remarks>
        /// <exception cref="ArgumentOutOfRangeException">
        /// <paramref name="source"/> is too small to contain a <see cref="float" />.
        /// </exception>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static float ReadSingleBigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                BitConverter.Int32BitsToSingle(ReverseEndianness(MemoryMarshal.Read<int>(source))) :
                MemoryMarshal.Read<float>(source);
        }

        /// <summary>
        /// Reads a <see cref="ushort" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        [CLSCompliant(false)]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ushort ReadUInt16BigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                ReverseEndianness(MemoryMarshal.Read<ushort>(source)) :
                MemoryMarshal.Read<ushort>(source);
        }

        /// <summary>
        /// Reads a <see cref="uint" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        [CLSCompliant(false)]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static uint ReadUInt32BigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                ReverseEndianness(MemoryMarshal.Read<uint>(source)) :
                MemoryMarshal.Read<uint>(source);
        }

        /// <summary>
        /// Reads a <see cref="ulong" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        [CLSCompliant(false)]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static ulong ReadUInt64BigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                ReverseEndianness(MemoryMarshal.Read<ulong>(source)) :
                MemoryMarshal.Read<ulong>(source);
        }

        /// <summary>
        /// Reads a <see cref="UInt128" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        [CLSCompliant(false)]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static UInt128 ReadUInt128BigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                ReverseEndianness(MemoryMarshal.Read<UInt128>(source)) :
                MemoryMarshal.Read<UInt128>(source);
        }

        /// <summary>
        /// Reads a <see cref="nuint" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        [CLSCompliant(false)]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static nuint ReadUIntPtrBigEndian(ReadOnlySpan<byte> source)
        {
            return BitConverter.IsLittleEndian ?
                ReverseEndianness(MemoryMarshal.Read<nuint>(source)) :
                MemoryMarshal.Read<nuint>(source);
        }

        /// <summary>
        /// Reads a <see cref="double" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <param name="source">The read-only span of bytes to read.</param>
        /// <param name="value">When this method returns, the value read out of the read-only span of bytes, as big endian.</param>
        /// <returns>
        /// <see langword="true" /> if the span is large enough to contain a <see cref="double" />; otherwise, <see langword="false" />.
        /// </returns>
        /// <remarks>Reads exactly 8 bytes from the beginning of the span.</remarks>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadDoubleBigEndian(ReadOnlySpan<byte> source, out double value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out long tmp);
                value = BitConverter.Int64BitsToDouble(ReverseEndianness(tmp));
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="Half" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <param name="source">The read-only span of bytes to read.</param>
        /// <param name="value">When this method returns, the value read out of the read-only span of bytes, as big endian.</param>
        /// <returns>
        /// <see langword="true" /> if the span is large enough to contain a <see cref="Half" />; otherwise, <see langword="false" />.
        /// </returns>
        /// <remarks>Reads exactly 2 bytes from the beginning of the span.</remarks>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadHalfBigEndian(ReadOnlySpan<byte> source, out Half value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out short tmp);
                value = BitConverter.Int16BitsToHalf(ReverseEndianness(tmp));
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="short" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <returns>If the span is too small to contain a <see cref="short" />, return false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadInt16BigEndian(ReadOnlySpan<byte> source, out short value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out short tmp);
                value = ReverseEndianness(tmp);
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="int" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <returns>If the span is too small to contain a <see cref="int" />, return false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadInt32BigEndian(ReadOnlySpan<byte> source, out int value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out int tmp);
                value = ReverseEndianness(tmp);
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="long" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <returns>If the span is too small to contain a <see cref="long" />, return false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadInt64BigEndian(ReadOnlySpan<byte> source, out long value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out long tmp);
                value = ReverseEndianness(tmp);
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="Int128" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <returns>If the span is too small to contain a <see cref="Int128" />, return false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadInt128BigEndian(ReadOnlySpan<byte> source, out Int128 value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out Int128 tmp);
                value = ReverseEndianness(tmp);
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="nint" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <returns>If the span is too small to contain a <see cref="nint" />, return false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadIntPtrBigEndian(ReadOnlySpan<byte> source, out nint value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out nint tmp);
                value = ReverseEndianness(tmp);
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="float" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <param name="source">The read-only span of bytes to read.</param>
        /// <param name="value">When this method returns, the value read out of the read-only span of bytes, as big endian.</param>
        /// <returns>
        /// <see langword="true" /> if the span is large enough to contain a <see cref="float" />; otherwise, <see langword="false" />.
        /// </returns>
        /// <remarks>Reads exactly 4 bytes from the beginning of the span.</remarks>
        public static bool TryReadSingleBigEndian(ReadOnlySpan<byte> source, out float value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out int tmp);
                value = BitConverter.Int32BitsToSingle(ReverseEndianness(tmp));
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="ushort" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <returns>If the span is too small to contain a <see cref="ushort" />, return false.</returns>
        [CLSCompliant(false)]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadUInt16BigEndian(ReadOnlySpan<byte> source, out ushort value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out ushort tmp);
                value = ReverseEndianness(tmp);
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="uint" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <returns>If the span is too small to contain a <see cref="uint" />, return false.</returns>
        [CLSCompliant(false)]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadUInt32BigEndian(ReadOnlySpan<byte> source, out uint value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out uint tmp);
                value = ReverseEndianness(tmp);
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="ulong" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <returns>If the span is too small to contain a <see cref="ulong" />, return false.</returns>
        [CLSCompliant(false)]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadUInt64BigEndian(ReadOnlySpan<byte> source, out ulong value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out ulong tmp);
                value = ReverseEndianness(tmp);
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="UInt128" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <returns>If the span is too small to contain a <see cref="UInt128" />, return false.</returns>
        [CLSCompliant(false)]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadUInt128BigEndian(ReadOnlySpan<byte> source, out UInt128 value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out UInt128 tmp);
                value = ReverseEndianness(tmp);
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }

        /// <summary>
        /// Reads a <see cref="nuint" /> from the beginning of a read-only span of bytes, as big endian.
        /// </summary>
        /// <returns>If the span is too small to contain a <see cref="nuint" />, return false.</returns>
        [CLSCompliant(false)]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public static bool TryReadUIntPtrBigEndian(ReadOnlySpan<byte> source, out nuint value)
        {
            if (BitConverter.IsLittleEndian)
            {
                bool success = MemoryMarshal.TryRead(source, out nuint tmp);
                value = ReverseEndianness(tmp);
                return success;
            }

            return MemoryMarshal.TryRead(source, out value);
        }
    }
}
