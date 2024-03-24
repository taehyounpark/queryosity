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

using partition_t = std::vector<part_t>;

namespace partition {

partition_t align(std::vector<partition_t> const &partitions);

partition_t truncate(partition_t const &parts, long long nentries_max);

partition_t merge(partition_t const &parts, unsigned int nslots_max);

} // namespace partition

} // namespace dataset

} // namespace queryosity

inline queryosity::dataset::partition_t queryosity::dataset::partition::align(
    std::vector<partition_t> const &partitions) {
  std::map<unsigned long long, unsigned int> edge_counts;
  const unsigned int num_vectors = partitions.size();

  // Count appearances of each edge
  for (const auto &vec : partitions) {
    std::map<unsigned long long, bool>
        seen_edges; // Ensure each edge is only counted once per vector
    for (const auto &p : vec) {
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
  std::vector<unsigned long long> aligned_edges;
  for (const auto &pair : edge_counts) {
    if (pair.second == num_vectors) {
      aligned_edges.push_back(pair.first);
    }
  }

  // Create aligned vector of pairs
  std::vector<std::pair<unsigned long long, unsigned long long>> aligned_ranges;
  for (size_t i = 0; i < aligned_edges.size() - 1; ++i) {
    aligned_ranges.emplace_back(aligned_edges[i], aligned_edges[i + 1]);
  }

  return aligned_ranges;
}

inline queryosity::dataset::partition_t queryosity::dataset::partition::merge(
    queryosity::dataset::partition_t const &parts, unsigned int nslots_max) {

  // no merging needed
  if (nslots_max >= static_cast<unsigned int>(parts.size()))
    return parts;

  assert(!parts.empty() && nslots_max > 0);

  partition_t parts_merged;

  const unsigned int total_size = parts.back().second - parts.front().first;
  const unsigned int size_per_slot = total_size / nslots_max;
  const unsigned int extra_size = total_size % nslots_max;

  unsigned int current_start = parts[0].first;
  unsigned int current_end = current_start;
  unsigned int accumulated_size = 0;
  unsigned int nslots_created = 0;

  for (const auto &part : parts) {
    unsigned int part_size = part.second - part.first;
    // check if another part can be added
    if (accumulated_size + part_size >
            size_per_slot + (nslots_created < extra_size ? 1 : 0) &&
        nslots_created < nslots_max - 1) {
      // add the current range if adding next part will exceed the average size
      parts_merged.emplace_back(current_start, current_end);
      current_start = current_end;
      accumulated_size = 0;
      ++nslots_created;
    }

    // add part size to the current slot
    accumulated_size += part_size;
    current_end += part_size;

    // handle the last slot differently to include all remaining parts
    if (nslots_created == nslots_max - 1) {
      parts_merged.emplace_back(current_start, parts.back().second);
      break; // All parts have been processed
    }
  }

  // ensure we have exactly nslots_max slots
  if (static_cast<unsigned int>(parts_merged.size()) < nslots_max) {
    parts_merged.emplace_back(current_start, parts.back().second);
  }

  return parts_merged;
}

inline queryosity::dataset::partition_t
queryosity::dataset::partition::truncate(
    queryosity::dataset::partition_t const &parts, long long nentries_max) {
  if (nentries_max < 0)
    return parts;

  partition_t parts_truncated;

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