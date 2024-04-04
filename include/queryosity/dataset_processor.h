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
  std::vector<std::vector<std::pair<unsigned long long, unsigned long long>>>
      partitions;
  for (auto const &ds : sources) {
    auto partition = ds->partition();
    if (partition.size())
      partitions.push_back(partition);
  }
  if (!partitions.size()) {
    throw std::logic_error("no valid dataset partition implemented");
  }
  // find common denominator partition
  auto partition = dataset::partition::align(partitions);
  // truncate entries to row limit
  partition = dataset::partition::truncate(partition, nrows);
  // merge partition to concurrency limit
  partition = dataset::partition::merge(partition, nslots);
  // match processor & partition parallelism
  this->downsize(partition.size());

  // 3. run event loop
  this->run(
      [&sources,
       scale](dataset::player *plyr, unsigned int slot,
              std::pair<unsigned long long, unsigned long long> part) {
        plyr->play(sources, scale, slot, part.first, part.second);
      },
      m_player_ptrs, m_range_slots, partition);

  // 4. exit event loop
  for (auto const &ds : sources) {
    ds->finalize();
  }
}

inline std::vector<queryosity::dataset::player *> const &
queryosity::dataset::processor::get_slots() const {
  return m_player_ptrs;
}