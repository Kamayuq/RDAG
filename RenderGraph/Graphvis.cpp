#include "Graphvis.h"

#if __clang__
constexpr const char* ColorStyle::ColorChart[];
constexpr const char* DrawStyle::StyleChart[];
constexpr const char* DrawStyle::ShapeChart[];
constexpr const char* ArrowStyle::HeadChart[];
constexpr const char* ArrowStyle::DirChart[];
#endif

U32 ActionStyle::GlobalActionIndex = 0;
