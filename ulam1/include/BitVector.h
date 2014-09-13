/*                                              -*- mode:C++ -*-
  BitVector.h Extended integral type
  Copyright (C) 2014 The Regents of the University of New Mexico.  All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
  USA
*/

/**
  \file BitVector.h Extended integral type
  \author David H. Ackley.
  \date (C) 2014 All rights reserved.
  \lgpl
 */
#ifndef BITVECTOR_H
#define BITVECTOR_H

#include "itype.h"
#include "Util.h"   /* for MakeMaskClip */
#include <climits>  /* for CHAR_BIT */
#include <stdlib.h> /* for abort */

namespace MFM {

  inline s32 _SignExtend32(u32 val, u32 bitwidth) {
    return ((s32)(val<<(32-bitwidth)))>>(32-bitwidth);
  }

  inline s64 _SignExtend64(u64 val, u32 bitwidth) {
    return ((s64)(val<<(64-bitwidth)))>>(64-bitwidth);
  }

  inline u32 _GetNOnes32(u32 bitwidth) {
    return (((u32)1)<<bitwidth)-1;
  }

  inline u64 _GetNOnes64(u32 bitwidth) {
    return (((u64)1)<<bitwidth)-1;
  }

  inline u32 _ShiftToBitNumber32(u32 value, u32 bitpos) {
    return value<<bitpos;
  }

  inline u64 _ShiftToBitNumber64(u32 value, u32 bitpos) {
    return ((u64) value)<<bitpos;
  }

  inline u32 _ShiftFromBitNumber32(u32 value, u32 bitpos) {
    return value>>bitpos;
  }

  inline u64 _ShiftFromBitNumber64(u64 value, u32 bitpos) {
    return value>>bitpos;
  }

  inline u32 _GetMask32(u32 bitpos, u32 bitwidth) {
    return _ShiftToBitNumber32(_GetNOnes32(bitwidth),bitpos);
  }

  inline u64 _GetMask64(u32 bitpos, u32 bitwidth) {
    return _ShiftToBitNumber64(_GetNOnes64(bitwidth),bitpos);
  }

  inline u32  _ExtractField32(u32 val, u32 bitpos,u32 bitwidth) {
    return _ShiftFromBitNumber32(val,bitpos)&_GetNOnes32(bitwidth);
  }

  inline u32  _ExtractUint32(u32 val, u32 bitpos,u32 bitwidth) {
    return _ExtractField32(val,bitpos,bitwidth);
  }

  inline s32  _ExtractSint32(u32 val, u32 bitpos,u32 bitwidth) {
    return _SignExtend32(_ExtractField32(val,bitpos,bitwidth),bitwidth);
  }

  inline u32 _getParity32(u32 v) {
    v ^= v >> 16;
    v ^= v >> 8;
    v ^= v >> 4;
    v &= 0xf;
    return (0x6996 >> v) & 1;
  }

  // v must be <= 0x7fffffff
  inline u32 _getNextPowerOf2(u32 v) {
    v |= v >> 16;
    v |= v >> 8;
    v |= v >> 4;
    v |= v >> 2;
    v |= v >> 1;
    return v+1;
  }

  /**
   * A bit vector with reasonably fast operations
   *
   * BITS should be a multiple of BITS_PER_UNIT (currently
   * 32). Otherwise there will, at least, be some wasted space -- and
   * may be other issues.
   */
  template <u32 B>
  class BitVector
  {
  public:
    enum { BITS = B };

    u32 GetLength() const {
      return BITS;
    }

    typedef u32 BitUnitType;
    static const u32 BITS_PER_UNIT = sizeof(BitUnitType) * CHAR_BIT;

    static const u32 ARRAY_LENGTH = (BITS + BITS_PER_UNIT - 1) / BITS_PER_UNIT;

  private:
    BitUnitType m_bits[ARRAY_LENGTH];

    /**
     * Low-level raw bitvector writing to a single array element.
     * startIdx==0 means the leftmost bit (MSB).  No checking is done:
     * Caller guarantees (1) idx is valid and (2) startIdx + length <=
     * 32
     */
    inline void WriteToUnit(const u32 idx, const u32 startIdx, const u32 length, const u32 value) {
      if (length == 0) return;
      const u32 shift = BITS_PER_UNIT - (startIdx + length);
      u32 mask = MakeMaskClip(length) << shift;
      m_bits[idx] = (m_bits[idx] & ~mask) | ((value << shift) & mask);
    }

    /**
     * Low-level raw bitvector reading from a single array element.
     * startIdx==0 means the leftmost bit (MSB).  No checking is done:
     * Caller guarantees (1) idx is valid and (2) startIdx + length <=
     * 32
     */
    inline u32 ReadFromUnit(const u32 idx, const u32 startIdx, const u32 length) const {
      if (length==0) { return 0; }
      if(idx >= ARRAY_LENGTH) abort();
      const u32 shift = BITS_PER_UNIT - (startIdx + length);
      return (m_bits[idx] >> shift) & MakeMaskClip(length);
    }

  public:

    /**
     * Writes a specified value to a particular bit in this BitVector.
     *
     * @param idx The bit to set, where the MSB is index 0.
     *
     * @param bit The value to set the bit at \c idx to.
     */
    void WriteBit(u32 idx, bool bit);

    /**
     * Reads a specified value from a particular bit in this BitVector.
     *
     * @param idx The bit to read, where the MSB is index 0.
     *
     * @returns The value of the bit at \c idx index.
     */
    bool ReadBit(u32 idx);

#if 0
    /**
     * Constructs a new BitVector. Set parameters of this BitVector
     * using the template arguments. All bits are initialized to \c 0 .
     */
    BitVector();

    /**
     * Copy-constructor for a BitVector. Creates an identical copy of
     * the specified BitVector.
     *
     * @param other The BitVector to copy properties of.
     */
    BitVector(const BitVector & other);

    /**
     * Constructs a BitVector with specified inital value.
     *
     * @param values A pointer to a big-endian array of values to
     *               initialize this BitVector to. This array must
     *               contain at least as many bits as this BitVector
     *               can hold (specified as a template parameter).
     */
    BitVector(const u32 * const values);
#endif

    /**
     * Reads up to 32 bits of a particular section of this BitVector.
     *
     * @param startIdx The index of the first bit to read inside this
     *                 BitVector, where the MSB is indexed at \c 0 .
     *
     * @param length The number of bits to read from this
     *               BitVector. This should be in the range \c [1,32] .
     *
     * @returns The bits read from the particular section of this
     *          BitVector.
     */
    inline u32 Read(const u32 startIdx, const u32 length) const;

    /**
     * Writes up to 32 bits of a specified u32 to a section of this BitVector.
     *
     * @param startIdx The index of the first bit to write inside this
     *                 BitVector, where the MSB is indexed at \c 0 .
     *
     * @param length The number of bits to write to this
     *               BitVector. This should be in the range \c [1,32] .
     *
     * @param value The bits to write to the specified section of this
     *              BitVector.
     */
    void Write(const u32 startIdx, const u32 length, const u32 value);

    /**
     * Sets the bit at a specified index in this BitVector.
     *
     * @param idx The index of the bit to set, where the MSB is
     *        indexed at \c 0 .
     */
    void SetBit(const u32 idx) {
      Write(idx, 1, 1);
    }

    /**
     * Clears the bit at a specified index in this BitVector.
     *
     * @param idx The index of the bit to clear, where the MSB is
     *        indexed at \c 0 .
     */
    void ClearBit(const u32 idx) {
      Write(idx, 1, 0);
    }

    /**
     * Reads a single bit at a specified index from this BitVector.
     *
     * @param idx The index of the bit to read.
     *
     * @returns \c 1 if this bit is set, else \c 0 .
     */
    bool ReadBit(const u32 idx) const {
      return Read(idx, 1) != 0;
    }

    /**
     * Flips the bit at a specified index in this BitVector.
     * (i.e. bv[idx] = !bv[idx])
     *
     * @param idx The index of the bit to toggle, where the MSB is
     *        indexed at \c 0 .
     */
    bool ToggleBit(const u32 idx);

    /**
     * Set a contiguous range of bits, so they all have value 1.
     *
     * @param startIdx The index of the first bit to set inside this
     *                 BitVector, where the MSB is indexed at \c 0 .
     *
     * @param length The number of bits to set in this BitVector.  If
     *               this length would exceed the number of bits in
     *               the BitVector, the excess length is ignored.
     *
     */
    void SetBits(const u32 startIdx, const u32 length) {
      StoreBits(0xffffffff, startIdx, length);
    }

    /**
     * Clear a contiguous range of bits, so they all have value 0.
     *
     * @param startIdx The index of the first bit to clear inside this
     *                 BitVector, where the MSB is indexed at \c 0 .
     *
     * @param length The number of bits to clear in this BitVector.
     *               If this length would exceed the number of bits in
     *               the BitVector, the excess length is ignored.
     *
     */
    void ClearBits(const u32 startIdx, const u32 length)  {
      StoreBits(0, startIdx, length);
    }

    /**
     * Store a bit value (or pattern) into a contiguous range of bits,
     * so they all have that value.
     *
     * @param bits The bits to store.  The supplied bits are tiled
     *                 onto the contiguous range of bits as many times
     *                 as needed to cover the given length, with the
     *                 u32 bits aligned to the underlying u32 storage
     *                 units. To avoid confusion, pass only 0x0 or
     *                 0xffffffff as bits.
     *
     * @param startIdx The index of the first bit to store inside this
     *                 BitVector, where the MSB is indexed at \c 0 .
     *
     * @param length The number of bits to store in this BitVector.
     *               If this length would exceed the number of bits in
     *               the BitVector, the excess length is ignored.
     *
     */
    void StoreBits(const u32 bits, const u32 startIdx, const u32 length) ;

    /**
     * Sets all bits of this BitVector to \c 0 .
     */
    void Clear();

    bool operator==(const BitVector & rhs) const ;

  };
} /* namespace MFM */

#include "BitVector.tcc"

#endif /*BITVECTOR_H*/
