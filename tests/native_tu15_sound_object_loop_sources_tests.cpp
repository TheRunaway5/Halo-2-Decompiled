// Differential test: the native object-loop "sources" kernels (18D5A0, 220C60,
// 18F9B0, 18FFB0, 190460, 1904D0, 220640, 18B580) against the lifted TU1.5
// routines they replace, from identical randomized game states across x87
// controls and MXCSR modes. The native run (native_tu15_object_loop_run) and
// the lifted run (source_run_object_loop_audio_tu15) see the same memory, the
// same camera views and the same deterministic simulations of every outside
// callee, and must agree on every register, XMM, x87/FP control and live
// memory byte, or fault at the same address.
//
// Coverage is measured from the lifted routine's own instruction trace.
#include "halo2/native_tu15_sound_manager.hpp"
#include "halo2/source_object_loop_audio_tu15.hpp"
#include "native_tu15_sound_test_support.hpp"
#include <cstring>
#include <set>
#include <string>
#include <vector>

using namespace halo2;
using namespace halo2::sound_test;
namespace {
using Abi=SourcePhysicsContactRuntimeAbi;
using Services=SourcePhysicsContactRuntimeServices;
using Call=SourcePhysicsContactRuntimeCall;
using Views=SourceObjectAudioViewsTu15;
using O=SourcePhysicsContactRecordObserver;
using FP=SourcePhysicsFactoryFloatingPoint;

// ---- lifted instruction PCs per routine (coverage denominators) ------------
const std::set<std::uint32_t> pc_18b580{0x18b580,0x18b583,0x18b586,0x18b589,0x18b58b,0x18b593,0x18b596,0x18b598,0x18b59b,0x18b5a3,0x18b5a8,0x18b5ab,0x18b5af,0x18b5b4,0x18b5b7};
const std::set<std::uint32_t> pc_1904d0{0x1904d0,0x1904d5,0x1904db,0x1904dd,0x1904e5,0x1904e7,0x1904ed,0x1904ef,0x1904f1,0x1904f4,0x1904f7,0x1904f9,0x1904ff,0x190504,0x190507,0x19050b,0x190510};
const std::set<std::uint32_t> pc_220640{0x220640,0x220641,0x220642,0x220648,0x22064a,0x22064d,0x22064f,0x220651,0x220654,0x220656,0x220658,0x22065b,0x22065d,0x22065f,0x220662,0x220664,0x220669,0x22066c,0x22066e,0x22066f,0x220674,0x220676,0x220678,0x22067a,0x22067d,0x220681,0x220683,0x220684,0x220689,0x22068a,0x22068b,0x22068c};
const std::set<std::uint32_t> pc_190460{0x190460,0x190466,0x190468,0x19046a,0x19046e,0x190474,0x190475,0x190478,0x19047a,0x19047c,0x190481,0x190485,0x19048a,0x19048f,0x190493,0x190498,0x19049b,0x19049f,0x1904a2,0x1904a6,0x1904aa,0x1904ad,0x1904b1,0x1904b5,0x1904b8,0x1904ba,0x1904bb,0x1904be,0x1904c1,0x1904c3,0x1904c4,0x1904c5,0x1904c7,0x1904c8};
const std::set<std::uint32_t> pc_18ffb0{0x18ffb0,0x18ffb1,0x18ffb7,0x18ffb9,0x18ffbb,0x18ffc0,0x18ffc2,0x18ffc5,0x18ffc8,0x18ffca,0x18ffcd,0x18ffd0,0x18ffd3,0x18ffd7,0x18ffda,0x18ffdc,0x18ffdf,0x18ffe5,0x18ffe6,0x18ffe7,0x18ffe8,0x18fff0,0x18fff3,0x18fff7,0x18fff9,0x18ffff,0x190002,0x190005,0x190008,0x19000a,0x19000c,0x19000f,0x190012,0x190015,0x190017,0x19001a,0x19001c,0x19001e,0x190020,0x190023,0x190026,0x190028,0x19002a,0x19002d,0x19002f,0x190031,0x190034,0x190036,0x190039,0x19003e,0x190040,0x190042,0x190045,0x190048,0x19004a,0x19004b,0x19004d,0x19004f,0x190051,0x190053,0x190056,0x190059,0x19005b,0x19005e,0x190061,0x190064,0x190068,0x19006b,0x19006d,0x190070,0x190076,0x190077,0x190078,0x190079,0x19007a};
const std::set<std::uint32_t> pc_18f9b0{0x18f9b0,0x18f9b6,0x18f9b7,0x18f9b8,0x18f9bd,0x18f9be,0x18f9c1,0x18f9c2,0x18f9c4,0x18f9ca,0x18f9cd,0x18f9d3,0x18f9d6,0x18f9d9,0x18f9de,0x18f9e1,0x18f9e5,0x18f9ea,0x18f9ed,0x18f9ef,0x18f9f2,0x18f9f4,0x18f9f9,0x18f9fb,0x18f9fe,0x18fa02,0x18fa05,0x18fa08,0x18fa0b,0x18fa11,0x18fa14,0x18fa1a,0x18fa1b,0x18fa21,0x18fa24,0x18fa27,0x18fa2a,0x18fa2d,0x18fa30,0x18fa32,0x18fa34,0x18fa36,0x18fa39,0x18fa3c,0x18fa3d,0x18fa41,0x18fa44,0x18fa45,0x18fa4a,0x18fa4c,0x18fa4e,0x18fa50,0x18fa54,0x18fa56,0x18fa5a,0x18fa5f,0x18fa65,0x18fa68,0x18fa6b,0x18fa6e,0x18fa73,0x18fa79,0x18fa7d,0x18fa7f,0x18fa82,0x18fa84,0x18fa87,0x18fa8a,0x18fa8d,0x18fa90,0x18fa92,0x18fa95,0x18fa9a,0x18fa9b,0x18fa9c,0x18fa9d,0x18fa9e,0x18faa4,0x18faa5,0x18faa8,0x18faab,0x18fab0,0x18fab2,0x18fab5,0x18fab7,0x18faba,0x18fabd,0x18fac0,0x18fac3,0x18fac6,0x18fac9,0x18facc,0x18facf,0x18fad5,0x18fad8,0x18fadd,0x18fae2,0x18fae7,0x18faea,0x18faeb,0x18faec,0x18faed,0x18faee,0x18faf4,0x18faf5,0x18faf8,0x18fafa,0x18fafc,0x18faff,0x18fb02,0x18fb04,0x18fb07,0x18fb09,0x18fb0c,0x18fb12,0x18fb15,0x18fb1b,0x18fb1c,0x18fb1f,0x18fb22,0x18fb25,0x18fb28,0x18fb29,0x18fb2b,0x18fb2c,0x18fb31,0x18fb32,0x18fb33,0x18fb34,0x18fb35,0x18fb3b,0x18fb3c,0x18fb3f,0x18fb42,0x18fb47,0x18fb49,0x18fb4c,0x18fb4e,0x18fb51,0x18fb54,0x18fb57,0x18fb5a,0x18fb5d,0x18fb60,0x18fb63,0x18fb66,0x18fb6b,0x18fb6d,0x18fb6f,0x18fb73,0x18fb76,0x18fb78,0x18fb7c,0x18fb83,0x18fb86,0x18fb88,0x18fb8a,0x18fb8e,0x18fb90,0x18fb93,0x18fb97,0x18fb9b,0x18fb9f,0x18fba5,0x18fba9,0x18fbab,0x18fbae,0x18fbb0,0x18fbb3,0x18fbb9,0x18fbbc,0x18fbc1,0x18fbc4,0x18fbc8,0x18fbcd,0x18fbcf,0x18fbd2,0x18fbd8,0x18fbdb,0x18fbde,0x18fbe3,0x18fbe6,0x18fbea,0x18fbf0,0x18fbf3,0x18fbf9,0x18fbfc,0x18fc04,0x18fc07,0x18fc09,0x18fc0c,0x18fc12,0x18fc15,0x18fc1d,0x18fc20,0x18fc22,0x18fc25,0x18fc2b,0x18fc2e,0x18fc36,0x18fc39,0x18fc3b,0x18fc3e,0x18fc46,0x18fc4b,0x18fc4e,0x18fc52,0x18fc5a,0x18fc5d,0x18fc5f,0x18fc62,0x18fc6a,0x18fc6f,0x18fc72,0x18fc76,0x18fc7d,0x18fc7f,0x18fc82,0x18fc88,0x18fc8b,0x18fc8f,0x18fc91,0x18fc94,0x18fc97,0x18fc9a,0x18fc9b,0x18fc9c,0x18fc9d,0x18fca2,0x18fca4,0x18fca7,0x18fcaa,0x18fcab,0x18fcae,0x18fcaf,0x18fcb0,0x18fcb5,0x18fcb7,0x18fcba,0x18fcbd,0x18fcc1,0x18fcc7,0x18fcc9,0x18fccd,0x18fccf,0x18fcd3,0x18fcd5,0x18fcd7,0x18fcda,0x18fcdf,0x18fce2,0x18fce4,0x18fce5,0x18fce6,0x18fce8,0x18fcec,0x18fcf1,0x18fcf8,0x18fcff,0x18fd02,0x18fd04,0x18fd0b,0x18fd0e,0x18fd12,0x18fd15,0x18fd1a,0x18fd1d,0x18fd20,0x18fd22,0x18fd24,0x18fd26,0x18fd29,0x18fd2b,0x18fd2e,0x18fd30,0x18fd33,0x18fd38,0x18fd3b,0x18fd3d,0x18fd40,0x18fd42,0x18fd45,0x18fd48,0x18fd4d,0x18fd50,0x18fd52,0x18fd55,0x18fd57,0x18fd5a,0x18fd5c,0x18fd60,0x18fd61,0x18fd62,0x18fd66,0x18fd6b,0x18fd6e,0x18fd72,0x18fd75,0x18fd77,0x18fd7a,0x18fd7c,0x18fd7f,0x18fd83,0x18fd85,0x18fd87,0x18fd89,0x18fd8b,0x18fd8d,0x18fd8f,0x18fd92,0x18fd94,0x18fd97,0x18fd99,0x18fd9c,0x18fd9f,0x18fda2,0x18fda4,0x18fda6,0x18fdaa,0x18fdab,0x18fdac,0x18fdad,0x18fdae,0x18fdb4};
const std::set<std::uint32_t> pc_18d5a0{0x18d5a0,0x18d5a6,0x18d5ac,0x18d5ad,0x18d5ae,0x18d5af,0x18d5b0,0x18d5b7,0x18d5b9,0x18d5be,0x18d5c1,0x18d5c6,0x18d5c9,0x18d5cc,0x18d5d0,0x18d5d6,0x18d5d8,0x18d5de,0x18d5e1,0x18d5e5,0x18d5e9,0x18d5ef,0x18d5f3,0x18d5f5,0x18d5f8,0x18d5fa,0x18d5fd,0x18d601,0x18d603,0x18d607,0x18d609,0x18d60c,0x18d60f,0x18d613,0x18d614,0x18d615,0x18d616,0x18d61b,0x18d61d,0x18d620,0x18d624,0x18d625,0x18d628,0x18d629,0x18d62a,0x18d62f,0x18d631,0x18d633,0x18d635,0x18d637,0x18d63b,0x18d640,0x18d643,0x18d647,0x18d649,0x18d64b,0x18d64f,0x18d651,0x18d655,0x18d658,0x18d65a,0x18d65d,0x18d662,0x18d666,0x18d66a,0x18d66c,0x18d66f,0x18d677,0x18d679,0x18d67d,0x18d67f,0x18d682,0x18d684,0x18d688,0x18d68a,0x18d68d,0x18d690,0x18d692,0x18d694,0x18d699,0x18d69d,0x18d6a1,0x18d6a3,0x18d6a5,0x18d6a8,0x18d6aa,0x18d6b0,0x18d6b2,0x18d6b4,0x18d6b6,0x18d6bc,0x18d6c0,0x18d6c5,0x18d6c8,0x18d6ca,0x18d6cc,0x18d6cf,0x18d6d4,0x18d6d6,0x18d6d9,0x18d6dc,0x18d6de,0x18d6e0,0x18d6e3,0x18d6e5,0x18d6e8,0x18d6ea,0x18d6ed,0x18d6f1,0x18d6f3,0x18d6fa,0x18d6ff,0x18d703,0x18d705,0x18d708,0x18d70b,0x18d70d,0x18d713,0x18d715,0x18d719,0x18d71e,0x18d720,0x18d724,0x18d727,0x18d72a,0x18d72c,0x18d72f,0x18d732,0x18d735,0x18d739,0x18d73b,0x18d73e,0x18d740,0x18d743,0x18d747,0x18d749,0x18d74c,0x18d74e,0x18d751,0x18d754,0x18d756,0x18d758,0x18d75b,0x18d75d,0x18d761,0x18d766,0x18d768,0x18d76a,0x18d76e,0x18d773,0x18d777,0x18d77b,0x18d77d,0x18d783,0x18d788,0x18d78a,0x18d78c,0x18d78f,0x18d791,0x18d793,0x18d797,0x18d799,0x18d79f,0x18d7a2,0x18d7a5,0x18d7aa,0x18d7ad,0x18d7b2,0x18d7b6,0x18d7ba,0x18d7be,0x18d7c0,0x18d7c2,0x18d7c4,0x18d7c7,0x18d7cc,0x18d7ce,0x18d7d1,0x18d7d3,0x18d7d5,0x18d7d8,0x18d7da,0x18d7df,0x18d7e2,0x18d7e6,0x18d7ea,0x18d7ed,0x18d7ef,0x18d7f0,0x18d7f1,0x18d7f5,0x18d7f6,0x18d7f7,0x18d7f8,0x18d7f9,0x18d7fa,0x18d7ff,0x18d801,0x18d803,0x18d807,0x18d80b,0x18d80f,0x18d816,0x18d81a,0x18d81c,0x18d81f,0x18d821,0x18d823,0x18d825,0x18d827,0x18d82a,0x18d82b,0x18d830,0x18d832,0x18d834,0x18d837,0x18d838,0x18d83d,0x18d840,0x18d842,0x18d847,0x18d84a,0x18d84f,0x18d852,0x18d855,0x18d858,0x18d85a,0x18d85e,0x18d864,0x18d866,0x18d86b,0x18d86c,0x18d86d,0x18d86e,0x18d86f,0x18d875,0x18d878,0x18d87b,0x18d87d,0x18d881,0x18d886,0x18d88a,0x18d88e,0x18d890,0x18d892,0x18d895,0x18d89c,0x18d89e,0x18d8a3,0x18d8a5,0x18d8a9,0x18d8ab,0x18d8b0,0x18d8b2,0x18d8b7,0x18d8bb,0x18d8bf,0x18d8c1,0x18d8c4,0x18d8c6,0x18d8c9,0x18d8cb,0x18d8d2,0x18d8d7,0x18d8db,0x18d8dc,0x18d8e0,0x18d8e5,0x18d8ea,0x18d8ec,0x18d8ef,0x18d8f2,0x18d8f4,0x18d8f9,0x18d8fb,0x18d903,0x18d906,0x18d908,0x18d90a,0x18d90d,0x18d911,0x18d913,0x18d919,0x18d91f,0x18d923,0x18d927,0x18d928,0x18d92c,0x18d92d,0x18d92f,0x18d933,0x18d934,0x18d937,0x18d938,0x18d939,0x18d93a,0x18d93b,0x18d940,0x18d942,0x18d948,0x18d94c};
const std::set<std::uint32_t> pc_220c60{0x220c60,0x220c63,0x220c64,0x220c65,0x220c69,0x220c6a,0x220c6e,0x220c71,0x220c76,0x220c79,0x220c7a,0x220c80,0x220c84,0x220c89,0x220c8b,0x220c8f,0x220c94,0x220c97,0x220c99,0x220c9f,0x220ca2,0x220ca4,0x220caa,0x220cad,0x220caf,0x220cb5,0x220cbb,0x220cbf,0x220cc1,0x220cc6,0x220cc9,0x220ccd,0x220cd1,0x220cd5,0x220cd7,0x220cda,0x220cdc,0x220cde,0x220ce2,0x220ce8,0x220cea,0x220cee,0x220cf3,0x220cf4,0x220cf5,0x220cfa,0x220cfd,0x220d01,0x220d06,0x220d0a,0x220d0c,0x220d11,0x220d13,0x220d15,0x220d17,0x220d1b,0x220d1d,0x220d1f,0x220d21,0x220d23,0x220d25,0x220d2b,0x220d2f,0x220d33,0x220d34,0x220d35,0x220d36,0x220d38,0x220d3d,0x220d40,0x220d44,0x220d48,0x220d4e,0x220d53,0x220d57,0x220d59,0x220d5e,0x220d61,0x220d67,0x220d6d,0x220d6f,0x220d73,0x220d75,0x220d77,0x220d79,0x220d7d,0x220d7f,0x220d82,0x220d87,0x220d89,0x220d8b,0x220d8f,0x220d92,0x220d97,0x220d9b,0x220d9d,0x220da3,0x220da9,0x220dac,0x220daf,0x220db1,0x220db3,0x220db9,0x220dbf,0x220dc5,0x220dcb,0x220dcd,0x220dd0,0x220dd3,0x220dd5,0x220dda,0x220ddc,0x220de0,0x220de5,0x220deb,0x220def,0x220df3,0x220df4,0x220df5,0x220df6,0x220dfb,0x220dfd,0x220e00,0x220e06,0x220e08,0x220e0b,0x220e0e,0x220e10,0x220e15,0x220e17,0x220e1b,0x220e20,0x220e26,0x220e2e,0x220e34,0x220e37,0x220e39,0x220e3f,0x220e43,0x220e47,0x220e48,0x220e49,0x220e4a,0x220e4f,0x220e53,0x220e55,0x220e5b,0x220e60,0x220e63,0x220e66,0x220e69,0x220e6b,0x220e71,0x220e75,0x220e79,0x220e7c,0x220e7e,0x220e83,0x220e89,0x220e8e,0x220e90,0x220e94,0x220e99,0x220e9b,0x220e9f,0x220ea0,0x220ea4,0x220ea9,0x220eb1,0x220eb4,0x220eb6,0x220ec0,0x220ec6,0x220ecc,0x220ecf,0x220ed0,0x220ed3,0x220ed7,0x220edb,0x220ede,0x220ee0,0x220ee3,0x220ee5,0x220ee9,0x220eeb,0x220ef1,0x220f00,0x220f06,0x220f0a,0x220f10,0x220f14,0x220f18,0x220f19,0x220f1c,0x220f1f,0x220f21,0x220f25,0x220f2a,0x220f2e,0x220f31,0x220f34,0x220f37,0x220f39,0x220f3d,0x220f3f,0x220f42,0x220f44,0x220f46,0x220f4a,0x220f4f,0x220f53,0x220f54,0x220f55,0x220f56,0x220f57,0x220f5a,0x220f5d,0x220f61,0x220f64,0x220f66,0x220f6b,0x220f73,0x220f79,0x220f80,0x220f84,0x220f88,0x220f8b,0x220f8d,0x220f90,0x220f93,0x220f97,0x220f9e,0x220fa4,0x220fa6,0x220fa9,0x220fac,0x220fb0,0x220fb4,0x220fb6,0x220fb9,0x220fbd,0x220fc1,0x220fc5,0x220fc8,0x220fcb,0x220fcd,0x220fcf,0x220fd2,0x220fd4,0x220fd7,0x220fd9,0x220fdc,0x220fde,0x220fe2,0x220fe4,0x220fe9,0x220fed,0x220fef,0x220ff1,0x220ff4,0x220ff7,0x220ffb,0x220ffd,0x221000,0x221002,0x221006,0x221008,0x22100b,0x22100d,0x221010,0x221013,0x221016,0x221019,0x22101b,0x221021,0x221024,0x221026,0x22102c,0x221030,0x221034,0x221037,0x22103b,0x22103d,0x221042,0x221046,0x221048,0x22104c,0x22104f,0x221053,0x221058,0x22105a,0x22105e,0x22105f,0x221061,0x221062,0x221063,0x221065,0x221066,0x221067,0x22106c,0x22106f,0x221074,0x221076,0x221077,0x221079,0x22107a,0x22107b,0x221080,0x221083,0x221085,0x221089,0x22108e,0x221094,0x221097,0x221099,0x22109f,0x2210a5,0x2210a7,0x2210ad,0x2210af,0x2210b4,0x2210ba,0x2210c2,0x2210c8,0x2210cc,0x2210d3,0x2210d9,0x2210e3,0x2210e7,0x2210eb,0x2210ee,0x2210ef,0x2210f0,0x2210f3,0x2210f8,0x2210fc,0x221100,0x221104,0x221107,0x22110a,0x22110d,0x22110f,0x221112,0x221118,0x22111c,0x221122,0x221125,0x221129,0x22112b,0x22112d,0x22112f,0x221132,0x221134,0x221138,0x22113c,0x22113e,0x221141,0x221143,0x221146,0x221148,0x22114a,0x22114d,0x22114f,0x221152,0x221155,0x221158,0x22115a,0x22115d,0x22115f,0x221162,0x221164,0x221166,0x221169,0x22116b,0x22116e,0x221171,0x221174,0x221176,0x221178,0x22117b,0x221182,0x221186,0x221188,0x22118c,0x221191,0x221193,0x221195,0x221198,0x22119a,0x22119e,0x2211a4,0x2211a7,0x2211ab,0x2211ad,0x2211af,0x2211b6,0x2211b8,0x2211bf,0x2211c1,0x2211c3,0x2211ca,0x2211cd,0x2211cf,0x2211d1,0x2211d3,0x2211d6,0x2211d9,0x2211db,0x2211e0,0x2211e2,0x2211e4,0x2211e6,0x2211ea,0x2211ee,0x2211f2,0x2211f4,0x2211f6,0x2211f8,0x2211ff,0x221201,0x221208,0x22120a,0x22120c,0x22120e,0x221211,0x221215,0x221218,0x22121b,0x22121d,0x221221,0x221225,0x221226,0x221228,0x221229,0x22122b,0x22122c,0x221230,0x221231,0x221236,0x221239,0x22123b,0x22123e,0x221243,0x221245,0x221249,0x22124c,0x22124f,0x221251,0x221257,0x22125b,0x22125d,0x22125f,0x221263,0x221267,0x221269,0x22126b,0x22126d,0x22126f,0x221272,0x221275,0x22127b,0x22127d,0x221283,0x221289,0x22128c,0x221291,0x221297,0x22129b,0x2212a1,0x2212a4,0x2212a9,0x2212ab,0x2212af,0x2212b2,0x2212b5,0x2212b7,0x2212bd,0x2212c1,0x2212c5,0x2212cb,0x2212ce,0x2212d0,0x2212d4,0x2212d8,0x2212dc,0x2212dd,0x2212df,0x2212e0,0x2212e3,0x2212e5,0x2212ea,0x2212ec,0x2212ef,0x2212f5,0x2212f9,0x2212fc,0x2212ff,0x221300,0x221302,0x221307,0x22130c,0x221310,0x221312,0x221315,0x221317,0x221319,0x22131b,0x22131e,0x221320,0x221323,0x221326,0x22132b,0x22132e,0x221330,0x221333,0x221339,0x22133e,0x221340,0x221346,0x221349,0x22134d,0x221351,0x221357,0x22135a,0x22135d,0x221360,0x221364,0x221367,0x22136b,0x22136f,0x221373,0x221376,0x22137b,0x22137f,0x221383,0x221388,0x22138c,0x221390,0x221393,0x221397,0x22139c,0x2213a0,0x2213a3,0x2213a7,0x2213a8,0x2213ad,0x2213af,0x2213b2,0x2213b4,0x2213b8,0x2213bc,0x2213c0,0x2213c1,0x2213c3,0x2213c4,0x2213c5,0x2213c6,0x2213c7,0x2213cc,0x2213ce,0x2213d1,0x2213d7,0x2213db,0x2213dd,0x2213e2,0x2213e4,0x2213e8,0x2213eb,0x2213ed,0x2213ef,0x2213f2,0x2213f3,0x2213f5,0x2213fa,0x2213fc,0x2213ff,0x221404,0x221407,0x22140a,0x22140d,0x221412,0x221416,0x22141c,0x221422,0x221429,0x22142a,0x22142d,0x22142f,0x221432,0x221435,0x22143b,0x22143f,0x221440,0x221441,0x221446,0x22144b,0x22144e,0x221453,0x221455,0x221459,0x22145c,0x22145e,0x221461,0x221463,0x221465,0x221469,0x22146b,0x22146e,0x221470,0x221473,0x221478,0x22147b,0x22147d,0x221480,0x221483,0x221485,0x221487,0x22148c,0x22148f,0x221492,0x221495,0x221497,0x221499,0x22149b,0x22149e,0x2214a0,0x2214a4,0x2214a7,0x2214ad,0x2214b1,0x2214b3,0x2214b5,0x2214b7,0x2214b9,0x2214be,0x2214c0,0x2214c5,0x2214cb,0x2214cc,0x2214d0,0x2214d1,0x2214d6,0x2214d9,0x2214db,0x2214dd,0x2214df,0x2214e3,0x2214e7,0x2214eb,0x2214ec,0x2214ee,0x2214ef,0x2214f1,0x2214f2,0x2214f3,0x2214f8,0x2214fa,0x2214fd,0x221500,0x221502,0x221508,0x22150b,0x221510,0x221516,0x221518,0x221520,0x221522,0x221526,0x221528,0x22152b,0x221532,0x221535,0x221539,0x22153d,0x221541,0x221544,0x221548,0x221549,0x22154b,0x22154f,0x221555,0x221559,0x22155c,0x22155e,0x221560,0x221563,0x221566,0x221569,0x22156c,0x22156e,0x221573,0x221575,0x221578,0x22157e,0x221584,0x221587,0x22158b,0x221590,0x221596,0x22159a,0x22159c,0x22159d,0x22159e,0x2215a3,0x2215a6,0x2215ac,0x2215b0,0x2215b2,0x2215b5,0x2215b7,0x2215b9,0x2215bb,0x2215bd,0x2215c0,0x2215c4,0x2215c6,0x2215c8,0x2215cb,0x2215ce,0x2215d0,0x2215d3,0x2215d5,0x2215d8,0x2215db,0x2215dd,0x2215e0,0x2215e2,0x2215e4,0x2215e6,0x2215e9,0x2215ed,0x2215ee,0x2215f1,0x2215f2,0x2215f3,0x2215f4,0x2215f7,0x2215fa,0x2215fc,0x2215ff,0x221603,0x221604,0x221607,0x221608,0x221609,0x22160a,0x22160d,0x221610,0x221611,0x221612,0x221617,0x22161b,0x22161c,0x22161d,0x221620,0x221623,0x221628,0x22162c,0x22162d,0x22162e,0x22162f,0x221630,0x221633};

// ---- memory layout ----------------------------------------------------------
constexpr std::uint32_t unmapped=0x7e000000u;
// Pointer globals (each holds a pointer into a mapped region).
constexpr std::uint32_t g_sound_globals=0x4ecc1cu, g_tag=0x4f0414u, g_sound_state=0x4f2c50u,
    g_loop_pool=0x4f9b5cu;
// Mapped regions.
constexpr std::uint32_t R_globals=0x10000000u, R_classref=0x10010000u, R_tag=0x10020000u,
    R_classdata=0x10030000u, R_state=0x10100000u, R_src=0x10200000u, R_pool=0x10300000u,
    R_rows=0x10400000u, R_bits=0x10500000u, R_pool2=0x10600000u, R_rows2=0x10610000u,
    R_pool3=0x10700000u, R_rows3=0x10710000u, R_rec3=0x10720000u, R_out=0x10800000u,
    R_defdata=0x10900000u, R_chan=0x11000000u, R_srcdef=0x11100000u, R_coll=0x11200000u,
    R_perm=0x11300000u, R_gest=0x11400000u, R_gdata=0x11410000u, R_bank=0x11500000u,
    R_emit=0x11600000u;

inline std::uint32_t rd32(const Memory&m,std::uint32_t p){return m.get32(p);}

// ---- outside-callee simulation ---------------------------------------------
// Deterministic from the call site and the live registers, so both the native
// and the lifted side compute the same result from the same state. Variety
// (sometimes -1, sometimes a small valid index) exercises both branches.
std::uint32_t g_callseq=0;   // position in the callee sequence (reset before each run)
std::uint32_t g_seed=0;      // per-state seed (identical on both sides, set in scenario)
inline std::uint32_t seqhash(std::uint32_t site){
    std::uint32_t h=(site*2654435761u) ^ (g_callseq*2246822519u) ^ (g_seed*3266489917u);
    h^=h>>15;h*=2246822519u;h^=h>>13;return h;
}
inline std::uint32_t small_or_neg(std::uint32_t h,std::uint32_t n){return (h%7u==0u)?0xffffffffu:(h%n);}

// Stdcall argument bytes popped by each simulated callee (RET N).
std::uint32_t callee_argbytes(std::uint32_t t){
    switch(t){
    case 0xbc200u: case 0x2208e0u: case 0x221a80u: case 0x222280u: case 0x129900u: return 4u;
    case 0x17e160u: case 0x223990u: case 0x198cf0u: case 0x222830u: case 0x222710u: return 8u;
    case 0xbcb70u: case 0xbad60u: case 0x1902e0u: case 0x230b10u: case 0x2218c0u:
    case 0x198d80u: case 0x12a810u: case 0x222320u: return 12u;
    case 0x222350u: return 24u;
    case 0x220c60u: return 32u;  // the loop-sound request builder, RET 32
    default: return 0u;  // RET 0 / no stack arguments
    }
}

const Services*g_services=nullptr;FP g_fp{};const Views*g_views=nullptr;
bool g_record=false;std::vector<std::array<std::uint32_t,10>> g_log;

void required_owner(SourcePhysicsContactMemory&mem,const Call&call,Abi&a,const O&){
    (void)mem;
    const auto t=call.target,site=call.instruction;
    if(g_record)g_log.push_back({site,t,a.gp[0],a.gp[1],a.gp[2],a.gp[3],a.gp[4],a.gp[5],a.gp[6],a.gp[7]});
    a.gp[4]+=4u;                                   // pop return address
    const auto h=seqhash(site);++g_callseq;
    switch(t){
    case 0x220370u: case 0x220320u: a.gp[0]=small_or_neg(h,8u); break;   // platform handle
    case 0x16ea60u: a.gp[0]=small_or_neg(h,6u); break;                   // first pool slot
    // 220C60 record/channel/permutation lookups: a small index or -1, so the
    // derived structure offsets stay inside the mapped regions and the many
    // "== -1" branches are exercised.
    case 0x223990u: case 0x2218c0u:                                     // channel index
        a.gp[0]=small_or_neg(h,6u); break;
    case 0x12a810u: case 0x222280u: case 0x222350u: case 0x222320u:
    case 0x12d320u: case 0x129900u:
        a.gp[0]=small_or_neg(h,0x30u); break;
    default: a.gp[0]=h; break;                     // generic result in EAX
    }
    a.gp[4]+=callee_argbytes(t);
}

Services make_services(std::set<std::uint32_t>*trace){
    Services s;
    s.required_owner=required_owner;
    if(trace)s.instruction=[trace](std::uint32_t pc){trace->insert(pc);};
    return s;
}

// ---- state comparison -------------------------------------------------------
std::string hexs(std::uint32_t v){char b[16];std::snprintf(b,sizeof b,"%08x",v);return b;}

std::string compare(const Abi&n,const Abi&l,Memory&nm,Memory&lm){
    static const char*names[8]={"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
    for(unsigned i=0;i<8;++i)if(n.gp[i]!=l.gp[i])
        return std::string("register ")+names[i]+" native="+hexs(n.gp[i])+" lifted="+hexs(l.gp[i]);
    for(unsigned i=0;i<8;++i)for(unsigned k=0;k<4;++k)if(n.xmm[i][k]!=l.xmm[i][k]){
        char b[64];std::snprintf(b,sizeof b,"xmm%u lane%u native=%08x lifted=%08x",i,k,n.xmm[i][k],l.xmm[i][k]);
        return b;
    }
    if(n.x87_depth!=l.x87_depth)return "x87 depth native="+std::to_string(n.x87_depth)+" lifted="+std::to_string(l.x87_depth);
    if(n.x87_status!=l.x87_status)return "x87 status native="+hexs(n.x87_status)+" lifted="+hexs(l.x87_status);
    for(unsigned i=0;i<n.x87_depth;++i)if(std::memcmp(&n.x87[i],&l.x87[i],sizeof(long double))!=0)
        return "x87 lane "+std::to_string(i)+" differs";
    if(n.floating_point.has_value()!=l.floating_point.has_value())return "floating_point presence differs";
    if(n.floating_point){
        if(n.floating_point->x87_control_word!=l.floating_point->x87_control_word)
            return "x87 control native="+hexs(n.floating_point->x87_control_word)+" lifted="+hexs(l.floating_point->x87_control_word);
        if((n.floating_point->mxcsr)!=(l.floating_point->mxcsr))
            return "mxcsr native="+hexs(n.floating_point->mxcsr)+" lifted="+hexs(l.floating_point->mxcsr);
    }
    // Memory, excluding dead stack below the caller's post-return ESP.
    const auto dead_end=l.gp[4];
    for(const auto&[page,bytes]:lm.pages){
        auto it=nm.pages.find(page);
        if(it==nm.pages.end())continue;
        const auto&other=*it->second;
        for(unsigned i=0;i<4096;++i){
            const auto p=(page<<12)+i;
            if(p<dead_end && p>=stack-0x40000u)continue;   // dead stack scratch
            if((*bytes)[i]!=other[i]){
                char b[96];std::snprintf(b,sizeof b,"memory at %08x native=%02x lifted=%02x",
                    p,std::to_integer<unsigned>(other[i]),std::to_integer<unsigned>((*bytes)[i]));
                return b;
            }
        }
    }
    return {};
}

bool agree(const Case&c,std::uint32_t entry,const Views*views,unsigned&faults,const char*label,unsigned n){
    auto nm=c.memory.clone(),lm=c.memory.clone();
    auto na=c.abi,la=c.abi;
    const auto services=make_services(nullptr);
    g_services=&services;g_fp=c.fp;g_views=views;
    std::string nf,lf;
    g_callseq=0;try{native_tu15_object_loop_run(nm,entry,na,c.fp,services,views);}catch(const std::exception&e){nf=e.what();}
    g_callseq=0;try{source_run_object_loop_audio_tu15(lm,entry,la,c.fp,services,{},views);}catch(const std::exception&e){lf=e.what();}
    std::string why;
    if(!nf.empty()||!lf.empty()){
        if(nf==lf){++faults;return true;}
        why="fault native=\""+nf+"\" lifted=\""+lf+"\"";
    } else why=compare(na,la,nm,lm);
    if(!why.empty()){
        std::cerr<<label<<" case "<<std::dec<<n<<": "<<why<<"\n";
        if(!lf.empty()){   // capture the last lifted PC before the fault
            std::uint32_t last=0;Services ts;ts.required_owner=required_owner;
            ts.instruction=[&last](std::uint32_t pc){last=pc;};
            auto mm=c.memory.clone();auto aa=c.abi;g_services=&ts;g_callseq=0;
            try{source_run_object_loop_audio_tu15(mm,entry,aa,c.fp,ts,{},views);}catch(...){}
            std::cerr<<"  last lifted pc="<<std::hex<<last<<std::dec<<"\n";
            g_services=&services;
        }
        // Diagnose: record the callee sequence on each side.
        g_record=true;
        {auto mm=c.memory.clone();auto aa=c.abi;g_log.clear();
         g_callseq=0;try{native_tu15_object_loop_run(mm,entry,aa,c.fp,services,views);}catch(...){}
         auto nlog=g_log;
         mm=c.memory.clone();aa=c.abi;g_log.clear();
         g_callseq=0;try{source_run_object_loop_audio_tu15(mm,entry,aa,c.fp,services,{},views);}catch(...){}
         auto&llog=g_log;
         const auto cnt=std::min(nlog.size(),llog.size());
         std::cerr<<"  callee seq native="<<nlog.size()<<" lifted="<<llog.size()<<"\n";
         if(entry==0x18d5a0u){const auto fr=stack-0xbcu;
             for(auto off:{0x13u,0x14u,0x1cu,0x20u,0x24u}){
                 const auto p=fr+off;
                 std::cerr<<"   local["<<std::hex<<off<<"] n="<<nm.get32(p)<<" l="<<lm.get32(p)<<std::dec<<"\n";
             }}
         for(std::size_t i=0;i<cnt;++i)std::cerr<<"   #"<<i<<" siteN="<<std::hex<<nlog[i][0]
             <<" siteL="<<llog[i][0]<<" tgt="<<nlog[i][1]
             <<" ebpN="<<nlog[i][7]<<" ebpL="<<llog[i][7]
             <<" ebxN="<<nlog[i][5]<<" ebxL="<<llog[i][5]<<std::dec<<"\n";
         for(std::size_t i=0;i<cnt;++i)if(nlog[i]!=llog[i]){
             static const char*rn[8]={"eax","ecx","edx","ebx","esp","ebp","esi","edi"};
             std::cerr<<"  first diff call#"<<i<<" site n="<<std::hex<<nlog[i][0]<<"/l="<<llog[i][0]
                 <<" target n="<<nlog[i][1]<<"/l="<<llog[i][1];
             for(unsigned k=0;k<8;++k)if(nlog[i][2+k]!=llog[i][2+k])
                 std::cerr<<" "<<rn[k]<<" n="<<nlog[i][2+k]<<"/l="<<llog[i][2+k];
             std::cerr<<std::dec<<"\n";break;
         }
        }
        g_record=false;
        return false;
    }
    return true;
}

// ---- scenario builders ------------------------------------------------------
std::uint32_t maybe_unmapped(Random&r,std::uint32_t p){return r.below(80)?p:unmapped+(r.word()&0xfff0u);}

void set_lane0(Abi&a,unsigned x,float v){a.xmm[x][0]=std::bit_cast<std::uint32_t>(v);}
float vary_float(Random&r){
    switch(r.below(8)){
    case 0:return 0.f;case 1:return -1.f;
    case 2:return 3.14159f;case 3:return 4.f;
    case 4:return r.scalar();
    default:return r.finite(10.f);
    }
}

void wide_stack(Memory&m){m.map(stack-0x40000u,0x41000u);}

// The retail .rdata constants the lifted routines read from memory (the native
// kernels fold the same values). Mapped with their TU1.5 XBE values.
void map_constants(Memory&m){
    m.put32(0x46a0d0u,0x3f800000u);   // 1.0
    m.put32(0x46a0e8u,0x00000000u);   // 0.0
    m.put32(0x46a120u,0x40800000u);   // 4.0
    m.put32(0x46a180u,0x3a83126fu);   // 0.001
    m.put32(0x46a18cu,0x3c23d70au);   // 0.01
    m.put32(0x46a1dcu,0x447a0000u);   // 1000.0
    m.put32(0x46a2ecu,0x40490fdbu);   // pi
    m.put32(0x46a584u,0x46a2f8e0u);   // 65536/2pi
    // Cluster-distance tables (0x455BA8/0x455BC4/0x455BE0, 7 entries each) and
    // the surrounding region, so sign-extended-index reads are consistent on
    // both sides. The first seven cells hold the real TU1.5 values.
    m.map(0x455800u,0x1000u);
    for(std::uint32_t p=0x455800u;p<0x456800u;p+=4u)m.put32(p,0xccccccccu);
    static const std::uint32_t t_a[7]{1,1,2,5,5,1,6},t_b[7]{3,3,2,4,4,3,6},
        t_c[7]{0x14,0x1c,0x24,0x40,0x2c,0x48,0x34};
    for(unsigned i=0;i<7;++i){m.put32(0x455ba8u+i*4u,t_a[i]);m.put32(0x455bc4u+i*4u,t_b[i]);m.put32(0x455be0u+i*4u,t_c[i]);}
}

Case scenario(Random&r,std::uint32_t entry,Views&views,const Views*&views_ptr){
    auto c=base_case(r);auto&m=c.memory;auto&a=c.abi;
    wide_stack(m);
    map_constants(m);
    g_seed=r.word();                                     // per-state callee-result seed
    views_ptr=nullptr;
    switch(entry){
    case 0x18b580u:
        set_lane0(a,1,vary_float(r));
        break;
    case 0x1904d0u:{
        m.put32(g_sound_globals,maybe_unmapped(r,R_globals));
        const auto count=r.below(5)?1u+r.below(3):0u;
        m.put32(R_globals+0xc0u,count);
        m.put32(R_globals+0xc4u,maybe_unmapped(r,R_classref));
        const auto base=count?R_classref:0u;
        const auto v=r.below(5)?(r.word()&0x7u):0xffffffffu;
        if(base==R_classref)m.put32(base+0x14u,v);
        m.put32(g_tag,maybe_unmapped(r,R_tag));
        if(v!=0xffffffffu){
            const auto idx=v&0xffffu;
            m.put32(R_tag+(idx<<4)+8u,maybe_unmapped(r,R_classdata));
            m.putf(R_classdata+0x54u,vary_float(r));
        }
        break;
    }
    case 0x220640u:{
        m.put32(g_sound_state,maybe_unmapped(r,R_state));
        for(auto off:{0x78u,0x79u,0x7au,0x7fu})m.put8(R_state+off,r.below(4)?std::uint8_t(1u+r.below(4)):0u);
        a.gp[0]=r.below(4)?0xffffffffu:r.word();          // eax = datum
        m.put32(stack+4u,r.word());                       // arg0
        break;
    }
    case 0x190460u:{
        m.put32(g_sound_state,maybe_unmapped(r,R_state));
        for(unsigned i=0;i<4;++i){
            const auto base=R_state+0x8eu+i*0x48u;
            m.put8(base,r.below(3)?std::uint8_t(1u+r.below(4)):0u);  // active byte at +0x8e
            m.putf(R_state+0xb8u+i*0x48u,vary_float(r));
            m.putf(R_state+0xbcu+i*0x48u,vary_float(r));
            m.putf(R_state+0xc0u+i*0x48u,vary_float(r));
        }
        a.gp[6]=R_src;                                    // esi = source position
        m.putf(R_src,vary_float(r));m.putf(R_src+4u,vary_float(r));m.putf(R_src+8u,vary_float(r));
        set_lane0(a,3,std::fabs(vary_float(r)));          // xmm3 threshold
        if(r.below(2)){                                   // half the states use native views
            for(unsigned i=0;i<4;++i){
                if(r.below(4))views[i].reset();
                else views[i]=SourceObjectAudioViewTu15{{vary_float(r),vary_float(r),vary_float(r)},
                    std::int16_t(r.word())};
            }
            views_ptr=&views;
        }
        break;
    }
    case 0x18ffb0u:{
        m.put32(g_loop_pool,maybe_unmapped(r,R_pool));
        m.put32(R_pool+0x24u,0x18u);                      // stride
        m.put32(R_pool+0x44u,maybe_unmapped(r,R_rows));   // rows
        const auto cap=0x20u;
        m.put32(R_pool+0x38u,cap);                        // capacity
        m.put32(R_pool+0x48u,maybe_unmapped(r,R_bits));   // bitfield
        m.map(R_bits,0x40u);
        for(unsigned w=0;w<cap/32u+1u;++w)m.put32(R_bits+w*4u,r.word());
        for(unsigned i=0;i<cap+2u;++i){
            const auto rec=R_rows+i*0x18u;
            m.put16(rec,std::uint16_t(r.word()));          // datum word
            m.put8(rec+4u,std::uint8_t(r.word()));         // flags
            m.put32(rec+0xcu,r.below(3)?0u:r.word());      // owner
        }
        m.put32(stack+4u,r.below(3)?0u:r.word());          // arg0
        break;
    }
    case 0x18f9b0u:{
        const auto idx=r.below(12);
        a.gp[0]=idx;                                      // loop slot index (AX)
        a.gp[1]=R_out;                                    // output emitter ptr (edi)
        m.map(R_out,0x100u);for(unsigned k=0;k<0x100u;k+=4u)m.put32(R_out+k,r.word());
        // Loop pool and its 0x18-byte records.
        m.put32(g_loop_pool,maybe_unmapped(r,R_pool));
        m.put32(R_pool+0x44u,maybe_unmapped(r,R_rows));
        const auto rec=R_rows+idx*0x18u;
        const auto mode=std::uint8_t(r.below(5));          // 0..4 spatial modes
        const auto objidx=r.below(12);
        m.put16(rec+0xcu,std::uint16_t(r.below(16)));       // def datum
        m.put8(rec+3u,mode);
        m.put8(rec+4u,std::uint8_t(r.word()));
        m.put32(rec+0x10u,objidx);
        m.put32(rec+0x14u,r.word());
        m.put32(rec+8u,r.word());
        m.put16(rec+6u,r.below(4)?0xffffu:std::uint16_t(r.below(16)));
        m.put8(rec+0x15u,std::uint8_t(r.word()));
        m.put16(rec+0x16u,r.below(4)?0xffffu:std::uint16_t(r.below(16)));
        // Tag instances and definition data.
        m.put32(g_tag,maybe_unmapped(r,R_tag));
        const auto defdatum=std::uint32_t(m.get16(rec+0xcu));
        m.put32(R_tag+(defdatum<<4)+8u,maybe_unmapped(r,R_defdata));
        m.map(R_defdata,0x20u);m.put32(R_defdata,r.word());  // [ebp] flags
        // Zero vector .rdata and the 0x44ebd8 word the position path reads.
        m.map(0x44e000u,0x2000u);m.put32(0x44ebd8u,0u);
        // Globals the mode-2 and clear paths read (values passed to simulated
        // callees); 0x52d68c indirects through a pool to its +0x44 rows.
        m.put32(0x52d68cu,maybe_unmapped(r,R_pool2));m.put32(R_pool2+0x44u,r.word());
        m.put32(0x474ce8u,r.word());m.put32(0x474d10u,r.word());
        // Pool2 (0x4f720c) with 0x190-byte records.
        m.put32(0x4f720cu,maybe_unmapped(r,R_pool2));
        m.put32(R_pool2+0x44u,maybe_unmapped(r,R_rows2));
        m.map(R_rows2,0x30u+16u*0x190u);
        for(unsigned i=0;i<16u;++i){const auto r2=R_rows2+i*0x190u+0x30u;m.put32(r2,r.word());m.put32(r2+4u,r.word());m.put32(r2+8u,r.word());}
        // Pool3 (0x4ecbd0): 0xc-byte entries, +3 class byte, +8 record pointer.
        m.put32(0x4ecbd0u,maybe_unmapped(r,R_pool3));
        m.put32(R_pool3+0x44u,maybe_unmapped(r,R_rows3));
        m.map(R_rows3,16u*0xcu*3u);
        for(unsigned i=0;i<16u*3u;++i){
            const auto e=R_rows3+i*0xcu;
            m.put8(e+3u,r.below(3)?10u:std::uint8_t(r.word()));  // class 10 enables the param block
            m.put32(e+8u,maybe_unmapped(r,R_rec3+i*0x200u));
        }
        m.map(R_rec3,16u*3u*0x200u);
        for(unsigned i=0;i<16u*3u;++i){
            const auto rc=R_rec3+i*0x200u;
            m.put8(rc+0x12cu,std::uint8_t(r.word()));
            m.put32(rc+0x130u,r.word());
            for(auto off:{0x134u,0x138u,0x13cu,0x140u,0x144u})m.putf(rc+off,vary_float(r));
        }
        break;
    }
    case 0x18d5a0u:{
        const auto idx=r.below(12);
        a.gp[0]=idx;                                      // input eax unused; arg0 is on stack
        m.put32(stack+4u,idx);                            // arg0 = loop index
        // Loop pool + record (shared with 18f9b0/18ffb0).
        m.put32(g_loop_pool,maybe_unmapped(r,R_pool));
        m.put32(R_pool+0x44u,maybe_unmapped(r,R_rows));
        m.put32(R_pool+0x24u,0x18u);m.put32(R_pool+0x38u,0x20u);
        m.put32(R_pool+0x48u,maybe_unmapped(r,R_bits));
        m.map(R_bits,0x40u);for(unsigned w=0;w<2u;++w)m.put32(R_bits+w*4u,r.word());
        for(unsigned i=0;i<34u;++i){                       // 0x18-byte records
            const auto rec=R_rows+i*0x18u;
            m.put16(rec,std::uint16_t(r.word()));           // datum word (18ffb0)
            m.put16(rec+0xcu,std::uint16_t(r.below(16)));   // def datum
            m.put8(rec+2u,std::uint8_t(r.word()));          // state byte
            m.put8(rec+3u,std::uint8_t(r.below(5)));        // spatial mode
            m.put16(rec+4u,std::uint16_t(r.word()));        // flags word
            m.put8(rec+5u,std::uint8_t(r.word()));          // flags byte
            m.put16(rec+6u,r.below(4)?0xffffu:std::uint16_t(r.below(16)));
            m.put32(rec+8u,r.word());
            m.put32(rec+0x10u,r.below(12));                 // object datum (small)
            m.put32(rec+0x14u,r.word());
            m.put8(rec+0x15u,std::uint8_t(r.word()));
            m.put16(rec+0x16u,r.below(4)?0xffffu:std::uint16_t(r.below(16)));
        }
        // Tag instances + definition data.
        m.put32(g_tag,maybe_unmapped(r,R_tag));
        for(unsigned d=0;d<16u;++d)m.put32(R_tag+(d<<4)+8u,maybe_unmapped(r,R_defdata+d*0x100u));
        m.map(R_defdata,0x1100u);for(unsigned k=0;k<0x1100u;k+=4u)m.put32(R_defdata+k,r.word());
        // Zero vector + mode-2/clear globals (18f9b0).
        m.map(0x44e000u,0x2000u);m.put32(0x44ebd8u,0u);
        m.put32(0x52d68cu,maybe_unmapped(r,R_pool2));m.put32(R_pool2+0x44u,r.word());
        m.put32(0x474ce8u,r.word());m.put32(0x474d10u,r.word());
        // Pool2/Pool3 for 18f9b0.
        m.put32(0x4f720cu,maybe_unmapped(r,R_pool2));m.put32(R_pool2+0x44u,maybe_unmapped(r,R_rows2));
        m.map(R_rows2,0x30u+16u*0x190u);
        for(unsigned i=0;i<16u;++i){const auto r2=R_rows2+i*0x190u+0x30u;m.put32(r2,r.word());m.put32(r2+4u,r.word());m.put32(r2+8u,r.word());}
        m.put32(0x4ecbd0u,maybe_unmapped(r,R_pool3));m.put32(R_pool3+0x44u,maybe_unmapped(r,R_rows3));
        m.map(R_rows3,16u*0xcu*3u);
        for(unsigned i=0;i<16u*3u;++i){const auto e=R_rows3+i*0xcu;
            m.put8(e+3u,r.below(3)?10u:std::uint8_t(r.word()));m.put32(e+8u,maybe_unmapped(r,R_rec3+i*0x200u));}
        m.map(R_rec3,16u*3u*0x200u);
        for(unsigned i=0;i<16u*3u;++i){const auto rc=R_rec3+i*0x200u;
            m.put8(rc+0x12cu,std::uint8_t(r.word()));m.put32(rc+0x130u,r.word());
            for(auto off:{0x134u,0x138u,0x13cu,0x140u,0x144u})m.putf(rc+off,vary_float(r));}
        // 1904d0 chain (sound globals).
        m.put32(g_sound_globals,maybe_unmapped(r,R_globals));
        m.put32(R_globals+0xc0u,r.below(5)?1u+r.below(3):0u);
        m.put32(R_globals+0xc4u,maybe_unmapped(r,R_classref));
        m.put32(R_classref+0x14u,r.below(5)?(r.word()&0x7u):0xffffffffu);
        m.put32(R_classdata+0x54u,std::bit_cast<std::uint32_t>(vary_float(r)));
        for(unsigned d=0;d<8u;++d)m.put32(R_tag+(d<<4)+8u,maybe_unmapped(r,R_classdata));
        // Sound-state (190460 views + 220640 device bytes).
        m.put32(g_sound_state,maybe_unmapped(r,R_state));
        for(auto off:{0x78u,0x79u,0x7au,0x7fu})m.put8(R_state+off,r.below(4)?std::uint8_t(1u+r.below(4)):0u);
        for(unsigned i=0;i<4;++i){
            m.put8(R_state+0x8eu+i*0x48u,r.below(3)?std::uint8_t(1u+r.below(4)):0u);
            m.putf(R_state+0xb8u+i*0x48u,vary_float(r));m.putf(R_state+0xbcu+i*0x48u,vary_float(r));
            m.putf(R_state+0xc0u+i*0x48u,vary_float(r));
        }
        // Globals read directly by 18d5a0.
        m.put32(0x51df50u,r.below(3)?0u:maybe_unmapped(r,R_defdata));
        if(r.below(2)){
            for(unsigned i=0;i<4;++i){
                if(r.below(4))views[i].reset();
                else views[i]=SourceObjectAudioViewTu15{{vary_float(r),vary_float(r),vary_float(r)},std::int16_t(r.word())};
            }
            views_ptr=&views;
        }
        break;
    }
    case 0x220c60u:{
        const auto src=r.below(10)?r.below(8):0xffffffffu,chan=r.below(8);
        // 8 stack arguments.
        m.put32(stack+4u,src);                            // arg0 source index (sometimes -1)
        m.put32(stack+8u,chan);                           // arg1 channel index (cx)
        m.put32(stack+0xcu,r.below(2));                   // arg2
        m.put32(stack+0x10u,r.word());                    // arg3
        m.put32(stack+0x14u,maybe_unmapped(r,R_emit));    // arg4 emitter/request ptr (edx early)
        m.put32(stack+0x18u,r.below(4));                  // arg5 mode (esi)
        m.put32(stack+0x1cu,std::uint8_t(r.word()));      // arg6 flags byte
        m.put32(stack+0x20u,std::bit_cast<std::uint32_t>(vary_float(r))); // arg7
        // Emitter/request struct at arg4.
        m.map(R_emit,0x80u);for(unsigned k=0;k<0x80u;k+=4u)m.put32(R_emit+k,r.word());
        m.put8(R_emit,std::uint8_t(r.word()));m.put8(R_emit+3u,std::uint8_t(r.word()));
        // Tag instances + source definition.
        m.put32(g_tag,maybe_unmapped(r,R_tag));
        for(unsigned d=0;d<8u;++d)m.put32(R_tag+(d<<4)+8u,maybe_unmapped(r,R_srcdef));
        m.map(R_srcdef,0x60u);for(unsigned k=0;k<0x60u;k+=4u)m.put32(R_srcdef+k,r.word());
        m.put8(R_srcdef,std::uint8_t(r.word()));
        const auto perms=r.below(4);                      // small permutation count
        m.put32(R_srcdef+0x1cu,perms);
        m.put32(R_srcdef+0x20u,maybe_unmapped(r,R_perm));
        // Permutation records (0x58 bytes each).
        m.map(R_perm,8u*0x58u);for(unsigned k=0;k<8u*0x58u;k+=4u)m.put32(R_perm+k,r.word());
        for(unsigned i=0;i<8u;++i){const auto p=R_perm+i*0x58u;
            m.put8(p+4u,std::uint8_t(r.word()));
            m.put32(p+0x18u,r.below(3)?0xffffffffu:r.word());
            m.put32(p+0x28u,r.word());}
        // Class runtime channel rows (6 records; 223990/2218c0 index them).
        m.put32(0x50ebe8u,maybe_unmapped(r,R_gest));m.put32(R_gest+0x44u,maybe_unmapped(r,R_chan));
        const auto chanspan=8u*0xd4u+0x200u;
        m.map(R_chan,chanspan);for(unsigned k=0;k<chanspan;k+=4u)m.put32(R_chan+k,r.word());
        // Permutation slots at channel+0x88+i*0x14: sometimes -1 so the
        // slot-init / pitch-gain path (L221330) is reached.
        for(unsigned rec=0;rec<6u;++rec)for(unsigned i=0;i<4u;++i)
            m.put32(R_chan+rec*0xd4u+0x88u+i*0x14u,r.below(2)?0xffffffffu:r.word());
        // Sound-state + banks + gestalt + random seed.
        m.put32(g_sound_state,maybe_unmapped(r,R_state));
        for(auto off:{0x78u,0x79u,0x7au})m.put8(R_state+off,r.below(6)?std::uint8_t(1u+r.below(4)):0u);
        m.put8(R_state+0x7fu,std::uint8_t(r.word()));
        m.put32(0x4f2c4cu,maybe_unmapped(r,R_bank));m.put32(R_bank+0x44u,maybe_unmapped(r,R_coll));
        m.map(R_coll,0x8000u);for(unsigned k=0;k<0x8000u;k+=4u)m.put32(R_coll+k,r.word());
        m.put32(0x4f3cd8u,r.word());
        m.put32(0x52d664u,maybe_unmapped(r,R_gdata));
        m.put32(R_gdata+4u,maybe_unmapped(r,R_coll));m.put32(R_gdata+0xcu,maybe_unmapped(r,R_coll));
        m.map(R_gdata,0x40u);
        a.gp[0]=src;
        break;
    }
    default:break;
    }
    return c;
}

int run(std::uint32_t entry,const char*label,const std::set<std::uint32_t>&pcs,unsigned iterations){
    Random r(0xa10c0000u^entry);
    std::set<std::uint32_t> covered;
    unsigned faults=0;
    for(unsigned n=0;n<iterations;++n){
        Views views{};const Views*vp=nullptr;
        const auto c=scenario(r,entry,views,vp);
        if(!agree(c,entry,vp,faults,label,n))return 1;
        // Coverage pass on a clone (lifted trace), ignoring faults.
        std::set<std::uint32_t> path;
        auto lm=c.memory.clone();auto la=c.abi;
        const auto services=make_services(&path);
        g_services=&services;g_fp=c.fp;g_views=vp;g_callseq=0;
        try{source_run_object_loop_audio_tu15(lm,entry,la,c.fp,services,{},vp);}catch(...){}
        covered.insert(path.begin(),path.end());
    }
    unsigned hit=0;for(auto pc:pcs)if(covered.count(pc))++hit;
    std::cout<<std::hex<<entry<<std::dec<<" ("<<label<<"): "<<iterations<<" states, "<<faults
        <<" identical faults, coverage "<<hit<<"/"<<pcs.size()<<"\n";
    if(hit<pcs.size()){
        std::cerr<<"  uncovered:";for(auto pc:pcs)if(!covered.count(pc))std::cerr<<" "<<std::hex<<pc;
        std::cerr<<std::dec<<"\n";
    }
    return 0;
}

} // namespace

int main(int argc,char**argv){
    const std::string which=argc>1?argv[1]:"all";
    struct Entry{std::uint32_t addr;const char*name;const std::set<std::uint32_t>*pcs;unsigned iters;};
    const Entry entries[]{
        {0x18b580u,"18b580",&pc_18b580,20000u},
        {0x1904d0u,"1904d0",&pc_1904d0,20000u},
        {0x220640u,"220640",&pc_220640,20000u},
        {0x190460u,"190460",&pc_190460,30000u},
        {0x18ffb0u,"18ffb0",&pc_18ffb0,40000u},
        {0x18f9b0u,"18f9b0",&pc_18f9b0,20000u},
        {0x18d5a0u,"18d5a0",&pc_18d5a0,20000u},
        {0x220c60u,"220c60",&pc_220c60,60000u},
    };
    for(const auto&e:entries){
        if(which!="all" && which!=e.name)continue;
        if(run(e.addr,e.name,*e.pcs,e.iters))return 1;
    }
    std::cout<<"PASS native object-loop sources kernels\n";
    return 0;
}
