#include <Texture/TexturePCH.h>

#include <Foundation/Math/Color16f.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Texture/Image/Conversions/DXTConversions.h>
#include <Texture/Image/Conversions/PixelConversions.h>
#include <Texture/Image/ImageConversion.h>

#if PL_SSE_LEVEL >= PL_SSE_41 && PL_SIMD_IMPLEMENTATION == PL_SIMD_IMPLEMENTATION_SSE
#  define PL_SUPPORTS_BC4_COMPRESSOR

#  include <emmintrin.h>
#  include <smmintrin.h>
#  include <tmmintrin.h>
#endif

void plDecompressBlockBC1(const plUInt8* pSource, plColorBaseUB* pTarget, bool bForceFourColorMode)
{
  plUInt16 uiColor0 = pSource[0] | (pSource[1] << 8);
  plUInt16 uiColor1 = pSource[2] | (pSource[3] << 8);

  plColorBaseUB colors[4];

  colors[0] = plDecompressB5G6R5(pSource[0] | (pSource[1] << 8));
  colors[1] = plDecompressB5G6R5(pSource[2] | (pSource[3] << 8));

  if (uiColor0 > uiColor1 || bForceFourColorMode)
  {
    colors[2] =
      plColorBaseUB((2 * colors[0].r + colors[1].r + 1) / 3, (2 * colors[0].g + colors[1].g + 1) / 3, (2 * colors[0].b + colors[1].b + 1) / 3, 0xFF);
    colors[3] =
      plColorBaseUB((colors[0].r + 2 * colors[1].r + 1) / 3, (colors[0].g + 2 * colors[1].g + 1) / 3, (colors[0].b + 2 * colors[1].b + 1) / 3, 0xFF);
  }
  else
  {
    colors[2] = plColorBaseUB((colors[0].r + colors[1].r) / 2, (colors[0].g + colors[1].g) / 2, (colors[0].b + colors[1].b) / 2, 0xFF);
    colors[3] = plColorBaseUB(0, 0, 0, 0);
  }

  for (plUInt32 uiByteIdx = 0; uiByteIdx < 4; uiByteIdx++)
  {
    plUInt8 uiIndices = pSource[4 + uiByteIdx];

    pTarget[4 * uiByteIdx + 0] = colors[(uiIndices >> 0) & 0x03];
    pTarget[4 * uiByteIdx + 1] = colors[(uiIndices >> 2) & 0x03];
    pTarget[4 * uiByteIdx + 2] = colors[(uiIndices >> 4) & 0x03];
    pTarget[4 * uiByteIdx + 3] = colors[(uiIndices >> 6) & 0x03];
  }
}

void plDecompressBlockBC4(const plUInt8* pSource, plUInt8* pTarget, plUInt32 uiStride, plUInt8 uiBias)
{
  plUInt8 inputPalette[2];
  inputPalette[0] = pSource[0] + uiBias;
  inputPalette[1] = pSource[1] + uiBias;

  plUInt32 alphas[8];

  plUnpackPaletteBC4(inputPalette[0], inputPalette[1], alphas);

  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(alphas); ++i)
  {
    alphas[i] = plUInt8(alphas[i] - uiBias);
  }

  for (plUInt32 uiTripleIdx = 0; uiTripleIdx < 2; uiTripleIdx++)
  {
    plUInt32 uiIndices = pSource[2 + uiTripleIdx * 3 + 0] << 0 | pSource[2 + uiTripleIdx * 3 + 1] << 8 | pSource[2 + uiTripleIdx * 3 + 2] << 16;

    pTarget[(8 * uiTripleIdx + 0) * uiStride] = plUInt8(alphas[(uiIndices >> 0) & 0x07]);
    pTarget[(8 * uiTripleIdx + 1) * uiStride] = plUInt8(alphas[(uiIndices >> 3) & 0x07]);
    pTarget[(8 * uiTripleIdx + 2) * uiStride] = plUInt8(alphas[(uiIndices >> 6) & 0x07]);
    pTarget[(8 * uiTripleIdx + 3) * uiStride] = plUInt8(alphas[(uiIndices >> 9) & 0x07]);
    pTarget[(8 * uiTripleIdx + 4) * uiStride] = plUInt8(alphas[(uiIndices >> 12) & 0x07]);
    pTarget[(8 * uiTripleIdx + 5) * uiStride] = plUInt8(alphas[(uiIndices >> 15) & 0x07]);
    pTarget[(8 * uiTripleIdx + 6) * uiStride] = plUInt8(alphas[(uiIndices >> 18) & 0x07]);
    pTarget[(8 * uiTripleIdx + 7) * uiStride] = plUInt8(alphas[(uiIndices >> 21) & 0x07]);
  }
}

void plUnpackPaletteBC4(plUInt32 ui0, plUInt32 ui1, plUInt32* pAlphas)
{
  pAlphas[0] = ui0;
  pAlphas[1] = ui1;

  if (ui0 > ui1)
  {
    // Implement division by 7 in range [0, 7 * 255] as (x * 2341) >> 14
    plUInt32 f0 = ui0 * 2341;
    plUInt32 f1 = ui1 * 2341;

    pAlphas[2] = (6 * f0 + 1 * f1 + 3 * 2341) >> 14;
    pAlphas[3] = (5 * f0 + 2 * f1 + 3 * 2341) >> 14;
    pAlphas[4] = (4 * f0 + 3 * f1 + 3 * 2341) >> 14;
    pAlphas[5] = (3 * f0 + 4 * f1 + 3 * 2341) >> 14;
    pAlphas[6] = (2 * f0 + 5 * f1 + 3 * 2341) >> 14;
    pAlphas[7] = (1 * f0 + 6 * f1 + 3 * 2341) >> 14;
  }
  else
  {
    // Implement division by 5 in range [0, 5 * 255] as (x * 1639) >> 13
    plUInt32 f0 = ui0 * 1639;
    plUInt32 f1 = ui1 * 1639;

    pAlphas[2] = (4 * f0 + 1 * f1 + 2 * 1639) >> 13;
    pAlphas[3] = (3 * f0 + 2 * f1 + 2 * 1639) >> 13;
    pAlphas[4] = (2 * f0 + 3 * f1 + 2 * 1639) >> 13;
    pAlphas[5] = (1 * f0 + 4 * f1 + 2 * 1639) >> 13;
    pAlphas[6] = 0x00;
    pAlphas[7] = 0xFF;
  }
}

namespace
{
#if defined(PL_SUPPORTS_BC4_COMPRESSOR)
  plUInt32 findBestPaletteIndexBC4(plInt32 iSourceValue, __m128i p0, __m128i p1)
  {
    __m128i source = _mm_set1_epi32(iSourceValue);

    __m128i e0 = _mm_abs_epi32(_mm_sub_epi32(p0, source));
    __m128i e1 = _mm_abs_epi32(_mm_sub_epi32(p1, source));

    __m128i h0 = _mm_min_epi32(e0, _mm_shuffle_epi32(e0, _MM_SHUFFLE(1, 0, 3, 2)));
    h0 = _mm_min_epi32(h0, _mm_shuffle_epi32(h0, _MM_SHUFFLE(2, 3, 0, 1)));

    __m128i h1 = _mm_min_epi32(e1, _mm_shuffle_epi32(e1, _MM_SHUFFLE(1, 0, 3, 2)));
    h1 = _mm_min_epi32(h1, _mm_shuffle_epi32(h1, _MM_SHUFFLE(2, 3, 0, 1)));

    plUInt32 s0 = _mm_cvtsi128_si32(h0);
    plUInt32 s1 = _mm_cvtsi128_si32(h1);

    uint32_t offset;
    __m128i min, minH;
    if (s0 <= s1)
    {
      min = e0;
      minH = h0;
      offset = 0;
    }
    else
    {
      min = e1;
      minH = h1;
      offset = 4;
    }

    plUInt32 mask = _mm_movemask_ps(_mm_castsi128_ps(_mm_cmpeq_epi32(min, minH)));

    return plMath::FirstBitLow(mask) + offset;
  }

  void packBlockBC4(const plUInt8* pSourceData, plUInt32 ui0, plUInt32 ui1, plUInt8* pTargetData)
  {
    pTargetData[0] = plUInt8(ui0);
    pTargetData[1] = plUInt8(ui1);

    plUInt32 palette[8];
    plUnpackPaletteBC4(ui0, ui1, palette);

    __m128i p0, p1;
    p0 = _mm_loadu_si128(reinterpret_cast<__m128i*>(palette + 0));
    p1 = _mm_loadu_si128(reinterpret_cast<__m128i*>(palette + 4));

    plUInt64 indices = 0;
    for (plUInt32 idx = 0; idx < 16; ++idx)
    {
      indices |= plUInt64(findBestPaletteIndexBC4(pSourceData[idx], p0, p1)) << (3 * idx);
    }

    memcpy(pTargetData + 2, &indices, 6);
  }

  plUInt32 getSquaredErrorBC4_SSE(const plUInt8* pSourceData, const __m128i* pPaletteAndCopy)
  {
    // See getSquaredErrorBC4() for what we want to achieve (sum of lowest squared errors).
    // Instead of converting to 32bit ints and actually computing squares, this function finds lowest absolute differences
    // (between the input data and a color from the palette) for each input.
    // Only then it actually expands them to integers, squares them and adds them together.

    // pal0 contains the palette repeated twice.
    // If we'll perform a vector op between src and pal0, src[0] will correspond to color[0] from the palette, src[1] to color[1], etc.
    // Since the palette is stored twice, src[8] will correspond to color[0] again, etc.
    // Below we generate 7 more rotations of this palette so that each input will correspond to each of 8 colors of palettes.
    const __m128i pal0 = _mm_loadu_si128(pPaletteAndCopy);
    const __m128i src = _mm_loadu_si128((__m128i*)pSourceData);

    auto makeDiff = [&](__m128i pal) {
      // Absolute difference is a difference between max and min of two numbers.
      const __m128i max8 = _mm_max_epu8(src, pal);
      const __m128i min8 = _mm_min_epu8(src, pal);
      return _mm_sub_epi8(max8, min8); // Below we treat the result as unsigned.
    };

    // Unfortunately it can't be a loop, because:
    // 1. last arg in _mm_alignr_epi8 must be a constant
    // 2. rotating the result by 1 in each iteration creates a data dependency
    // 3. VS2015 fails to unroll it properly and generates inefficient code.

    const __m128i diff0 = makeDiff(pal0);
    const __m128i diff1 = makeDiff(_mm_alignr_epi8(pal0, pal0, 1));
    const __m128i diff2 = makeDiff(_mm_alignr_epi8(pal0, pal0, 2));
    const __m128i diff3 = makeDiff(_mm_alignr_epi8(pal0, pal0, 3));
    const __m128i diff4 = makeDiff(_mm_alignr_epi8(pal0, pal0, 4));
    const __m128i diff5 = makeDiff(_mm_alignr_epi8(pal0, pal0, 5));
    const __m128i diff6 = makeDiff(_mm_alignr_epi8(pal0, pal0, 6));
    const __m128i diff7 = makeDiff(_mm_alignr_epi8(pal0, pal0, 7));

    // Now we have absolute differences between each input and each color in the palette.
    // We want to find the lowest one for each input.

    const __m128i minDiff01 = _mm_min_epu8(diff0, diff1);
    const __m128i minDiff23 = _mm_min_epu8(diff2, diff3);
    const __m128i minDiff45 = _mm_min_epu8(diff4, diff5);
    const __m128i minDiff67 = _mm_min_epu8(diff6, diff7);

    const __m128i minDiff = _mm_min_epu8(_mm_min_epu8(minDiff01, minDiff23), _mm_min_epu8(minDiff45, minDiff67));

    // Expands bytes to 32bit integers
    const __m128i zero = _mm_setzero_si128();
    const __m128i diff16Lo = _mm_unpacklo_epi8(minDiff, zero);
    const __m128i diff16Hi = _mm_unpackhi_epi8(minDiff, zero);

    auto square = [](__m128i input) { return _mm_mullo_epi32(input, input); };

    const __m128i square0 = square(_mm_unpacklo_epi16(diff16Lo, zero));
    const __m128i square1 = square(_mm_unpackhi_epi16(diff16Lo, zero));
    const __m128i square2 = square(_mm_unpacklo_epi16(diff16Hi, zero));
    const __m128i square3 = square(_mm_unpackhi_epi16(diff16Hi, zero));

    // Adds all 16 squares together.
    const __m128i sum4 = _mm_add_epi32(_mm_add_epi32(square0, square1), _mm_add_epi32(square2, square3));
    const __m128i sum2 = _mm_hadd_epi32(sum4, sum4);
    return _mm_cvtsi128_si32(_mm_hadd_epi32(sum2, sum2));
  }

  // The order of args is [3], [2], [1], [0]
  static const __m128i div7_a0LoMultiplier = _mm_setr_epi32(5 * 2341, 6 * 2341, 0 * 2341, 7 * 2341);
  static const __m128i div7_a0HiMultiplier = _mm_setr_epi32(1 * 2341, 2 * 2341, 3 * 2341, 4 * 2341);
  static const __m128i div7_a1LoMultiplier = _mm_setr_epi32(2 * 2341, 1 * 2341, 7 * 2341, 0 * 2341);
  static const __m128i div7_a1HiMultiplier = _mm_setr_epi32(6 * 2341, 5 * 2341, 4 * 2341, 3 * 2341);
  static const __m128i div7_correctionAdd = _mm_setr_epi32(3 * 2341, 3 * 2341, 3 * 2341, 3 * 2341);

  static const __m128i div5_a0LoMultiplier = _mm_setr_epi32(3 * 1639, 4 * 1639, 0 * 1639, 5 * 1639);
  static const __m128i div5_a0HiMultiplier = _mm_setr_epi32(0, 0, 1 * 1639, 2 * 1639);
  static const __m128i div5_a1LoMultiplier = _mm_setr_epi32(2 * 1639, 1 * 1639, 5 * 1639, 0);
  static const __m128i div5_a1HiMultiplier = _mm_setr_epi32(0, 0, 4 * 1639, 3 * 1639);
  static const __m128i div5_correctionAdd = _mm_setr_epi32(2 * 1639, 2 * 1639, 2 * 1639, 2 * 1639);
  static const __m128i lastTwoAlphas_0_255 = _mm_setr_epi32(255, 0, 0, 0);

  // Does the same thing as unpackPaletteBC4(), but stores the 8 result numbers twice as bytes
  // (low 8 bytes of alphasAndAlphasCopy will be equal to high 8 bytes)
  // See unpackPaletteBC4 for the explanation regarding magic numbers
  void unpackPaletteBC4AsBytesTwice(plUInt32 ui0, plUInt32 ui1, __m128i* pAlphasAndAlphasCopy)
  {
    const __m128i v0 = _mm_set1_epi32(ui0);
    const __m128i v1 = _mm_set1_epi32(ui1);
    if (ui0 > ui1)
    {
      __m128i sumLo0 = _mm_mullo_epi32(v0, div7_a0LoMultiplier);
      __m128i sumLo1 = _mm_mullo_epi32(v1, div7_a1LoMultiplier);
      __m128i sumHi0 = _mm_mullo_epi32(v0, div7_a0HiMultiplier);
      __m128i sumHi1 = _mm_mullo_epi32(v1, div7_a1HiMultiplier);
      sumLo0 = _mm_add_epi32(div7_correctionAdd, sumLo0);
      sumHi0 = _mm_add_epi32(div7_correctionAdd, sumHi0);
      const __m128i sumLo = _mm_add_epi32(sumLo0, sumLo1);
      const __m128i sumHi = _mm_add_epi32(sumHi0, sumHi1);
      const __m128i resLo = _mm_srli_epi32(sumLo, 14);
      const __m128i resHi = _mm_srli_epi32(sumHi, 14);

      const __m128i res16 = _mm_packs_epi32(resLo, resHi);
      _mm_storeu_si128(pAlphasAndAlphasCopy, _mm_packus_epi16(res16, res16));
    }
    else
    {
      __m128i sumLo0 = _mm_mullo_epi32(v0, div5_a0LoMultiplier);
      __m128i sumHi0 = _mm_mullo_epi32(v0, div5_a0HiMultiplier);
      __m128i sumLo1 = _mm_mullo_epi32(v1, div5_a1LoMultiplier);
      __m128i sumHi1 = _mm_mullo_epi32(v1, div5_a1HiMultiplier);
      sumLo0 = _mm_add_epi32(div5_correctionAdd, sumLo0);
      sumHi0 = _mm_add_epi32(div5_correctionAdd, sumHi0);
      const __m128i sumLo = _mm_add_epi32(sumLo0, sumLo1);
      const __m128i sumHi = _mm_add_epi32(sumHi0, sumHi1);
      const __m128i resHiIncomplete = _mm_srli_epi32(sumHi, 13);
      const __m128i resLo = _mm_srli_epi32(sumLo, 13);
      const __m128i resHi = _mm_add_epi32(resHiIncomplete, lastTwoAlphas_0_255);

      const __m128i res16 = _mm_packs_epi32(resLo, resHi);
      _mm_storeu_si128(pAlphasAndAlphasCopy, _mm_packus_epi16(res16, res16));
    }
  }

  plUInt32 getSquaredErrorBC4_SSE(plUInt32 ui0, plUInt32 ui1, const plUInt8* pSourceData)
  {
    __m128i paletteAndCopy;
    unpackPaletteBC4AsBytesTwice(plUInt8(ui0), plUInt8(ui1), &paletteAndCopy);
    return getSquaredErrorBC4_SSE(pSourceData, &paletteAndCopy);
  }

  void findBestPaletteBC4(const plUInt8* pSourceData, plUInt32& ref_uiBestA0, plUInt32& ref_uiBestA1)
  {
    plInt32 minA = 255;
    plInt32 maxA = 0;

    plInt32 minA_greater8 = 247;
    plInt32 maxA_less248 = 9;

    for (plUInt32 idx = 0; idx < 16; ++idx)
    {
      plUInt32 value = pSourceData[idx];
      minA = plMath::Min<plUInt32>(minA, value);
      maxA = plMath::Max<plUInt32>(maxA, value);

      if (value > 8 && value < 248)
      {
        minA_greater8 = plMath::Min<plUInt32>(minA_greater8, value);
        maxA_less248 = plMath::Max<plUInt32>(maxA_less248, value);
      }
    }

    // Palette covers range perfectly
    if (maxA - minA < 8)
    {
      ref_uiBestA0 = maxA;
      ref_uiBestA1 = minA;
      return;
    }

    plUInt32 bestError = plUInt32(-1);
    ref_uiBestA0 = plUInt32(-1);
    ref_uiBestA1 = plUInt32(-1);

    // Try to find optimal values by searching around min and max
    {
      plInt32 minA0 = plMath::Max(1, maxA - 4);
      plInt32 maxA0 = plMath::Min(256, maxA + 8);
      for (plInt32 a0 = minA0; a0 < maxA0; ++a0)
      {
        plInt32 minA1 = plMath::Max(0, minA - 8);
        plInt32 maxA1 = plMath::Min(a0, minA + 4);
        for (plInt32 a1 = minA1; a1 < maxA1; ++a1)
        {
          plUInt32 error = getSquaredErrorBC4_SSE(a0, a1, pSourceData);

          if (error < bestError)
          {
            bestError = error;
            ref_uiBestA0 = a0;
            ref_uiBestA1 = a1;

            if (error == 0)
            {
              return;
            }
          }
        }
      }
    }

    // If we have any values close to 0 or 255, try the flipped palette versions too, searching around the secondary min/max values
    if (minA < 8 || maxA > 248)
    {
      plInt32 minA1 = maxA_less248 - 4;
      plInt32 maxA1 = maxA_less248 + 8;
      for (plInt32 a1 = minA1; a1 < maxA1; ++a1)
      {
        plInt32 minA0 = minA_greater8 - 8;
        plInt32 maxA0 = plMath::Min(a1, minA_greater8 + 4);
        for (plInt32 a0 = minA0; a0 < maxA0; ++a0)
        {
          plUInt32 error = getSquaredErrorBC4_SSE(a0, a1, pSourceData);

          if (error < bestError)
          {
            bestError = error;
            ref_uiBestA0 = a0;
            ref_uiBestA1 = a1;

            if (error == 0)
            {
              return;
            }
          }
        }
      }
    }
  }
#endif


  // The following BC6 + BC7 decompression implementations were adapted from
  // https://github.com/Microsoft/DirectXTex/blob/master/DirectXTex/BC6HBC7.cpp
  static const plUInt32 s_bc67NumPixelsPerBlock = 16;

  static const plUInt32 s_bc67WeightMax = 64;
  static const plUInt32 s_bc67WeightShift = 6;
  static const plUInt32 s_bc67WeightRound = 32;

  static const int s_bc67InterpolationWeights2[] = {0, 21, 43, 64};
  static const int s_bc67InterpolationWeights3[] = {0, 9, 18, 27, 37, 46, 55, 64};
  static const int s_bc67InterpolationWeights4[] = {0, 4, 9, 13, 17, 21, 26, 30, 34, 38, 43, 47, 51, 55, 60, 64};

  // Partition, Shape, Pixel (index into 4x4 block)
  const plUInt8 s_bc67PartitionTable[3][64][16] = {
    {// 1 Region case has no subsets (all 0)
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},

    {
      // BC6H/BC7 Partition Set for 2 Subsets
      {0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1}, // Shape 0
      {0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1}, // Shape 1
      {0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1}, // Shape 2
      {0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1}, // Shape 3
      {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1}, // Shape 4
      {0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1}, // Shape 5
      {0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1}, // Shape 6
      {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 1}, // Shape 7
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1}, // Shape 8
      {0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // Shape 9
      {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1}, // Shape 10
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1}, // Shape 11
      {0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // Shape 12
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}, // Shape 13
      {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, // Shape 14
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1}, // Shape 15
      {0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1}, // Shape 16
      {0, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}, // Shape 17
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0}, // Shape 18
      {0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0}, // Shape 19
      {0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0}, // Shape 20
      {0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0}, // Shape 21
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0}, // Shape 22
      {0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1}, // Shape 23
      {0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0}, // Shape 24
      {0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 0}, // Shape 25
      {0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0}, // Shape 26
      {0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0}, // Shape 27
      {0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0}, // Shape 28
      {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0}, // Shape 29
      {0, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0}, // Shape 30
      {0, 0, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0}, // Shape 31

      // BC7 Partition Set for 2 Subsets (second-half)
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, // Shape 32
      {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1}, // Shape 33
      {0, 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0}, // Shape 34
      {0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0}, // Shape 35
      {0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0}, // Shape 36
      {0, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0}, // Shape 37
      {0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0, 1}, // Shape 38
      {0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1}, // Shape 39
      {0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0}, // Shape 40
      {0, 0, 0, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 0, 0}, // Shape 41
      {0, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 0}, // Shape 42
      {0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0}, // Shape 43
      {0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0}, // Shape 44
      {0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 1}, // Shape 45
      {0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1}, // Shape 46
      {0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0}, // Shape 47
      {0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0}, // Shape 48
      {0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0}, // Shape 49
      {0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0}, // Shape 50
      {0, 0, 0, 0, 0, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0}, // Shape 51
      {0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1}, // Shape 52
      {0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 1}, // Shape 53
      {0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0}, // Shape 54
      {0, 0, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1, 0}, // Shape 55
      {0, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 1}, // Shape 56
      {0, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1}, // Shape 57
      {0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1}, // Shape 58
      {0, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 1, 1, 1}, // Shape 59
      {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1}, // Shape 60
      {0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0}, // Shape 61
      {0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0}, // Shape 62
      {0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1}  // Shape 63
    },

    {
      // BC7 Partition Set for 3 Subsets
      {0, 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 1, 2, 2, 2, 2}, // Shape 0
      {0, 0, 0, 1, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 2, 1}, // Shape 1
      {0, 0, 0, 0, 2, 0, 0, 1, 2, 2, 1, 1, 2, 2, 1, 1}, // Shape 2
      {0, 2, 2, 2, 0, 0, 2, 2, 0, 0, 1, 1, 0, 1, 1, 1}, // Shape 3
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2}, // Shape 4
      {0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 2, 2}, // Shape 5
      {0, 0, 2, 2, 0, 0, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1}, // Shape 6
      {0, 0, 1, 1, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1}, // Shape 7
      {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2}, // Shape 8
      {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2}, // Shape 9
      {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2}, // Shape 10
      {0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2}, // Shape 11
      {0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 2, 0, 1, 1, 2}, // Shape 12
      {0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 2, 0, 1, 2, 2}, // Shape 13
      {0, 0, 1, 1, 0, 1, 1, 2, 1, 1, 2, 2, 1, 2, 2, 2}, // Shape 14
      {0, 0, 1, 1, 2, 0, 0, 1, 2, 2, 0, 0, 2, 2, 2, 0}, // Shape 15
      {0, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 2, 1, 1, 2, 2}, // Shape 16
      {0, 1, 1, 1, 0, 0, 1, 1, 2, 0, 0, 1, 2, 2, 0, 0}, // Shape 17
      {0, 0, 0, 0, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1, 2, 2}, // Shape 18
      {0, 0, 2, 2, 0, 0, 2, 2, 0, 0, 2, 2, 1, 1, 1, 1}, // Shape 19
      {0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2, 0, 2, 2, 2}, // Shape 20
      {0, 0, 0, 1, 0, 0, 0, 1, 2, 2, 2, 1, 2, 2, 2, 1}, // Shape 21
      {0, 0, 0, 0, 0, 0, 1, 1, 0, 1, 2, 2, 0, 1, 2, 2}, // Shape 22
      {0, 0, 0, 0, 1, 1, 0, 0, 2, 2, 1, 0, 2, 2, 1, 0}, // Shape 23
      {0, 1, 2, 2, 0, 1, 2, 2, 0, 0, 1, 1, 0, 0, 0, 0}, // Shape 24
      {0, 0, 1, 2, 0, 0, 1, 2, 1, 1, 2, 2, 2, 2, 2, 2}, // Shape 25
      {0, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 1, 0, 1, 1, 0}, // Shape 26
      {0, 0, 0, 0, 0, 1, 1, 0, 1, 2, 2, 1, 1, 2, 2, 1}, // Shape 27
      {0, 0, 2, 2, 1, 1, 0, 2, 1, 1, 0, 2, 0, 0, 2, 2}, // Shape 28
      {0, 1, 1, 0, 0, 1, 1, 0, 2, 0, 0, 2, 2, 2, 2, 2}, // Shape 29
      {0, 0, 1, 1, 0, 1, 2, 2, 0, 1, 2, 2, 0, 0, 1, 1}, // Shape 30
      {0, 0, 0, 0, 2, 0, 0, 0, 2, 2, 1, 1, 2, 2, 2, 1}, // Shape 31
      {0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 2, 2, 2}, // Shape 32
      {0, 2, 2, 2, 0, 0, 2, 2, 0, 0, 1, 2, 0, 0, 1, 1}, // Shape 33
      {0, 0, 1, 1, 0, 0, 1, 2, 0, 0, 2, 2, 0, 2, 2, 2}, // Shape 34
      {0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0, 0, 1, 2, 0}, // Shape 35
      {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0}, // Shape 36
      {0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0, 1, 2, 0}, // Shape 37
      {0, 1, 2, 0, 2, 0, 1, 2, 1, 2, 0, 1, 0, 1, 2, 0}, // Shape 38
      {0, 0, 1, 1, 2, 2, 0, 0, 1, 1, 2, 2, 0, 0, 1, 1}, // Shape 39
      {0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 0, 0, 0, 0, 1, 1}, // Shape 40
      {0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2}, // Shape 41
      {0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1}, // Shape 42
      {0, 0, 2, 2, 1, 1, 2, 2, 0, 0, 2, 2, 1, 1, 2, 2}, // Shape 43
      {0, 0, 2, 2, 0, 0, 1, 1, 0, 0, 2, 2, 0, 0, 1, 1}, // Shape 44
      {0, 2, 2, 0, 1, 2, 2, 1, 0, 2, 2, 0, 1, 2, 2, 1}, // Shape 45
      {0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 1, 0, 1}, // Shape 46
      {0, 0, 0, 0, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 1}, // Shape 47
      {0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 2, 2, 2, 2}, // Shape 48
      {0, 2, 2, 2, 0, 1, 1, 1, 0, 2, 2, 2, 0, 1, 1, 1}, // Shape 49
      {0, 0, 0, 2, 1, 1, 1, 2, 0, 0, 0, 2, 1, 1, 1, 2}, // Shape 50
      {0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 1, 2, 2, 1, 1, 2}, // Shape 51
      {0, 2, 2, 2, 0, 1, 1, 1, 0, 1, 1, 1, 0, 2, 2, 2}, // Shape 52
      {0, 0, 0, 2, 1, 1, 1, 2, 1, 1, 1, 2, 0, 0, 0, 2}, // Shape 53
      {0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 2, 2}, // Shape 54
      {0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2, 2, 1, 1, 2}, // Shape 55
      {0, 1, 1, 0, 0, 1, 1, 0, 2, 2, 2, 2, 2, 2, 2, 2}, // Shape 56
      {0, 0, 2, 2, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 2, 2}, // Shape 57
      {0, 0, 2, 2, 1, 1, 2, 2, 1, 1, 2, 2, 0, 0, 2, 2}, // Shape 58
      {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 1, 1, 2}, // Shape 59
      {0, 0, 0, 2, 0, 0, 0, 1, 0, 0, 0, 2, 0, 0, 0, 1}, // Shape 60
      {0, 2, 2, 2, 1, 2, 2, 2, 0, 2, 2, 2, 1, 2, 2, 2}, // Shape 61
      {0, 1, 0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2}, // Shape 62
      {0, 1, 1, 1, 2, 0, 1, 1, 2, 2, 0, 1, 2, 2, 2, 0}  // Shape 63
    }};

  // Partition, Shape, Fixup
  static const plUInt8 s_bc67FixUp[3][64][3] = {
    {// No fix-ups for 1st subset for BC6H or BC7
      {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
      {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
      {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
      {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0},
      {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}, {0, 0, 0}},

    {// BC6H/BC7 Partition Set Fixups for 2 Subsets
      {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0},
      {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 2, 0}, {0, 8, 0}, {0, 2, 0}, {0, 2, 0}, {0, 8, 0}, {0, 8, 0}, {0, 15, 0},
      {0, 2, 0}, {0, 8, 0}, {0, 2, 0}, {0, 2, 0}, {0, 8, 0}, {0, 8, 0}, {0, 2, 0}, {0, 2, 0},

      // BC7 Partition Set Fixups for 2 Subsets (second-half)
      {0, 15, 0}, {0, 15, 0}, {0, 6, 0}, {0, 8, 0}, {0, 2, 0}, {0, 8, 0}, {0, 15, 0}, {0, 15, 0}, {0, 2, 0}, {0, 8, 0}, {0, 2, 0}, {0, 2, 0},
      {0, 2, 0}, {0, 15, 0}, {0, 15, 0}, {0, 6, 0}, {0, 6, 0}, {0, 2, 0}, {0, 6, 0}, {0, 8, 0}, {0, 15, 0}, {0, 15, 0}, {0, 2, 0}, {0, 2, 0},
      {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 15, 0}, {0, 2, 0}, {0, 2, 0}, {0, 15, 0}},

    {// BC7 Partition Set Fixups for 3 Subsets
      {0, 3, 15}, {0, 3, 8}, {0, 15, 8}, {0, 15, 3}, {0, 8, 15}, {0, 3, 15}, {0, 15, 3}, {0, 15, 8}, {0, 8, 15}, {0, 8, 15}, {0, 6, 15}, {0, 6, 15},
      {0, 6, 15}, {0, 5, 15}, {0, 3, 15}, {0, 3, 8}, {0, 3, 15}, {0, 3, 8}, {0, 8, 15}, {0, 15, 3}, {0, 3, 15}, {0, 3, 8}, {0, 6, 15}, {0, 10, 8},
      {0, 5, 3}, {0, 8, 15}, {0, 8, 6}, {0, 6, 10}, {0, 8, 15}, {0, 5, 15}, {0, 15, 10}, {0, 15, 8}, {0, 8, 15}, {0, 15, 3}, {0, 3, 15}, {0, 5, 10},
      {0, 6, 10}, {0, 10, 8}, {0, 8, 9}, {0, 15, 10}, {0, 15, 6}, {0, 3, 15}, {0, 15, 8}, {0, 5, 15}, {0, 15, 3}, {0, 15, 6}, {0, 15, 6}, {0, 15, 8},
      {0, 3, 15}, {0, 15, 3}, {0, 5, 15}, {0, 5, 15}, {0, 5, 15}, {0, 8, 15}, {0, 5, 15}, {0, 10, 15}, {0, 5, 15}, {0, 10, 15}, {0, 8, 15},
      {0, 13, 15}, {0, 15, 3}, {0, 12, 15}, {0, 3, 15}, {0, 3, 8}}};

  static const plUInt32 s_bc6MaxRegions = 2;
  static const plUInt32 s_bc6MaxIndices = 16;

  enum BC6EField : plUInt8
  {
    NA, // N/A
    M,  // Mode
    D,  // Shape
    RW,
    RX,
    RY,
    RZ,
    GW,
    GX,
    GY,
    GZ,
    BW,
    BX,
    BY,
    BZ,
  };

  struct BC6ModeDescriptor
  {
    BC6EField field;
    plUInt8 bit;
  };

  static const BC6ModeDescriptor s_bc6ModeDescs[][82] = {{
                                                           // Mode 1 (0x00) - 10 5 5 5
                                                           {M, 0},
                                                           {M, 1},
                                                           {GY, 4},
                                                           {BY, 4},
                                                           {BZ, 4},
                                                           {RW, 0},
                                                           {RW, 1},
                                                           {RW, 2},
                                                           {RW, 3},
                                                           {RW, 4},
                                                           {RW, 5},
                                                           {RW, 6},
                                                           {RW, 7},
                                                           {RW, 8},
                                                           {RW, 9},
                                                           {GW, 0},
                                                           {GW, 1},
                                                           {GW, 2},
                                                           {GW, 3},
                                                           {GW, 4},
                                                           {GW, 5},
                                                           {GW, 6},
                                                           {GW, 7},
                                                           {GW, 8},
                                                           {GW, 9},
                                                           {BW, 0},
                                                           {BW, 1},
                                                           {BW, 2},
                                                           {BW, 3},
                                                           {BW, 4},
                                                           {BW, 5},
                                                           {BW, 6},
                                                           {BW, 7},
                                                           {BW, 8},
                                                           {BW, 9},
                                                           {RX, 0},
                                                           {RX, 1},
                                                           {RX, 2},
                                                           {RX, 3},
                                                           {RX, 4},
                                                           {GZ, 4},
                                                           {GY, 0},
                                                           {GY, 1},
                                                           {GY, 2},
                                                           {GY, 3},
                                                           {GX, 0},
                                                           {GX, 1},
                                                           {GX, 2},
                                                           {GX, 3},
                                                           {GX, 4},
                                                           {BZ, 0},
                                                           {GZ, 0},
                                                           {GZ, 1},
                                                           {GZ, 2},
                                                           {GZ, 3},
                                                           {BX, 0},
                                                           {BX, 1},
                                                           {BX, 2},
                                                           {BX, 3},
                                                           {BX, 4},
                                                           {BZ, 1},
                                                           {BY, 0},
                                                           {BY, 1},
                                                           {BY, 2},
                                                           {BY, 3},
                                                           {RY, 0},
                                                           {RY, 1},
                                                           {RY, 2},
                                                           {RY, 3},
                                                           {RY, 4},
                                                           {BZ, 2},
                                                           {RZ, 0},
                                                           {RZ, 1},
                                                           {RZ, 2},
                                                           {RZ, 3},
                                                           {RZ, 4},
                                                           {BZ, 3},
                                                           {D, 0},
                                                           {D, 1},
                                                           {D, 2},
                                                           {D, 3},
                                                           {D, 4},
                                                         },

    {
      // Mode 2 (0x01) - 7 6 6 6
      {M, 0},
      {M, 1},
      {GY, 5},
      {GZ, 4},
      {GZ, 5},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {BZ, 0},
      {BZ, 1},
      {BY, 4},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {BY, 5},
      {BZ, 2},
      {GY, 4},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BZ, 3},
      {BZ, 5},
      {BZ, 4},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RX, 4},
      {RX, 5},
      {GY, 0},
      {GY, 1},
      {GY, 2},
      {GY, 3},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GX, 4},
      {GX, 5},
      {GZ, 0},
      {GZ, 1},
      {GZ, 2},
      {GZ, 3},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BX, 4},
      {BX, 5},
      {BY, 0},
      {BY, 1},
      {BY, 2},
      {BY, 3},
      {RY, 0},
      {RY, 1},
      {RY, 2},
      {RY, 3},
      {RY, 4},
      {RY, 5},
      {RZ, 0},
      {RZ, 1},
      {RZ, 2},
      {RZ, 3},
      {RZ, 4},
      {RZ, 5},
      {D, 0},
      {D, 1},
      {D, 2},
      {D, 3},
      {D, 4},
    },

    {
      // Mode 3 (0x02) - 11 5 4 4
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {RW, 8},
      {RW, 9},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {GW, 8},
      {GW, 9},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {BW, 8},
      {BW, 9},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RX, 4},
      {RW, 10},
      {GY, 0},
      {GY, 1},
      {GY, 2},
      {GY, 3},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GW, 10},
      {BZ, 0},
      {GZ, 0},
      {GZ, 1},
      {GZ, 2},
      {GZ, 3},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BW, 10},
      {BZ, 1},
      {BY, 0},
      {BY, 1},
      {BY, 2},
      {BY, 3},
      {RY, 0},
      {RY, 1},
      {RY, 2},
      {RY, 3},
      {RY, 4},
      {BZ, 2},
      {RZ, 0},
      {RZ, 1},
      {RZ, 2},
      {RZ, 3},
      {RZ, 4},
      {BZ, 3},
      {D, 0},
      {D, 1},
      {D, 2},
      {D, 3},
      {D, 4},
    },

    {
      // Mode 4 (0x06) - 11 4 5 4
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {RW, 8},
      {RW, 9},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {GW, 8},
      {GW, 9},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {BW, 8},
      {BW, 9},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RW, 10},
      {GZ, 4},
      {GY, 0},
      {GY, 1},
      {GY, 2},
      {GY, 3},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GX, 4},
      {GW, 10},
      {GZ, 0},
      {GZ, 1},
      {GZ, 2},
      {GZ, 3},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BW, 10},
      {BZ, 1},
      {BY, 0},
      {BY, 1},
      {BY, 2},
      {BY, 3},
      {RY, 0},
      {RY, 1},
      {RY, 2},
      {RY, 3},
      {BZ, 0},
      {BZ, 2},
      {RZ, 0},
      {RZ, 1},
      {RZ, 2},
      {RZ, 3},
      {GY, 4},
      {BZ, 3},
      {D, 0},
      {D, 1},
      {D, 2},
      {D, 3},
      {D, 4},
    },

    {
      // Mode 5 (0x0a) - 11 4 4 5
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {RW, 8},
      {RW, 9},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {GW, 8},
      {GW, 9},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {BW, 8},
      {BW, 9},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RW, 10},
      {BY, 4},
      {GY, 0},
      {GY, 1},
      {GY, 2},
      {GY, 3},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GW, 10},
      {BZ, 0},
      {GZ, 0},
      {GZ, 1},
      {GZ, 2},
      {GZ, 3},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BX, 4},
      {BW, 10},
      {BY, 0},
      {BY, 1},
      {BY, 2},
      {BY, 3},
      {RY, 0},
      {RY, 1},
      {RY, 2},
      {RY, 3},
      {BZ, 1},
      {BZ, 2},
      {RZ, 0},
      {RZ, 1},
      {RZ, 2},
      {RZ, 3},
      {BZ, 4},
      {BZ, 3},
      {D, 0},
      {D, 1},
      {D, 2},
      {D, 3},
      {D, 4},
    },

    {
      // Mode 6 (0x0e) - 9 5 5 5
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {RW, 8},
      {BY, 4},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {GW, 8},
      {GY, 4},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {BW, 8},
      {BZ, 4},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RX, 4},
      {GZ, 4},
      {GY, 0},
      {GY, 1},
      {GY, 2},
      {GY, 3},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GX, 4},
      {BZ, 0},
      {GZ, 0},
      {GZ, 1},
      {GZ, 2},
      {GZ, 3},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BX, 4},
      {BZ, 1},
      {BY, 0},
      {BY, 1},
      {BY, 2},
      {BY, 3},
      {RY, 0},
      {RY, 1},
      {RY, 2},
      {RY, 3},
      {RY, 4},
      {BZ, 2},
      {RZ, 0},
      {RZ, 1},
      {RZ, 2},
      {RZ, 3},
      {RZ, 4},
      {BZ, 3},
      {D, 0},
      {D, 1},
      {D, 2},
      {D, 3},
      {D, 4},
    },

    {
      // Mode 7 (0x12) - 8 6 5 5
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {GZ, 4},
      {BY, 4},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {BZ, 2},
      {GY, 4},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {BZ, 3},
      {BZ, 4},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RX, 4},
      {RX, 5},
      {GY, 0},
      {GY, 1},
      {GY, 2},
      {GY, 3},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GX, 4},
      {BZ, 0},
      {GZ, 0},
      {GZ, 1},
      {GZ, 2},
      {GZ, 3},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BX, 4},
      {BZ, 1},
      {BY, 0},
      {BY, 1},
      {BY, 2},
      {BY, 3},
      {RY, 0},
      {RY, 1},
      {RY, 2},
      {RY, 3},
      {RY, 4},
      {RY, 5},
      {RZ, 0},
      {RZ, 1},
      {RZ, 2},
      {RZ, 3},
      {RZ, 4},
      {RZ, 5},
      {D, 0},
      {D, 1},
      {D, 2},
      {D, 3},
      {D, 4},
    },

    {
      // Mode 8 (0x16) - 8 5 6 5
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {BZ, 0},
      {BY, 4},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {GY, 5},
      {GY, 4},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {GZ, 5},
      {BZ, 4},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RX, 4},
      {GZ, 4},
      {GY, 0},
      {GY, 1},
      {GY, 2},
      {GY, 3},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GX, 4},
      {GX, 5},
      {GZ, 0},
      {GZ, 1},
      {GZ, 2},
      {GZ, 3},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BX, 4},
      {BZ, 1},
      {BY, 0},
      {BY, 1},
      {BY, 2},
      {BY, 3},
      {RY, 0},
      {RY, 1},
      {RY, 2},
      {RY, 3},
      {RY, 4},
      {BZ, 2},
      {RZ, 0},
      {RZ, 1},
      {RZ, 2},
      {RZ, 3},
      {RZ, 4},
      {BZ, 3},
      {D, 0},
      {D, 1},
      {D, 2},
      {D, 3},
      {D, 4},
    },

    {
      // Mode 9 (0x1a) - 8 5 5 6
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {BZ, 1},
      {BY, 4},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {BY, 5},
      {GY, 4},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {BZ, 5},
      {BZ, 4},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RX, 4},
      {GZ, 4},
      {GY, 0},
      {GY, 1},
      {GY, 2},
      {GY, 3},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GX, 4},
      {BZ, 0},
      {GZ, 0},
      {GZ, 1},
      {GZ, 2},
      {GZ, 3},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BX, 4},
      {BX, 5},
      {BY, 0},
      {BY, 1},
      {BY, 2},
      {BY, 3},
      {RY, 0},
      {RY, 1},
      {RY, 2},
      {RY, 3},
      {RY, 4},
      {BZ, 2},
      {RZ, 0},
      {RZ, 1},
      {RZ, 2},
      {RZ, 3},
      {RZ, 4},
      {BZ, 3},
      {D, 0},
      {D, 1},
      {D, 2},
      {D, 3},
      {D, 4},
    },

    {
      // Mode 10 (0x1e) - 6 6 6 6
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {GZ, 4},
      {BZ, 0},
      {BZ, 1},
      {BY, 4},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GY, 5},
      {BY, 5},
      {BZ, 2},
      {GY, 4},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {GZ, 5},
      {BZ, 3},
      {BZ, 5},
      {BZ, 4},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RX, 4},
      {RX, 5},
      {GY, 0},
      {GY, 1},
      {GY, 2},
      {GY, 3},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GX, 4},
      {GX, 5},
      {GZ, 0},
      {GZ, 1},
      {GZ, 2},
      {GZ, 3},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BX, 4},
      {BX, 5},
      {BY, 0},
      {BY, 1},
      {BY, 2},
      {BY, 3},
      {RY, 0},
      {RY, 1},
      {RY, 2},
      {RY, 3},
      {RY, 4},
      {RY, 5},
      {RZ, 0},
      {RZ, 1},
      {RZ, 2},
      {RZ, 3},
      {RZ, 4},
      {RZ, 5},
      {D, 0},
      {D, 1},
      {D, 2},
      {D, 3},
      {D, 4},
    },

    {
      // Mode 11 (0x03) - 10 10
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {RW, 8},
      {RW, 9},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {GW, 8},
      {GW, 9},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {BW, 8},
      {BW, 9},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RX, 4},
      {RX, 5},
      {RX, 6},
      {RX, 7},
      {RX, 8},
      {RX, 9},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GX, 4},
      {GX, 5},
      {GX, 6},
      {GX, 7},
      {GX, 8},
      {GX, 9},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BX, 4},
      {BX, 5},
      {BX, 6},
      {BX, 7},
      {BX, 8},
      {BX, 9},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
    },

    {
      // Mode 12 (0x07) - 11 9
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {RW, 8},
      {RW, 9},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {GW, 8},
      {GW, 9},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {BW, 8},
      {BW, 9},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RX, 4},
      {RX, 5},
      {RX, 6},
      {RX, 7},
      {RX, 8},
      {RW, 10},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GX, 4},
      {GX, 5},
      {GX, 6},
      {GX, 7},
      {GX, 8},
      {GW, 10},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BX, 4},
      {BX, 5},
      {BX, 6},
      {BX, 7},
      {BX, 8},
      {BW, 10},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
    },

    {
      // Mode 13 (0x0b) - 12 8
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {RW, 8},
      {RW, 9},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {GW, 8},
      {GW, 9},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {BW, 8},
      {BW, 9},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RX, 4},
      {RX, 5},
      {RX, 6},
      {RX, 7},
      {RW, 11},
      {RW, 10},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GX, 4},
      {GX, 5},
      {GX, 6},
      {GX, 7},
      {GW, 11},
      {GW, 10},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BX, 4},
      {BX, 5},
      {BX, 6},
      {BX, 7},
      {BW, 11},
      {BW, 10},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
    },

    {
      // Mode 14 (0x0f) - 16 4
      {M, 0},
      {M, 1},
      {M, 2},
      {M, 3},
      {M, 4},
      {RW, 0},
      {RW, 1},
      {RW, 2},
      {RW, 3},
      {RW, 4},
      {RW, 5},
      {RW, 6},
      {RW, 7},
      {RW, 8},
      {RW, 9},
      {GW, 0},
      {GW, 1},
      {GW, 2},
      {GW, 3},
      {GW, 4},
      {GW, 5},
      {GW, 6},
      {GW, 7},
      {GW, 8},
      {GW, 9},
      {BW, 0},
      {BW, 1},
      {BW, 2},
      {BW, 3},
      {BW, 4},
      {BW, 5},
      {BW, 6},
      {BW, 7},
      {BW, 8},
      {BW, 9},
      {RX, 0},
      {RX, 1},
      {RX, 2},
      {RX, 3},
      {RW, 15},
      {RW, 14},
      {RW, 13},
      {RW, 12},
      {RW, 11},
      {RW, 10},
      {GX, 0},
      {GX, 1},
      {GX, 2},
      {GX, 3},
      {GW, 15},
      {GW, 14},
      {GW, 13},
      {GW, 12},
      {GW, 11},
      {GW, 10},
      {BX, 0},
      {BX, 1},
      {BX, 2},
      {BX, 3},
      {BW, 15},
      {BW, 14},
      {BW, 13},
      {BW, 12},
      {BW, 11},
      {BW, 10},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
      {NA, 0},
    }};

  struct BC6ModeInfo
  {
    plUInt8 mode;
    plUInt8 partitions;
    bool transformed;
    plUInt8 indexPrec;
    plColorBaseUB rgbaPrec[s_bc6MaxRegions][2];
  };

  static const BC6ModeInfo s_bc6ModeInfos[] = {
    {0x00, 1, true, 3, {{plColorBaseUB(10, 10, 10, 0), plColorBaseUB(5, 5, 5, 0)}, {plColorBaseUB(5, 5, 5, 0), plColorBaseUB(5, 5, 5, 0)}}}, // Mode 1
    {0x01, 1, true, 3, {{plColorBaseUB(7, 7, 7, 0), plColorBaseUB(6, 6, 6, 0)}, {plColorBaseUB(6, 6, 6, 0), plColorBaseUB(6, 6, 6, 0)}}},    // Mode 2
    {0x02, 1, true, 3, {{plColorBaseUB(11, 11, 11, 0), plColorBaseUB(5, 4, 4, 0)}, {plColorBaseUB(5, 4, 4, 0), plColorBaseUB(5, 4, 4, 0)}}}, // Mode 3
    {0x06, 1, true, 3, {{plColorBaseUB(11, 11, 11, 0), plColorBaseUB(4, 5, 4, 0)}, {plColorBaseUB(4, 5, 4, 0), plColorBaseUB(4, 5, 4, 0)}}}, // Mode 4
    {0x0a, 1, true, 3, {{plColorBaseUB(11, 11, 11, 0), plColorBaseUB(4, 4, 5, 0)}, {plColorBaseUB(4, 4, 5, 0), plColorBaseUB(4, 4, 5, 0)}}}, // Mode 5
    {0x0e, 1, true, 3, {{plColorBaseUB(9, 9, 9, 0), plColorBaseUB(5, 5, 5, 0)}, {plColorBaseUB(5, 5, 5, 0), plColorBaseUB(5, 5, 5, 0)}}},    // Mode 6
    {0x12, 1, true, 3, {{plColorBaseUB(8, 8, 8, 0), plColorBaseUB(6, 5, 5, 0)}, {plColorBaseUB(6, 5, 5, 0), plColorBaseUB(6, 5, 5, 0)}}},    // Mode 7
    {0x16, 1, true, 3, {{plColorBaseUB(8, 8, 8, 0), plColorBaseUB(5, 6, 5, 0)}, {plColorBaseUB(5, 6, 5, 0), plColorBaseUB(5, 6, 5, 0)}}},    // Mode 8
    {0x1a, 1, true, 3, {{plColorBaseUB(8, 8, 8, 0), plColorBaseUB(5, 5, 6, 0)}, {plColorBaseUB(5, 5, 6, 0), plColorBaseUB(5, 5, 6, 0)}}},    // Mode 9
    {0x1e, 1, false, 3, {{plColorBaseUB(6, 6, 6, 0), plColorBaseUB(6, 6, 6, 0)}, {plColorBaseUB(6, 6, 6, 0), plColorBaseUB(6, 6, 6, 0)}}}, // Mode 10
    {0x03, 0, false, 4,
      {{plColorBaseUB(10, 10, 10, 0), plColorBaseUB(10, 10, 10, 0)}, {plColorBaseUB(0, 0, 0, 0), plColorBaseUB(0, 0, 0, 0)}}}, // Mode 11
    {0x07, 0, true, 4,
      {{plColorBaseUB(11, 11, 11, 0), plColorBaseUB(9, 9, 9, 0)}, {plColorBaseUB(0, 0, 0, 0), plColorBaseUB(0, 0, 0, 0)}}}, // Mode 12
    {0x0b, 0, true, 4,
      {{plColorBaseUB(12, 12, 12, 0), plColorBaseUB(8, 8, 8, 0)}, {plColorBaseUB(0, 0, 0, 0), plColorBaseUB(0, 0, 0, 0)}}}, // Mode 13
    {0x0f, 0, true, 4,
      {{plColorBaseUB(16, 16, 16, 0), plColorBaseUB(4, 4, 4, 0)}, {plColorBaseUB(0, 0, 0, 0), plColorBaseUB(0, 0, 0, 0)}}}, // Mode 14
  };

  static const plInt32 s_bc6ModeToInfo[] = {
    0,  // Mode 1   - 0x00
    1,  // Mode 2   - 0x01
    2,  // Mode 3   - 0x02
    10, // Mode 11  - 0x03
    -1, // Invalid  - 0x04
    -1, // Invalid  - 0x05
    3,  // Mode 4   - 0x06
    11, // Mode 12  - 0x07
    -1, // Invalid  - 0x08
    -1, // Invalid  - 0x09
    4,  // Mode 5   - 0x0a
    12, // Mode 13  - 0x0b
    -1, // Invalid  - 0x0c
    -1, // Invalid  - 0x0d
    5,  // Mode 6   - 0x0e
    13, // Mode 14  - 0x0f
    -1, // Invalid  - 0x10
    -1, // Invalid  - 0x11
    6,  // Mode 7   - 0x12
    -1, // Reserved - 0x13
    -1, // Invalid  - 0x14
    -1, // Invalid  - 0x15
    7,  // Mode 8   - 0x16
    -1, // Reserved - 0x17
    -1, // Invalid  - 0x18
    -1, // Invalid  - 0x19
    8,  // Mode 9   - 0x1a
    -1, // Reserved - 0x1b
    -1, // Invalid  - 0x1c
    -1, // Invalid  - 0x1d
    9,  // Mode 10  - 0x1e
    -1, // Resreved - 0x1f
  };

  static const plUInt32 s_bc7MaxRegions = 3;
  static const plUInt32 s_bc7MaxIndices = 16;
  static const plUInt32 s_bc7NumChannels = 4;
  static const plUInt32 s_bc7MaxShapes = 64;

  struct BC7ModeInfo
  {
    plUInt8 partitions;
    plUInt8 partitionBits;
    plUInt8 pBits;
    plUInt8 rotationBits;
    plUInt8 indexModeBits;
    plUInt8 indexPrec;
    plUInt8 indexPrec2;
    plColorBaseUB rgbaPrec;
    plColorBaseUB rgbaPrecWithP;
  };

  static const BC7ModeInfo s_bc7ModeInfos[] = {
    // Mode 0: Color only, 3 Subsets, RGBP 4441 (unique P-bit), 3-bit indecies, 16 partitions
    {2, 4, 6, 0, 0, 3, 0, plColorBaseUB(4, 4, 4, 0), plColorBaseUB(5, 5, 5, 0)},
    // Mode 1: Color only, 2 Subsets, RGBP 6661 (shared P-bit), 3-bit indecies, 64 partitions
    {1, 6, 2, 0, 0, 3, 0, plColorBaseUB(6, 6, 6, 0), plColorBaseUB(7, 7, 7, 0)},
    // Mode 2: Color only, 3 Subsets, RGB 555, 2-bit indecies, 64 partitions
    {2, 6, 0, 0, 0, 2, 0, plColorBaseUB(5, 5, 5, 0), plColorBaseUB(5, 5, 5, 0)},
    // Mode 3: Color only, 2 Subsets, RGBP 7771 (unique P-bit), 2-bits indecies, 64 partitions
    {1, 6, 4, 0, 0, 2, 0, plColorBaseUB(7, 7, 7, 0), plColorBaseUB(8, 8, 8, 0)},
    // Mode 4: Color w/ Separate Alpha, 1 Subset, RGB 555, A6, 16x2/16x3-bit indices, 2-bit rotation, 1-bit index selector
    {0, 0, 0, 2, 1, 2, 3, plColorBaseUB(5, 5, 5, 6), plColorBaseUB(5, 5, 5, 6)},
    // Mode 5: Color w/ Separate Alpha, 1 Subset, RGB 777, A8, 16x2/16x2-bit indices, 2-bit rotation
    {0, 0, 0, 2, 0, 2, 2, plColorBaseUB(7, 7, 7, 8), plColorBaseUB(7, 7, 7, 8)},
    // Mode 6: Color+Alpha, 1 Subset, RGBAP 77771 (unique P-bit), 16x4-bit indecies
    {0, 0, 2, 0, 0, 4, 0, plColorBaseUB(7, 7, 7, 7), plColorBaseUB(8, 8, 8, 8)},
    // Mode 7: Color+Alpha, 2 Subsets, RGBAP 55551 (unique P-bit), 2-bit indices, 64 partitions
    {1, 6, 4, 0, 0, 2, 0, plColorBaseUB(5, 5, 5, 5), plColorBaseUB(6, 6, 6, 6)}};

  plUInt8 getBit(const plUInt8* pBits, plUInt32& ref_uiStartBit)
  {
    PL_ASSERT_DEV(ref_uiStartBit < 128, "");

    plUInt32 index = ref_uiStartBit >> 3;
    plUInt8 ret = (pBits[index] >> (ref_uiStartBit - (index << 3))) & 0x01;
    ++ref_uiStartBit;
    return ret;
  }

  plUInt8 getBits(const plUInt8* pBits, plUInt32& ref_uiStartBit, plUInt32 uiNumBits)
  {
    if (uiNumBits == 0)
      return 0;
    PL_ASSERT_DEV(ref_uiStartBit + uiNumBits <= 128 && uiNumBits <= 8, "");

    plUInt8 ret;
    plUInt32 index = ref_uiStartBit >> 3;
    plUInt32 base = ref_uiStartBit - (index << 3);
    if (base + uiNumBits > 8)
    {
      plUInt32 firstIndexBits = 8 - base;
      plUInt32 nextIndexBits = uiNumBits - firstIndexBits;
      ret = (pBits[index] >> base) | ((pBits[index + 1] & ((1 << nextIndexBits) - 1)) << firstIndexBits);
    }
    else
    {
      ret = (pBits[index] >> base) & ((1 << uiNumBits) - 1);
    }
    PL_ASSERT_DEV(ret < (1 << uiNumBits), "");
    ref_uiStartBit += uiNumBits;
    return ret;
  }

  inline bool isFixUpOffset(plUInt32 uiPartitions, plUInt32 uiShape, plUInt32 uiOffset)
  {
    PL_ASSERT_DEV(uiPartitions < 3 && uiShape < 64 && uiOffset < 16, "");

    for (plUInt32 p = 0; p <= uiPartitions; ++p)
    {
      if (uiOffset == s_bc67FixUp[uiPartitions][uiShape][p])
      {
        return true;
      }
    }
    return false;
  }

  void interpolateRGB(const plColorBaseUB& c0, const plColorBaseUB& c1, plUInt32 uiWc, plUInt32 uiWcprec, plColorBaseUB& ref_out)
  {
    const int* weights = nullptr;
    switch (uiWcprec)
    {
      case 2:
        weights = s_bc67InterpolationWeights2;
        PL_ASSERT_DEV(uiWc < 4, "");

        break;
      case 3:
        weights = s_bc67InterpolationWeights3;
        PL_ASSERT_DEV(uiWc < 8, "");

        break;
      case 4:
        weights = s_bc67InterpolationWeights4;
        PL_ASSERT_DEV(uiWc < 16, "");

        break;
      default:
        PL_ASSERT_NOT_IMPLEMENTED;
        ref_out.r = ref_out.g = ref_out.b = 0;
        return;
    }
    ref_out.r = plUInt8(
      (plUInt32(c0.r) * plUInt32(s_bc67WeightMax - weights[uiWc]) + plUInt32(c1.r) * plUInt32(weights[uiWc]) + s_bc67WeightRound) >> s_bc67WeightShift);
    ref_out.g = plUInt8(
      (plUInt32(c0.g) * plUInt32(s_bc67WeightMax - weights[uiWc]) + plUInt32(c1.g) * plUInt32(weights[uiWc]) + s_bc67WeightRound) >> s_bc67WeightShift);
    ref_out.b = plUInt8(
      (plUInt32(c0.b) * plUInt32(s_bc67WeightMax - weights[uiWc]) + plUInt32(c1.b) * plUInt32(weights[uiWc]) + s_bc67WeightRound) >> s_bc67WeightShift);
  }

  static void interpolateA(const plColorBaseUB& c0, const plColorBaseUB& c1, plUInt32 uiWa, plUInt32 uiWaprec, plColorBaseUB& ref_out)
  {
    const int* weights = nullptr;
    switch (uiWaprec)
    {
      case 2:
        weights = s_bc67InterpolationWeights2;
        PL_ASSERT_DEV(uiWa < 4, "");

        break;
      case 3:
        weights = s_bc67InterpolationWeights3;
        PL_ASSERT_DEV(uiWa < 8, "");

        break;
      case 4:
        weights = s_bc67InterpolationWeights4;
        PL_ASSERT_DEV(uiWa < 16, "");

        break;
      default:
        PL_ASSERT_NOT_IMPLEMENTED;
        ref_out.a = 0;
        return;
    }
    ref_out.a = plUInt8(
      (plUInt32(c0.a) * plUInt32(s_bc67WeightMax - weights[uiWa]) + plUInt32(c1.a) * plUInt32(weights[uiWa]) + s_bc67WeightRound) >> s_bc67WeightShift);
  }

  static void interpolate(
    const plColorBaseUB& c0, const plColorBaseUB& c1, plUInt32 uiWc, plUInt32 uiWa, plUInt32 uiWcprec, plUInt32 uiWaprec, plColorBaseUB& ref_out)
  {
    interpolateRGB(c0, c1, uiWc, uiWcprec, ref_out);
    interpolateA(c0, c1, uiWa, uiWaprec, ref_out);
  }

  static const plUInt16 s_bc6Float16Sign_Mask = 0x8000; // f16 sign mask
  static const plUInt16 s_bc6Float16Max = 0x7bff;       // MAXFLT bit pattern

  class BC6IntColor
  {
  public:
    plInt32 r, g, b;
    plInt32 pad = 0;

  public:
    BC6IntColor() = default;
    BC6IntColor(plInt32 iNr, plInt32 iNg, plInt32 iNb)
    {
      r = iNr;
      g = iNg;
      b = iNb;
    }

    BC6IntColor& operator+=(const BC6IntColor& c)
    {
      r += c.r;
      g += c.g;
      b += c.b;
      return *this;
    }

    BC6IntColor& operator&=(const BC6IntColor& c)
    {
      r &= c.r;
      g &= c.g;
      b &= c.b;
      return *this;
    }

    BC6IntColor& clamp(plInt32 iMin, plInt32 iMax)
    {
      r = plMath::Min(iMax, plMath::Max(iMin, r));
      g = plMath::Min(iMax, plMath::Max(iMin, g));
      b = plMath::Min(iMax, plMath::Max(iMin, b));
      return *this;
    }

    BC6IntColor& signExtend(const plColorBaseUB& prec)
    {
      r = signExtend(r, prec.r);
      g = signExtend(g, prec.g);
      b = signExtend(b, prec.b);
      return *this;
    }

    void toF16(plFloat16 p16[4], bool bIsSigned) const
    {
      p16[0] = intToF16(r, bIsSigned);
      p16[1] = intToF16(g, bIsSigned);
      p16[2] = intToF16(b, bIsSigned);
      p16[3] = 1.0f;
    }

  private:
    static plInt32 signExtend(plInt32 x, plInt32 nb) { return ((x & (1 << (nb - 1))) ? (~0 << nb) : 0) | x; }

    static plFloat16 intToF16(plInt32 input, bool isSigned)
    {
      plFloat16 h;
      plUInt16 out;
      if (isSigned)
      {
        int s = 0;
        if (input < 0)
        {
          s = s_bc6Float16Sign_Mask;
          input = -input;
        }
        out = plUInt16(s | input);
      }
      else
      {
        PL_ASSERT_DEV(input >= 0 && input <= s_bc6Float16Max, "");
        out = (plUInt16)input;
      }

      *((plUInt16*)&h) = out;
      return h;
    }
  };

  static_assert(sizeof(BC6IntColor) == 16, "Unexpected packing.");

  struct BC6IntEndPntPair
  {
    BC6IntColor A;
    BC6IntColor B;
  };

  inline void bc6TransformInverse(BC6IntEndPntPair pEndPts[], const plColorBaseUB& prec, bool bIsSigned)
  {
    BC6IntColor wrapMask((1 << prec.r) - 1, (1 << prec.g) - 1, (1 << prec.b) - 1);
    pEndPts[0].B += pEndPts[0].A;
    pEndPts[0].B &= wrapMask;
    pEndPts[1].A += pEndPts[0].A;
    pEndPts[1].A &= wrapMask;
    pEndPts[1].B += pEndPts[0].A;
    pEndPts[1].B &= wrapMask;
    if (bIsSigned)
    {
      pEndPts[0].B.signExtend(prec);
      pEndPts[1].A.signExtend(prec);
      pEndPts[1].B.signExtend(prec);
    }
  }

  static plInt32 bc6Unquantize(plInt32 iComp, plUInt8 uiBitsPerComp, bool bIsSigned)
  {
    plInt32 unq = 0, s = 0;
    if (bIsSigned)
    {
      if (uiBitsPerComp >= 16)
      {
        unq = iComp;
      }
      else
      {
        if (iComp < 0)
        {
          s = 1;
          iComp = -iComp;
        }

        if (iComp == 0)
          unq = 0;
        else if (iComp >= ((1 << (uiBitsPerComp - 1)) - 1))
          unq = 0x7FFF;
        else
          unq = ((iComp << 15) + 0x4000) >> (uiBitsPerComp - 1);

        if (s)
          unq = -unq;
      }
    }
    else
    {
      if (uiBitsPerComp >= 15)
        unq = iComp;
      else if (iComp == 0)
        unq = 0;
      else if (iComp == ((1 << uiBitsPerComp) - 1))
        unq = 0xFFFF;
      else
        unq = ((iComp << 16) + 0x8000) >> uiBitsPerComp;
    }

    return unq;
  }

  static plInt32 bc6FinishUnquantize(plInt32 iComp, bool bIsSigned)
  {
    if (bIsSigned)
    {
      return (iComp < 0) ? -(((-iComp) * 31) >> 5) : (iComp * 31) >> 5; // scale the magnitude by 31/32
    }
    else
    {
      return (iComp * 31) >> 6; // scale the magnitude by 31/64
    }
  }

  plUInt8 bc7Unquantize(plUInt8 uiComp, plUInt32 uiPrec)
  {
    PL_ASSERT_DEV(0 < uiPrec && uiPrec <= 8, "");
    uiComp = uiComp << (8 - uiPrec);
    return uiComp | (uiComp >> uiPrec);
  }

  plColorBaseUB bc7Unquantize(const plColorBaseUB& c, const plColorBaseUB& rgbaPrec)
  {
    plColorBaseUB q;
    q.r = bc7Unquantize(c.r, rgbaPrec.r);
    q.g = bc7Unquantize(c.g, rgbaPrec.g);
    q.b = bc7Unquantize(c.b, rgbaPrec.b);
    q.a = rgbaPrec.a > 0 ? bc7Unquantize(c.a, rgbaPrec.a) : 255;
    return q;
  }

  void fillWithErrorColors(plColorLinear16f* pOutputRGBA)
  {
    for (plUInt32 i = 0; i < s_bc67NumPixelsPerBlock; ++i)
    {
      pOutputRGBA[i] = plColorLinear16f(0.0f, 0.0f, 0.0f, 1.0f);
    }
  }

  void fillWithErrorColors(plColorBaseUB* pOutputRGBA)
  {
    for (plUInt32 i = 0; i < s_bc67NumPixelsPerBlock; ++i)
    {
      pOutputRGBA[i] = plColorBaseUB(0, 0, 0, 255);
    }
  }
} // namespace

void plDecompressBlockBC6(const plUInt8* pSource, plColorLinear16f* pTarget, bool bIsSigned)
{
  PL_ASSERT_DEV(pTarget, "");

  plUInt32 startBit = 0;
  plUInt8 mode = getBits(pSource, startBit, 2);
  if (mode != 0x00 && mode != 0x01)
  {
    mode = (getBits(pSource, startBit, 3) << 2) | mode;
  }

  PL_ASSERT_DEV(mode < 32, "");


  if (s_bc6ModeToInfo[mode] >= 0)
  {
    PL_ASSERT_DEV(s_bc6ModeToInfo[mode] < PL_ARRAY_SIZE(s_bc6ModeInfos), "");

    const BC6ModeDescriptor* desc = s_bc6ModeDescs[s_bc6ModeToInfo[mode]];

    PL_ASSERT_DEV(s_bc6ModeToInfo[mode] < PL_ARRAY_SIZE(s_bc6ModeDescs), "");

    const BC6ModeInfo& info = s_bc6ModeInfos[s_bc6ModeToInfo[mode]];

    BC6IntEndPntPair endPts[s_bc6MaxRegions];
    memset(endPts, 0, s_bc6MaxRegions * 2 * sizeof(BC6IntColor));
    plUInt32 shape = 0;

    // Read header
    const plUInt32 headerBits = info.partitions > 0 ? 82 : 65;
    while (startBit < headerBits)
    {
      plUInt32 curBit = startBit;
      if (getBit(pSource, startBit))
      {
        switch (desc[curBit].field)
        {
          case D:
            shape |= 1 << plUInt32(desc[curBit].bit);
            break;
          case RW:
            endPts[0].A.r |= 1 << plUInt32(desc[curBit].bit);
            break;
          case RX:
            endPts[0].B.r |= 1 << plUInt32(desc[curBit].bit);
            break;
          case RY:
            endPts[1].A.r |= 1 << plUInt32(desc[curBit].bit);
            break;
          case RZ:
            endPts[1].B.r |= 1 << plUInt32(desc[curBit].bit);
            break;
          case GW:
            endPts[0].A.g |= 1 << plUInt32(desc[curBit].bit);
            break;
          case GX:
            endPts[0].B.g |= 1 << plUInt32(desc[curBit].bit);
            break;
          case GY:
            endPts[1].A.g |= 1 << plUInt32(desc[curBit].bit);
            break;
          case GZ:
            endPts[1].B.g |= 1 << plUInt32(desc[curBit].bit);
            break;
          case BW:
            endPts[0].A.b |= 1 << plUInt32(desc[curBit].bit);
            break;
          case BX:
            endPts[0].B.b |= 1 << plUInt32(desc[curBit].bit);
            break;
          case BY:
            endPts[1].A.b |= 1 << plUInt32(desc[curBit].bit);
            break;
          case BZ:
            endPts[1].B.b |= 1 << plUInt32(desc[curBit].bit);
            break;
          default:
            plLog::Warning("BC6H: Invalid header bits encountered during decoding.");
            fillWithErrorColors(pTarget);
            return;
        }
      }
    }

    PL_ASSERT_DEV(shape < 64, "");


    // Sign extend necessary end points
    if (bIsSigned)
    {
      endPts[0].A.signExtend(info.rgbaPrec[0][0]);
    }
    if (bIsSigned || info.transformed)
    {
      PL_ASSERT_DEV(info.partitions < s_bc6MaxRegions, "");

      for (plUInt32 p = 0; p <= info.partitions; ++p)
      {
        if (p != 0)
        {
          endPts[p].A.signExtend(info.rgbaPrec[p][0]);
        }
        endPts[p].B.signExtend(info.rgbaPrec[p][1]);
      }
    }

    // Inverse transform the end points
    if (info.transformed)
    {
      bc6TransformInverse(endPts, info.rgbaPrec[0][0], bIsSigned);
    }

    // Read indices
    for (plUInt32 i = 0; i < s_bc67NumPixelsPerBlock; ++i)
    {
      plUInt32 numBits = isFixUpOffset(info.partitions, shape, i) ? info.indexPrec - 1 : info.indexPrec;
      if (startBit + numBits > 128)
      {
        plLog::Warning("BC6H: Invalid block encountered during decoding.");
        fillWithErrorColors(pTarget);
        return;
      }
      plUInt8 index = getBits(pSource, startBit, numBits);

      if (index >= ((info.partitions > 0) ? 8 : 16))
      {
        plLog::Warning("BC6H: Invalid index encountered during decoding.");
        fillWithErrorColors(pTarget);
        return;
      }

      plUInt32 region = s_bc67PartitionTable[info.partitions][shape][i];
      PL_ASSERT_DEV(region < s_bc6MaxRegions, "");


      // Unquantize endpoints and interpolate
      int r1 = bc6Unquantize(endPts[region].A.r, info.rgbaPrec[0][0].r, bIsSigned);
      int g1 = bc6Unquantize(endPts[region].A.g, info.rgbaPrec[0][0].g, bIsSigned);
      int b1 = bc6Unquantize(endPts[region].A.b, info.rgbaPrec[0][0].b, bIsSigned);
      int r2 = bc6Unquantize(endPts[region].B.r, info.rgbaPrec[0][0].r, bIsSigned);
      int g2 = bc6Unquantize(endPts[region].B.g, info.rgbaPrec[0][0].g, bIsSigned);
      int b2 = bc6Unquantize(endPts[region].B.b, info.rgbaPrec[0][0].b, bIsSigned);
      const int* weights = info.partitions > 0 ? s_bc67InterpolationWeights3 : s_bc67InterpolationWeights4;
      BC6IntColor fc;
      fc.r = bc6FinishUnquantize((r1 * (s_bc67WeightMax - weights[index]) + r2 * weights[index] + s_bc67WeightRound) >> s_bc67WeightShift, bIsSigned);
      fc.g = bc6FinishUnquantize((g1 * (s_bc67WeightMax - weights[index]) + g2 * weights[index] + s_bc67WeightRound) >> s_bc67WeightShift, bIsSigned);
      fc.b = bc6FinishUnquantize((b1 * (s_bc67WeightMax - weights[index]) + b2 * weights[index] + s_bc67WeightRound) >> s_bc67WeightShift, bIsSigned);

      plColorLinear16f outColor;
      fc.toF16(outColor.GetData(), bIsSigned);
      pTarget[i] = outColor;
    }
  }
  else
  {
    plStringBuilder msg = "BC6H: Invalid mode encountered during decoding.";
    switch (mode)
    {
      case 0x13:
        msg = "BC6H: Reserved mode 10011 encountered during decoding.";
        break;
      case 0x17:
        msg = "BC6H: Reserved mode 10111 encountered during decoding.";
        break;
      case 0x1B:
        msg = "BC6H: Reserved mode 11011 encountered during decoding.";
        break;
      case 0x1F:
        msg = "BC6H: Reserved mode 11111 encountered during decoding.";
        break;
    }
    plLog::Warning(msg);

    // Per the BC6 format spec, we must return opaque black.
    fillWithErrorColors(pTarget);
  }
}

PL_TEXTURE_DLL void plDecompressBlockBC7(const plUInt8* pSource, plColorBaseUB* pTarget)
{
  PL_ASSERT_DEV(pTarget, "");

  plUInt32 first = 0;
  while (first < 128 && !getBit(pSource, first))
  { /* intentionally left empty */
  }
  plUInt8 mode = plUInt8(first - 1);

  if (mode < 8)
  {
    const plUInt8 uPartitions = s_bc7ModeInfos[mode].partitions;
    PL_ASSERT_DEV(uPartitions < s_bc7MaxRegions, "");


    const plUInt8 numEndPts = (uPartitions + 1) << 1;
    const plUInt8 indexPrec = s_bc7ModeInfos[mode].indexPrec;
    const plUInt8 indexPrec2 = s_bc7ModeInfos[mode].indexPrec2;

    plUInt32 startBit = mode + 1;
    plUInt8 p[6];
    plUInt8 shape = getBits(pSource, startBit, s_bc7ModeInfos[mode].partitionBits);
    PL_ASSERT_DEV(shape < s_bc7MaxShapes, "");


    plUInt8 rotation = getBits(pSource, startBit, s_bc7ModeInfos[mode].rotationBits);
    PL_ASSERT_DEV(rotation < 4, "");

    plUInt8 indexMode = getBits(pSource, startBit, s_bc7ModeInfos[mode].indexModeBits);
    PL_ASSERT_DEV(indexMode < 2, "");

    plColorBaseUB c[s_bc7MaxRegions << 1];
    const plColorBaseUB RGBAPrec = s_bc7ModeInfos[mode].rgbaPrec;
    const plColorBaseUB RGBAPrecWithP = s_bc7ModeInfos[mode].rgbaPrecWithP;

    PL_ASSERT_DEV(numEndPts <= (s_bc7MaxRegions << 1), "");
    plUInt32 i = 0;

    // Red channel
    for (i = 0; i < numEndPts; ++i)
    {
      if (startBit + RGBAPrec.r > 128)
      {
        plLog::Warning("BC7: Invalid block encountered during decoding.");
        fillWithErrorColors(pTarget);
        return;
      }

      c[i].r = getBits(pSource, startBit, RGBAPrec.r);
    }

    // Green channel
    for (i = 0; i < numEndPts; ++i)
    {
      if (startBit + RGBAPrec.g > 128)
      {
        plLog::Warning("BC7: Invalid block encountered during decoding.");
        fillWithErrorColors(pTarget);
        return;
      }

      c[i].g = getBits(pSource, startBit, RGBAPrec.g);
    }

    // Blue channel
    for (i = 0; i < numEndPts; ++i)
    {
      if (startBit + RGBAPrec.b > 128)
      {
        plLog::Warning("BC7: Invalid block encountered during decoding.");
        fillWithErrorColors(pTarget);
        return;
      }

      c[i].b = getBits(pSource, startBit, RGBAPrec.b);
    }

    // Alpha channel
    for (i = 0; i < numEndPts; ++i)
    {
      if (startBit + RGBAPrec.a > 128)
      {
        plLog::Warning("BC7: Invalid block encountered during decoding.");
        fillWithErrorColors(pTarget);
        return;
      }

      c[i].a = RGBAPrec.a ? getBits(pSource, startBit, RGBAPrec.a) : 255;
    }

    // P-bits
    PL_ASSERT_DEV(s_bc7ModeInfos[mode].pBits <= 6, "");

    for (i = 0; i < s_bc7ModeInfos[mode].pBits; ++i)
    {
      if (startBit > 127)
      {
        plLog::Warning("BC7: Invalid block encountered during decoding.");
        fillWithErrorColors(pTarget);
        return;
      }

      p[i] = getBit(pSource, startBit);
    }

    if (s_bc7ModeInfos[mode].pBits)
    {
      for (i = 0; i < numEndPts; ++i)
      {
        plUInt32 pi = i * s_bc7ModeInfos[mode].pBits / numEndPts;
        for (plUInt8 ch = 0; ch < s_bc7NumChannels; ++ch)
        {
          if (RGBAPrec.GetData()[ch] != RGBAPrecWithP.GetData()[ch])
          {
            c[i].GetData()[ch] = (c[i].GetData()[ch] << 1) | p[pi];
          }
        }
      }
    }

    for (i = 0; i < numEndPts; ++i)
    {
      c[i] = bc7Unquantize(c[i], RGBAPrecWithP);
    }

    plUInt8 w1[s_bc67NumPixelsPerBlock], w2[s_bc67NumPixelsPerBlock];

    // read color indices
    for (i = 0; i < s_bc67NumPixelsPerBlock; ++i)
    {
      plUInt32 numBits = isFixUpOffset(s_bc7ModeInfos[mode].partitions, shape, i) ? indexPrec - 1 : indexPrec;
      if (startBit + numBits > 128)
      {
        plLog::Warning("BC7: Invalid block encountered during decoding.");
        fillWithErrorColors(pTarget);
        return;
      }
      w1[i] = getBits(pSource, startBit, numBits);
    }

    // read alpha indices
    if (indexPrec2)
    {
      for (i = 0; i < s_bc67NumPixelsPerBlock; ++i)
      {
        plUInt32 numBits = i ? indexPrec2 : indexPrec2 - 1;
        if (startBit + numBits > 128)
        {
          plLog::Warning("BC7: Invalid block encountered during decoding.");
          fillWithErrorColors(pTarget);
          return;
        }
        w2[i] = getBits(pSource, startBit, numBits);
      }
    }

    for (i = 0; i < s_bc67NumPixelsPerBlock; ++i)
    {
      plUInt8 uRegion = s_bc67PartitionTable[uPartitions][shape][i];
      plColorBaseUB outPixel;
      if (indexPrec2 == 0)
      {
        interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w1[i], w1[i], indexPrec, indexPrec, outPixel);
      }
      else
      {
        if (indexMode == 0)
        {
          interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w1[i], w2[i], indexPrec, indexPrec2, outPixel);
        }
        else
        {
          interpolate(c[uRegion << 1], c[(uRegion << 1) + 1], w2[i], w1[i], indexPrec2, indexPrec, outPixel);
        }
      }

      switch (rotation)
      {
        case 1:
          plMath::Swap(outPixel.r, outPixel.a);
          break;
        case 2:
          plMath::Swap(outPixel.g, outPixel.a);
          break;
        case 3:
          plMath::Swap(outPixel.b, outPixel.a);
          break;
      }

      pTarget[i] = outPixel;
    }
  }
  else
  {
    plLog::Warning("BC7: Reserved mode 8 encountered during decoding.");
    // Per the BC7 format spec, we must return transparent black.
    fillWithErrorColors(pTarget);
  }
} // namespace

class plImageConversion_BC1_RGBA : public plImageConversionStepDecompressBlocks
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::BC1_UNORM, plImageFormat::R8G8B8A8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::BC1_UNORM_SRGB, plImageFormat::R8G8B8A8_UNORM_SRGB, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult DecompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumBlocks, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    const plUInt32 elementsPerBlock = 16;

    plUInt32 sourceStride = elementsPerBlock * plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = elementsPerBlock * plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    for (plUInt32 blockIndex = 0; blockIndex < uiNumBlocks; blockIndex++)
    {
      plDecompressBlockBC1(reinterpret_cast<const plUInt8*>(sourcePointer), reinterpret_cast<plColorBaseUB*>(targetPointer), false);

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_BC2_RGBA : public plImageConversionStepDecompressBlocks
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::BC2_UNORM, plImageFormat::R8G8B8A8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::BC2_UNORM_SRGB, plImageFormat::R8G8B8A8_UNORM_SRGB, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult DecompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumBlocks, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    const plUInt32 elementsPerBlock = 16;

    plUInt32 sourceStride = elementsPerBlock * plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = elementsPerBlock * plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    for (plUInt32 blockIndex = 0; blockIndex < uiNumBlocks; blockIndex++)
    {
      decompressBlock(reinterpret_cast<const plUInt8*>(sourcePointer), reinterpret_cast<plColorBaseUB*>(targetPointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
    }

    return PL_SUCCESS;
  }

  static void decompressBlock(const plUInt8* pSourcePointer, plColorBaseUB* pTargetPointer)
  {
    plDecompressBlockBC1(pSourcePointer + 8, pTargetPointer, true);

    for (plUInt32 uiByteIdx = 0; uiByteIdx < 8; uiByteIdx++)
    {
      plUInt8 uiIndices = pSourcePointer[uiByteIdx];

      pTargetPointer[2 * uiByteIdx + 0].a = (uiIndices & 0x0F) | (uiIndices << 4);
      pTargetPointer[2 * uiByteIdx + 1].a = (uiIndices & 0xF0) | (uiIndices >> 4);
    }
  }
};

class plImageConversion_BC3_RGBA : public plImageConversionStepDecompressBlocks
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::BC3_UNORM, plImageFormat::R8G8B8A8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::BC3_UNORM_SRGB, plImageFormat::R8G8B8A8_UNORM_SRGB, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult DecompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumBlocks, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    const plUInt32 elementsPerBlock = 16;

    plUInt32 sourceStride = elementsPerBlock * plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = elementsPerBlock * plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    for (plUInt32 blockIndex = 0; blockIndex < uiNumBlocks; blockIndex++)
    {
      decompressBlock(reinterpret_cast<const plUInt8*>(sourcePointer), reinterpret_cast<plColorBaseUB*>(targetPointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
    }

    return PL_SUCCESS;
  }

  static void decompressBlock(const plUInt8* pSourcePointer, plColorBaseUB* pTargetPointer)
  {
    plDecompressBlockBC1(pSourcePointer + 8, pTargetPointer, true);
    plDecompressBlockBC4(pSourcePointer, reinterpret_cast<plUInt8*>(pTargetPointer) + 3, 4, 0);
  }
};

class plImageConversion_BC4_R : public plImageConversionStepDecompressBlocks
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::BC4_UNORM, plImageFormat::R8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::BC4_SNORM, plImageFormat::R8_SNORM, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult DecompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumBlocks, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    const plUInt32 elementsPerBlock = 16;

    plUInt32 sourceStride = elementsPerBlock * plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = elementsPerBlock * plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    // Bias to shift signed data into unsigned range so we can treat it the same as unsigned
    plUInt8 bias = 0;
    if (plImageFormat::GetDataType(sourceFormat) == plImageFormatDataType::SNORM)
    {
      bias = 128;
    }

    for (plUInt32 blockIndex = 0; blockIndex < uiNumBlocks; blockIndex++)
    {
      decompressBlock(reinterpret_cast<const plUInt8*>(sourcePointer), reinterpret_cast<plUInt8*>(targetPointer), bias);

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
    }

    return PL_SUCCESS;
  }

  static void decompressBlock(const plUInt8* pSourcePointer, plUInt8* pTargetPointer, plUInt8 uiBias)
  {
    plDecompressBlockBC4(pSourcePointer, pTargetPointer, 1, uiBias);
  }
};

class plImageConversion_BC5_RG : public plImageConversionStepDecompressBlocks
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::BC5_UNORM, plImageFormat::R8G8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::BC5_SNORM, plImageFormat::R8G8_SNORM, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult DecompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumBlocks, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    const plUInt32 elementsPerBlock = 16;

    plUInt32 sourceStride = elementsPerBlock * plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt32 targetStride = elementsPerBlock * plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    // Bias to shift signed data into unsigned range so we can treat it the same as unsigned
    plUInt8 bias = 0;
    if (plImageFormat::GetDataType(sourceFormat) == plImageFormatDataType::SNORM)
    {
      bias = 128;
    }

    for (plUInt32 blockIndex = 0; blockIndex < uiNumBlocks; blockIndex++)
    {
      decompressBlock(reinterpret_cast<const plUInt8*>(sourcePointer), reinterpret_cast<plUInt8*>(targetPointer), bias);

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
    }

    return PL_SUCCESS;
  }

  static void decompressBlock(const plUInt8* pSourcePointer, plUInt8* pTargetPointer, plUInt8 uiBias)
  {
    plDecompressBlockBC4(pSourcePointer + 0, pTargetPointer + 0, 2, uiBias);
    plDecompressBlockBC4(pSourcePointer + 8, pTargetPointer + 1, 2, uiBias);
  }
};


class plImageConversion_BC6_RGB : public plImageConversionStepDecompressBlocks
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::BC6H_UF16, plImageFormat::R16G16B16A16_FLOAT, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::BC6H_SF16, plImageFormat::R16G16B16A16_FLOAT, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult DecompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumBlocks, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    const plUInt32 targetFormatByteSize = plImageFormat::GetBitsPerPixel(targetFormat) / 8;
    PL_ASSERT_DEV(targetFormatByteSize == sizeof(plColorLinear16f), "");

    const plUInt32 sourceStride = s_bc67NumPixelsPerBlock * plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    const plUInt32 targetStride = s_bc67NumPixelsPerBlock * targetFormatByteSize;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    const bool isSourceFormatSigned = sourceFormat == plImageFormat::BC6H_SF16;

    for (plUInt32 blockIndex = 0; blockIndex < uiNumBlocks; ++blockIndex)
    {
      plDecompressBlockBC6(reinterpret_cast<const plUInt8*>(sourcePointer), reinterpret_cast<plColorLinear16f*>(targetPointer), isSourceFormatSigned);

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_BC7_RGBA : public plImageConversionStepDecompressBlocks
{
public:
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::BC7_UNORM, plImageFormat::R8G8B8A8_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::BC7_UNORM_SRGB, plImageFormat::R8G8B8A8_UNORM_SRGB, plImageConversionFlags::Default)};
    return supportedConversions;
  }

  virtual plResult DecompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 uiNumBlocks, plImageFormat::Enum sourceFormat,
    plImageFormat::Enum targetFormat) const override
  {
    const plUInt32 sourceStride = s_bc67NumPixelsPerBlock * plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    const plUInt32 targetStride = s_bc67NumPixelsPerBlock * plImageFormat::GetBitsPerPixel(targetFormat) / 8;

    const void* sourcePointer = source.GetPtr();
    void* targetPointer = target.GetPtr();

    for (plUInt32 blockIndex = 0; blockIndex < uiNumBlocks; ++blockIndex)
    {
      plDecompressBlockBC7(reinterpret_cast<const plUInt8*>(sourcePointer), reinterpret_cast<plColorBaseUB*>(targetPointer));

      sourcePointer = plMemoryUtils::AddByteOffset(sourcePointer, sourceStride);
      targetPointer = plMemoryUtils::AddByteOffset(targetPointer, targetStride);
    }

    return PL_SUCCESS;
  }
};

#if defined(PL_SUPPORTS_BC4_COMPRESSOR)
class plImageConversion_CompressBC4 : public plImageConversionStepCompressBlocks
{
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R8_UNORM, plImageFormat::BC4_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8_SNORM, plImageFormat::BC4_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_UNORM, plImageFormat::BC4_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_SNORM, plImageFormat::BC4_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, plImageFormat::BC4_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_SNORM, plImageFormat::BC4_SNORM, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult CompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 numBlocksX, plUInt32 numBlocksY,
    plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat) const override
  {
    plUInt32 stride = plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt64 rowPitch = plImageFormat::GetRowPitch(sourceFormat, 4 * numBlocksX);

    // Bias to shift signed data into unsigned range so we can treat it the same as unsigned
    plUInt8 bias = 0;
    if (plImageFormat::GetDataType(sourceFormat) == plImageFormatDataType::SNORM)
    {
      bias = 128;
    }

    for (plUInt32 blockY = 0; blockY < numBlocksY; ++blockY)
    {
      for (plUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
      {
        plUInt8 sourceBlock[16];

        for (plUInt32 y = 0; y < 4; ++y)
        {
          const plUInt8* sourcePointer = static_cast<const plUInt8*>(source.GetPtr()) + (4 * blockY + y) * rowPitch;

          for (plUInt32 x = 0; x < 4; ++x)
          {
            sourceBlock[4 * y + x] = sourcePointer[(x + 4 * blockX) * stride] + bias;
          }
        }

        plUInt32 a0, a1;
        findBestPaletteBC4(sourceBlock, a0, a1);

        plUInt8* targetPointer = static_cast<plUInt8*>(target.GetPtr()) + (blockY * numBlocksX + blockX) * 8;
        packBlockBC4(sourceBlock, a0, a1, targetPointer);

        targetPointer[0] -= bias;
        targetPointer[1] -= bias;
      }
    }

    return PL_SUCCESS;
  }
};

class plImageConversion_CompressBC5 : public plImageConversionStepCompressBlocks
{
  virtual plArrayPtr<const plImageConversionEntry> GetSupportedConversions() const override
  {
    static plImageConversionEntry supportedConversions[] = {
      plImageConversionEntry(plImageFormat::R8G8_UNORM, plImageFormat::BC5_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8_SNORM, plImageFormat::BC5_SNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_UNORM, plImageFormat::BC5_UNORM, plImageConversionFlags::Default),
      plImageConversionEntry(plImageFormat::R8G8B8A8_SNORM, plImageFormat::BC5_SNORM, plImageConversionFlags::Default),
    };
    return supportedConversions;
  }

  virtual plResult CompressBlocks(plConstByteBlobPtr source, plByteBlobPtr target, plUInt32 numBlocksX, plUInt32 numBlocksY,
    plImageFormat::Enum sourceFormat, plImageFormat::Enum targetFormat) const override
  {
    plUInt32 stride = plImageFormat::GetBitsPerPixel(sourceFormat) / 8;
    plUInt64 rowPitch = plImageFormat::GetRowPitch(sourceFormat, 4 * numBlocksX);

    // Bias to shift signed data into unsigned range so we can treat it the same as unsigned
    plUInt8 bias = 0;
    if (plImageFormat::GetDataType(sourceFormat) == plImageFormatDataType::SNORM)
    {
      bias = 128;
    }

    for (plUInt32 blockY = 0; blockY < numBlocksY; ++blockY)
    {
      for (plUInt32 blockX = 0; blockX < numBlocksX; ++blockX)
      {
        plUInt8 sourceBlockR[16];
        plUInt8 sourceBlockG[16];

        for (plUInt32 y = 0; y < 4; ++y)
        {
          const plUInt8* sourcePointer = static_cast<const plUInt8*>(source.GetPtr()) + (4 * blockY + y) * rowPitch;

          for (plUInt32 x = 0; x < 4; ++x)
          {
            sourceBlockR[4 * y + x] = sourcePointer[(x + 4 * blockX) * stride + 0] + bias;
            sourceBlockG[4 * y + x] = sourcePointer[(x + 4 * blockX) * stride + 1] + bias;
          }
        }

        plUInt8* targetPointer = static_cast<plUInt8*>(target.GetPtr()) + (blockY * numBlocksX + blockX) * 16;

        {
          plUInt32 a0, a1;
          findBestPaletteBC4(sourceBlockR, a0, a1);
          packBlockBC4(sourceBlockR, a0, a1, targetPointer);

          // Undo biasing for signed formats by shifting palette upper and lower bound back into signed range
          targetPointer[0] -= bias;
          targetPointer[1] -= bias;
        }

        {
          plUInt32 a0, a1;
          findBestPaletteBC4(sourceBlockG, a0, a1);
          packBlockBC4(sourceBlockG, a0, a1, targetPointer + 8);

          // Undo biasing for signed formats by shifting palette upper and lower bound back into signed range
          targetPointer[8] -= bias;
          targetPointer[9] -= bias;
        }
      }
    }

    return PL_SUCCESS;
  }
};

static plImageConversion_CompressBC4 s_conversion_compressBC4;
static plImageConversion_CompressBC5 s_conversion_compressBC5;

#endif

// PL_STATICLINK_FORCE
static plImageConversion_BC1_RGBA s_conversion_BC1_RGBA;
static plImageConversion_BC2_RGBA s_conversion_BC2_RGBA;
static plImageConversion_BC3_RGBA s_conversion_BC3_RGBA;
static plImageConversion_BC4_R s_conversion_BC4_R;
static plImageConversion_BC5_RG s_conversion_BC5_RG;
static plImageConversion_BC6_RGB s_conversion_BC6_RGB;
static plImageConversion_BC7_RGBA s_conversion_BC7_RGBA;




PL_STATICLINK_FILE(Texture, Texture_Image_Conversions_DXTConversions);

