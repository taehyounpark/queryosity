#pragma once

#include "dataset.h"
#include "dataset_player.h"
#include "multithread.h"

namespace queryosity {

namespace dataset {

class processor : public multithread::core, public ensemble::slotted<player> {
public:
  processor(int suggestion);
  virtual ~processor() = default;

  processor(const processor &) = delete;
  processor &operator=(const processor &) = delete;

  processor(processor &&) noexcept = default;
  processor &operator=(processor &&) noexcept = default;

  template <typename DS, typename Val>
  auto read(dataset::reader<DS> &ds, const std::string &column_name)
      -> std::vector<read_column_t<DS, Val> *>;

  void downsize(unsigned int nslots);
  void process(std::vector<std::unique_ptr<source>> const &sources,
               double scale, unsigned long long nrows);

  virtual std::vector<player *> const &get_slots() const override;

protected:
  std::vector<unsigned int> m_range_slots;
  std::vector<dataset::player> m_players;
  std::vector<dataset::player *> m_player_ptrs;
};

} // namespace dataset

namespace multithread {

dataset::processor enable(int suggestion = -1);
dataset::processor disable();

} // namespace multithread

} // namespace queryosity

inline queryosity::dataset::processor
queryosity::multithread::enable(int suggestion) {
  return dataset::processor(suggestion);
}

inline queryosity::dataset::processor queryosity::multithread::disable() {
  return dataset::processor(false);
}

inline queryosity::dataset::processor::processor(int suggestion)
    : multithread::core::core(suggestion) {
  const auto nslots = this->concurrency();
  m_players = std::vector<player>(nslots);
  m_player_ptrs = std::vector<player *>(nslots, nullptr);
  std::transform(m_players.begin(), m_players.end(), m_player_ptrs.begin(),
                 [](player &plyr) -> player * { return &plyr; });
  m_range_slots.clear();
  m_range_slots.reserve(nslots);
  for (unsigned int i = 0; i < nslots; ++i) {
    m_range_slots.push_back(i);
  }
}

template <typename DS, typename Val>
auto queryosity::dataset::processor::read(dataset::reader<DS> &ds,
                                          const std::string &column_name)
    -> std::vector<read_column_t<DS, Val> *> {
  return ensemble::invoke(
      [column_name, &ds](dataset::player *plyr, unsigned int slot) {
        return plyr->template read<DS, Val>(ds, slot, column_name);
      },
      m_player_ptrs, m_range_slots);
}

inline void queryosity::dataset::processor::downsize(unsigned int nslots) {
  if (nslots > this->size()) {
    throw std::logic_error("requested thread count too high");
  };
  while (m_players.size() > nslots) {
    // avoid copy-constructor
    m_players.pop_back();
  }
  m_player_ptrs.resize(nslots);
  m_range_slots.resize(nslots);
}

inline void queryosity::dataset::processor::process(
    std::vector<std::unique_ptr<source>> const &sources, double scale,
    unsigned long long nrows) {

  const auto nslots = this->concurrency();

  // 1. enter event loop
  for (auto const &ds : sources) {
    ds->initialize();
  }

  // 2. partition dataset(s)
  // 2.1 get partition from each dataset source
  std::vector<partition_t> partitions_from_sources;
  for (auto const &ds : sources) {
    auto partition_from_source = ds->partition();
    if (partition_from_source.size())
      partitions_from_sources.push_back(std::move(partition_from_source));
  }
  if (!partitions_from_sources.size()) {
    throw std::runtime_error("no valid dataset partition found");
  }
  // 2.2 find common denominator partition
  const auto partition_aligned =
      dataset::partition::align(partitions_from_sources);
  // 2.3 truncate entries to row limit
  const auto partition_truncated =
      dataset::partition::truncate(partition_aligned, nrows);
  // 2.3 distribute partition amongst threads
  std::vector<partition_t> partitions_for_slots(nslots);
  auto nparts_remaining = partition_truncated.size();
  const auto nparts = nparts_remaining;
  while (nparts_remaining) {
    for (unsigned int islot = 0; islot < nslots; ++islot) {
      partitions_for_slots[islot].push_back(
          std::move(partition_truncated[nparts - (nparts_remaining--)]));
      if (!nparts_remaining)
        break;
    }
  }
  // todo: can intel tbb distribute slots during parallel processing?

  // 3. run event loop
  this->run(
      [&sources, scale](
          dataset::player *plyr, unsigned int slot,
          std::vector<std::pair<unsigned long long, unsigned long long>> const
              &parts) { plyr->play(sources, scale, slot, parts); },
      m_player_ptrs, m_range_slots, partitions_for_slots);

  // 4. exit event loop
  for (auto const &ds : sources) {
    ds->finalize();
  }
}

inline std::vector<queryosity::dataset::player *> const &
queryosity::dataset::processor::get_slots() const {
  return m_player_ptrs;
}