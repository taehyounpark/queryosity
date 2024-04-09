#pragma once

#include <cassert>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

#include "dataset.h"
#include "multithread.h"

namespace queryosity {

namespace dataset {

using entry_t = unsigned long long;

using partition_t = std::vector<part_t>;

using slot_t = unsigned int;

namespace partition {

partition_t align(std::vector<partition_t> const &partitions);

partition_t truncate(partition_t const &parts, long long nentries_max);

} // namespace partition

} // namespace dataset

} // namespace queryosity

inline queryosity::dataset::partition_t queryosity::dataset::partition::align(
    std::vector<partition_t> const &partitions) {
  std::map<entry_t, unsigned int> edge_counts;
  const unsigned int num_vectors = partitions.size();

  // Count appearances of each edge
  for (auto const &vec : partitions) {
    std::map<entry_t, bool>
        seen_edges; // Ensure each edge is only counted once per vector
    for (auto const &p : vec) {
      if (seen_edges.find(p.first) == seen_edges.end()) {
        edge_counts[p.first]++;
        seen_edges[p.first] = true;
      }
      if (seen_edges.find(p.second) == seen_edges.end()) {
        edge_counts[p.second]++;
        seen_edges[p.second] = true;
      }
    }
  }

  // Filter edges that appear in all vectors
  std::vector<entry_t> aligned_edges;
  for (auto const &pair : edge_counts) {
    if (pair.second == num_vectors) {
      aligned_edges.push_back(pair.first);
    }
  }

  // Create aligned vector of pairs
  std::vector<std::pair<entry_t, entry_t>> aligned_ranges;
  for (size_t i = 0; i < aligned_edges.size() - 1; ++i) {
    aligned_ranges.emplace_back(aligned_edges[i], aligned_edges[i + 1]);
  }

  return aligned_ranges;
}

inline queryosity::dataset::partition_t
queryosity::dataset::partition::truncate(
    queryosity::dataset::partition_t const &parts, long long nentries_max) {
  if (nentries_max < 0)
    return parts;

  partition_t parts_truncated;

  for (auto const &part : parts) {
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