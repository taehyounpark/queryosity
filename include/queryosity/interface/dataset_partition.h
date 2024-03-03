#pragma once

#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "dataset.h"
#include "multithread.h"

namespace queryosity {

namespace dataset {

namespace partition {

std::vector<range> truncate(std::vector<range> const &parts,
                            long long nentries_max);

std::vector<range> merge(std::vector<range> const &parts,
                         unsigned int nslots_max);

} // namespace partition

} // namespace dataset

} // namespace queryosity

inline std::vector<queryosity::dataset::range>
queryosity::dataset::partition::merge(
    std::vector<queryosity::dataset::range> const &parts,
    unsigned int nslots_max) {

  // no merging needed
  if (nslots_max >= static_cast<unsigned int>(parts.size()))
    return parts;

  assert(!parts.empty() && nslots_max > 0);

  std::vector<range> parts_merged;

  const unsigned int total_size = parts.back().second - parts.front().first;
  const unsigned int size_per_slot = total_size / nslots_max;
  const unsigned int extra_size =
      total_size % nslots_max; // Extra units to distribute across slots

  unsigned int current_start = parts[0].first;
  unsigned int current_end = current_start;
  unsigned int accumulated_size = 0;
  unsigned int nslots_created = 0;

  for (const auto &part : parts) {
    unsigned int part_size = part.second - part.first;
    if (accumulated_size + part_size >
            size_per_slot + (nslots_created < extra_size ? 1 : 0) &&
        nslots_created < nslots_max - 1) {
      // Finalize the current slot before it exceeds the average size too much
      parts_merged.emplace_back(current_start, current_end);
      current_start = current_end;
      accumulated_size = 0;
      ++nslots_created;
    }

    // Add part size to the current slot
    accumulated_size += part_size;
    current_end += part_size;

    // Handle the last slot differently to include all remaining parts
    if (nslots_created == nslots_max - 1) {
      parts_merged.emplace_back(current_start, parts.back().second);
      break; // All parts have been processed
    }
  }

  // Ensure we have exactly nslots_max slots
  if (static_cast<unsigned int>(parts_merged.size()) < nslots_max) {
    parts_merged.emplace_back(current_start, parts.back().second);
  }

  return parts_merged;
}

inline std::vector<queryosity::dataset::range>
queryosity::dataset::partition::truncate(
    std::vector<queryosity::dataset::range> const &parts,
    long long nentries_max) {
  if (nentries_max < 0)
    return parts;

  std::vector<range> parts_truncated;

  for (const auto &part : parts) {
    auto part_end = nentries_max >= 0
                        ? std::min(part.first + nentries_max, part.second)
                        : part.second;
    parts_truncated.emplace_back(part.first, part_end);
    nentries_max -= (part_end - part.first);
    if (!nentries_max)
      break;
  }

  return parts_truncated;
}