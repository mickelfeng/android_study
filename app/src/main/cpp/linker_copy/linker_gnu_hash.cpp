//
// Created by thouger on 2024/11/11.
//

#include <arm_neon.h>
#include "linker_gnu_hash.h"

struct __attribute__((aligned(8))) GnuHashInitEntry {
    uint64_t ignore_mask;
    uint32_t accum;
};

constexpr uint32_t kStep0 = 1;
constexpr uint32_t kStep1 = kStep0 * 33;
constexpr uint32_t kStep2 = kStep1 * 33;
constexpr uint32_t kStep3 = kStep2 * 33;
constexpr uint32_t kStep4 = kStep3 * 33;
constexpr uint32_t kStep5 = kStep4 * 33;
constexpr uint32_t kStep6 = kStep5 * 33;
constexpr uint32_t kStep7 = kStep6 * 33;
constexpr uint32_t kStep8 = kStep7 * 33;
constexpr uint32_t kStep9 = kStep8 * 33;
constexpr uint32_t kStep10 = kStep9 * 33;
constexpr uint32_t kStep11 = kStep10 * 33;

// Step by -1 through -7:  33 * 0x3e0f83e1 == 1 (mod 2**32)
constexpr uint32_t kStepN1 = kStep0 * 0x3e0f83e1;
constexpr uint32_t kStepN2 = kStepN1 * 0x3e0f83e1;
constexpr uint32_t kStepN3 = kStepN2 * 0x3e0f83e1;
constexpr uint32_t kStepN4 = kStepN3 * 0x3e0f83e1;
constexpr uint32_t kStepN5 = kStepN4 * 0x3e0f83e1;
constexpr uint32_t kStepN6 = kStepN5 * 0x3e0f83e1;
constexpr uint32_t kStepN7 = kStepN6 * 0x3e0f83e1;

std::pair<uint32_t, uint32_t> calculate_gnu_hash_neon(const char* name){
    static const struct GnuHashInitEntry kInitTable[] = {
            { // (addr&7) == 0
                    0ull,
                    5381u*kStep0,
            }, { // (addr&7) == 1
                    0xffull,
                    5381u*kStepN1 - 0xffu*kStepN1,
            }, { // (addr&7) == 2
                    0xffffull,
                    5381u*kStepN2 - 0xffu*kStepN1 - 0xffu*kStepN2,
            }, { // (addr&7) == 3
                    0xffffffull,
                    5381u*kStepN3 - 0xffu*kStepN1 - 0xffu*kStepN2 - 0xffu*kStepN3,
            }, { // (addr&7) == 4
                    0xffffffffull,
                    5381u*kStepN4 - 0xffu*kStepN1 - 0xffu*kStepN2 - 0xffu*kStepN3 - 0xffu*kStepN4,
            }, { // (addr&7) == 5
                    0xffffffffffull,
                    5381u*kStepN5 - 0xffu*kStepN1 - 0xffu*kStepN2 - 0xffu*kStepN3 - 0xffu*kStepN4 - 0xffu*kStepN5,
            }, { // (addr&7) == 6
                    0xffffffffffffull,
                    5381u*kStepN6 - 0xffu*kStepN1 - 0xffu*kStepN2 - 0xffu*kStepN3 - 0xffu*kStepN4 - 0xffu*kStepN5 - 0xffu*kStepN6,
            }, { // (addr&7) == 7
                    0xffffffffffffffull,
                    5381u*kStepN7 - 0xffu*kStepN1 - 0xffu*kStepN2 - 0xffu*kStepN3 - 0xffu*kStepN4 - 0xffu*kStepN5 - 0xffu*kStepN6 - 0xffu*kStepN7,
            },
    };

    uint8_t offset = reinterpret_cast<uintptr_t>(name) & 7;
    const uint64_t* chunk_ptr = reinterpret_cast<const uint64_t*>(reinterpret_cast<uintptr_t>(name) & ~7);
    const struct GnuHashInitEntry* entry = &kInitTable[offset];

    uint8x8_t chunk = vld1_u8(reinterpret_cast<const uint8_t*>(chunk_ptr));
    chunk |= vld1_u8(reinterpret_cast<const uint8_t*>(&entry->ignore_mask));

    uint32x4_t accum_lo = { 0 };
    uint32x4_t accum_hi = { entry->accum, 0, 0, 0 };
    const uint16x4_t kInclineVec = { kStep3, kStep2, kStep1, kStep0 };
    const uint32x4_t kStep8Vec = vdupq_n_u32(kStep8);
    uint8x8_t is_nul;
    uint16x8_t expand;

    while (1) {
        // Exit the loop if any of the 8 bytes is NUL.
        is_nul = vceq_u8(chunk, (uint8x8_t){ 0 });
        expand = vmovl_u8(chunk);
        uint64x1_t is_nul_64 = vreinterpret_u64_u8(is_nul);
        if (vget_lane_u64(is_nul_64, 0)) break;

        // Multiply both accumulators by 33**8.
        accum_lo = vmulq_u32(accum_lo, kStep8Vec);
        accum_hi = vmulq_u32(accum_hi, kStep8Vec);

        // Multiply each 4-piece subchunk by (33**3, 33**2, 33*1, 1), then accumulate the result. The lo
        // accumulator will be behind by 33**4 until the very end of the computation.
        accum_lo = vmlal_u16(accum_lo, vget_low_u16(expand), kInclineVec);
        accum_hi = vmlal_u16(accum_hi, vget_high_u16(expand), kInclineVec);

        // Load the next chunk.
        chunk = vld1_u8(reinterpret_cast<const uint8_t*>(++chunk_ptr));
    }

    // Reverse the is-NUL vector so we can use clz to count the number of remaining bytes.
    is_nul = vrev64_u8(is_nul);
    const uint64_t is_nul_u64 = vget_lane_u64(vreinterpret_u64_u8(is_nul), 0);
    const uint32_t num_valid_bits = __builtin_clzll(is_nul_u64);

    const uint32_t name_len = reinterpret_cast<const char*>(chunk_ptr) - name + (num_valid_bits >> 3);

    static const uint32_t kFinalStepTable[] = {
            kStep4, kStep0,   // 0 remaining bytes
            kStep5, kStep1,   // 1 remaining byte
            kStep6, kStep2,   // 2 remaining bytes
            kStep7, kStep3,   // 3 remaining bytes
            kStep8, kStep4,   // 4 remaining bytes
            kStep9, kStep5,   // 5 remaining bytes
            kStep10, kStep6,  // 6 remaining bytes
            kStep11, kStep7,  // 7 remaining bytes
    };

    // Advance the lo/hi accumulators appropriately for the number of remaining bytes. Multiply 33**4
    // into the lo accumulator to catch it up with the hi accumulator.
    const uint32_t* final_step = &kFinalStepTable[num_valid_bits >> 2];
    accum_lo = vmulq_u32(accum_lo, vdupq_n_u32(final_step[0]));
    accum_lo = vmlaq_u32(accum_lo, accum_hi, vdupq_n_u32(final_step[1]));

    static const uint32_t kFinalInclineTable[] = {
            0,      kStep6, kStep5, kStep4, kStep3, kStep2, kStep1, kStep0,
            0,      0,      0,      0,      0,      0,      0,      0,
    };

    // Prepare a vector to multiply powers of 33 into each of the remaining bytes.
    const uint32_t* const incline = &kFinalInclineTable[8 - (num_valid_bits >> 3)];
    const uint32x4_t incline_lo = vld1q_u32(incline);
    const uint32x4_t incline_hi = vld1q_u32(incline + 4);

    // Multiply 33 into each of the remaining 4-piece vectors, then accumulate everything into
    // accum_lo. Combine everything into a single 32-bit result.
    accum_lo = vmlaq_u32(accum_lo, vmovl_u16(vget_low_u16(expand)), incline_lo);
    accum_lo = vmlaq_u32(accum_lo, vmovl_u16(vget_high_u16(expand)), incline_hi);

    uint32x2_t sum = vadd_u32(vget_low_u32(accum_lo), vget_high_u32(accum_lo));
    const uint32_t hash = sum[0] + sum[1];

    return { hash, name_len };
}