#pragma once
namespace halo2 {
// Complete TU1.5 callers enter C1120 with 1/1 for automatic placement and
// 0/0 for runtime creation. D75A0 uses this context to select creation flag8;
// B9B70 projects that flag to object+C1bit0. It is not a post-birth override.
enum class SourceScenarioBirth { automatic, runtime };
}
