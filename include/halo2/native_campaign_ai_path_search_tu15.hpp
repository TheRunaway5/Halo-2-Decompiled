#pragma once
#include "halo2/physics_world_runtime.hpp"
#include "halo2/native_tu15_physics_frame.hpp"
#include <stdexcept>

namespace halo2 {
// Original state the native cannot reproduce: 27BD30 would publish neighbor
// row `row` (index >= 64) into 27B340's 64-row frame array. The original's
// 65th row overwrites 27B340's return address, its argument and 27B2B0's
// saved registers. Thrown before that row's first store; every preceding
// read and store has been published.
class NativeCampaignAiPathSearchOverrunTu15 final:public std::runtime_error {
    std::uint32_t site_;std::int32_t row_;
public:
    NativeCampaignAiPathSearchOverrunTu15(std::uint32_t site,std::int32_t row);
    std::uint32_t site()const noexcept{return site_;}
    std::int32_t row()const noexcept{return row_;}
};

// Original 27B2B0 path search (EAX = workspace). It resets the retained
// search state (+AE node count, +110B0 heap count, the 0x2000-byte +120B6
// node hash, +90/+94/+98 best-goal fields), runs native 27B100 (query start)
// and native 27B340 (expansion), then publishes the +70 retry counter.
// Returns the original AL.
//
// `stack` is ESP at 27B2B0's first instruction (the address that holds the
// return address). Like the original, the expansion keeps its 64 neighbor
// rows (0x30 bytes each, at stack-0xC14) and its candidate point (stack-0xC4C)
// in that real frame below `stack`, writes 218780's two resolved jump points
// below them, and reads the bytes the original never initializes there
// (row +A/+B and the candidate frame's upper word) from actual stack history.
// Every retained workspace/navigation/global read and store keeps the
// original order, width and store PC.
//
// Native closure: 27B100 (+218C40/32CC0C/27BCE0), 27B340, 27BB60, 27BC00,
// 27BD30, 202970, 186E30, 218C40/218B20/218780, 218EB0, 27C520 (+218960),
// 278E60 (+FF1A0/FF580/30BD0). No callee is transported.
// Requires masked x87/SIMD exceptions and an empty x87 stack.
bool native_campaign_ai_path_search_tu15(SourcePhysicsContactMemory &,
    std::uint32_t workspace,std::uint32_t stack,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// Actual 27B2B0 call ABI (26A380's CALL at 26A831, return 26A836): EAX
// workspace, [ESP] return address. Exit: AL result, EBX/EBP/ESI/EDI
// preserved, ESP + 4 (plain RET), FP controls/status threaded. EAX[31:8],
// ECX, EDX and XMM are caller-saved scratch (26A836 consumes only AL):
// EAX is the zero-extended result, ECX/EDX/XMM are left as they were.
void native_campaign_ai_path_search_call_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});

// Original 27B340 expansion: pop the heap, expand through 27BD30 neighbor
// rows, score/hash/insert nodes and track the best goal. `stack` is ESP at
// 27B340's first instruction ([stack] return, [stack+4] workspace).
bool native_campaign_ai_path_expand_tu15(SourcePhysicsContactMemory &,
    std::uint32_t workspace,std::uint32_t stack,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// Actual 27B340 ABI: [ESP+4] workspace; RET 4; AL; EBX/EBP/ESI/EDI preserved.
void native_campaign_ai_path_expand_call_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});

// Original 27BD30 neighbor generation for one search node: the linked
// navigation links (kinds 0 portal, 1/6 jump, 2 and 5 special points; 3/4
// skipped) and the sector's boundary-edge ring. Rows (0x30 bytes) are stored
// at `rows` with the original field widths and order. `stack` is ESP at
// 27BD30's first instruction; 218780's resolved points use its frame.
// `row_capacity` 0 publishes like the original without bound; otherwise a
// row index >= capacity throws NativeCampaignAiPathSearchOverrunTu15.
// Returns the original AX (row count).
std::int16_t native_campaign_ai_path_neighbors_tu15(SourcePhysicsContactMemory &,
    std::uint32_t navigation,std::uint32_t node,std::uint32_t rows,std::uint32_t workspace,
    std::uint32_t stack,std::uint32_t row_capacity,NativeTu15PhysicsFrameFp &,
    const SourcePhysicsContactRecordObserver & = {});
// Actual 27BD30 ABI: [ESP+4] navigation, +8 node, +C rows, +10 workspace;
// RET 10; AX count (EAX zero-extended); EBX/EBP/ESI/EDI preserved.
void native_campaign_ai_path_neighbors_call_tu15(SourcePhysicsContactMemory &,
    SourcePhysicsWorldRuntimeAbi &,const SourcePhysicsContactRecordObserver & = {});
}
