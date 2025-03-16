
#include "Core/Memory/MemoryDefragmenter.h"

#include <algorithm>
#include <cstring>

#include "Core/Assert.h"
#include "Log/LogSystem.h"

namespace Engine {
    namespace Memory {

        void MemoryDefragmenter::MoveBlock(void *source,
                                           void *destination,
                                           size_t size) {
            if (source == destination) return;

            // Use memmove to handle potential overlapping regions
            std::memmove(destination, source, size);

            // Update stats
            stats.totalMoves++;
            stats.totalMoved += size;
        }

        float MemoryDefragmenter::CalculateBlockPriority(
            const Block &block) const {
            // Calculate priority based on multiple factors
            float sizePriority =
                static_cast<float>(block.size) / config.maxMoveSize;
            float fragmentationPriority = 0.0f;

            // Check surrounding blocks for fragmentation
            void *blockEnd = static_cast<char *>(block.address) + block.size;
            auto it = std::find_if(
                blocks.begin(), blocks.end(), [blockEnd](const Block &b) {
                    return b.address > blockEnd;
                });

            if (it != blocks.end()) {
                size_t gap = static_cast<char *>(it->address) -
                             static_cast<char *>(blockEnd);
                fragmentationPriority =
                    static_cast<float>(gap) / config.maxMoveSize;
            }

            // Combine factors (weights can be adjusted)
            return sizePriority * 0.4f + fragmentationPriority * 0.6f;
        }

        std::vector<MemoryDefragmenter::FragmentInfo>
        MemoryDefragmenter::AnalyzeFragmentation() const {
            std::lock_guard<std::mutex> lock(mutex);
            std::vector<FragmentInfo> fragments;

            if (blocks.empty()) return fragments;

            // Sort blocks by address
            std::vector<Block> sortedBlocks = blocks;
            std::sort(sortedBlocks.begin(),
                      sortedBlocks.end(),
                      [](const Block &a, const Block &b) {
                          return a.address < b.address;
                      });

            // Find gaps between blocks
            for (size_t i = 0; i < sortedBlocks.size() - 1; ++i) {
                void *currentEnd =
                    static_cast<char *>(sortedBlocks[i].address) +
                    sortedBlocks[i].size;
                void *nextStart = sortedBlocks[i + 1].address;

                if (currentEnd < nextStart) {
                    size_t gap = static_cast<char *>(nextStart) -
                                 static_cast<char *>(currentEnd);

                    if (gap >= config.minFragmentSize) {
                        FragmentInfo info;
                        info.address = currentEnd;
                        info.size = gap;
                        info.canMove = sortedBlocks[i].isMovable;

                        // Calculate fragmentation score based on size and
                        // position
                        float sizeScore =
                            static_cast<float>(gap) / config.maxMoveSize;
                        float positionScore =
                            static_cast<float>(i) / sortedBlocks.size();
                        info.score = sizeScore * 0.7f + positionScore * 0.3f;

                        fragments.push_back(info);
                    }
                }
            }

            return fragments;
        }

        void MemoryDefragmenter::BackgroundThreadFunction() {
            while (!shouldStop) {
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    condition.wait_for(
                        lock,
                        std::chrono::milliseconds(config.defragInterval),
                        [this]() { return shouldStop || !isPaused; });

                    if (shouldStop) break;
                    if (isPaused) continue;
                }

                // Check fragmentation level
                float fragScore = CalculateFragmentationScore();
                if (fragScore > config.fragmentationThreshold) {
                    ProcessIncrementalDefrag();
                }
            }
        }

    }  // namespace Memory
}  // namespace Engine
