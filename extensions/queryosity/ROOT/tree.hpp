#pragma once

#include <memory>
#include <string>
#include <vector>

#include <ROOT/RVec.hxx>

#include "TChain.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TROOT.h"
#include "TTree.h"
#include "TTreeReader.h"
#include "TTreeReaderArray.h"
#include "TTreeReaderValue.h"

#include <queryosity.hpp>

namespace queryosity {
  
namespace ROOT {

class tree : public queryosity::dataset::reader<tree> {

public:
  class Reader;

  template <typename T> class branch;

  template <typename... Ts> class snapshot;

public:
  tree(const std::vector<std::string> &file_paths, const std::string &tree_name);
  tree(std::initializer_list<std::string> file_paths,
       const std::string &tree_name);
  virtual ~tree() = default;

  virtual void parallelize(unsigned int nslots) final override;

  virtual std::vector<std::pair<unsigned long long, unsigned long long>>
  partition() final override;

  template <typename U>
  std::unique_ptr<branch<U>> read(unsigned int slot,
                                  const std::string &branchName);

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) final override;
  virtual void execute(unsigned int slot,
                       unsigned long long entry) final override;
  virtual void finalize(unsigned int slot) final override;
  using queryosity::dataset::source::finalize;
  using queryosity::dataset::source::initialize;

protected:
  std::vector<std::string> m_file_paths;
  std::string m_tree_name;

  std::vector<std::unique_ptr<TTree>> m_trees;              //!
  std::vector<std::unique_ptr<TTreeReader>> m_tree_readers;  //!
};

template <typename T>
class tree::branch : public queryosity::column::reader<T> {

public:
  branch(const std::string &branchName, TTreeReader &tree_reader)
      : m_branchName(branchName) {
    m_tree_reader_value = std::make_unique<TTreeReaderValue<T>>(
        tree_reader, this->m_branchName.c_str());
  }
  ~branch() = default;

  virtual T const &read(unsigned int, unsigned long long) const final override {
    return **m_tree_reader_value;
  }

protected:
  std::string m_branchName;
  std::unique_ptr<TTreeReaderValue<T>> m_tree_reader_value; //!
};

template <typename T>
class tree::branch<::ROOT::RVec<T>>
    : public queryosity::column::reader<::ROOT::RVec<T>> {

public:
  branch(const std::string &branchName, TTreeReader &tree_reader)
      : m_branchName(branchName) {
    m_tree_reader_array = std::make_unique<TTreeReaderArray<T>>(
        tree_reader, this->m_branchName.c_str());
  }
  ~branch() = default;

  virtual void initialize(unsigned int, unsigned long long,
                          unsigned long long) final override {}

  virtual ::ROOT::RVec<T> const &read(unsigned int,
                                    unsigned long long) const final override {
    if (auto arraySize = m_tree_reader_array->GetSize()) {
      ::ROOT::RVec<T> readArray(&m_tree_reader_array->At(0), arraySize);
      std::swap(m_readArray, readArray);
    } else {
      ::ROOT::RVec<T> emptyVector{};
      std::swap(m_readArray, emptyVector);
    }
    return m_readArray;
  }

protected:
  std::string m_branchName;
  std::unique_ptr<TTreeReaderArray<T>> m_tree_reader_array;  //!
  mutable ::ROOT::RVec<T> m_readArray;  //!
};

template <>
class tree::branch<::ROOT::RVec<bool>>
    : public queryosity::column::reader<::ROOT::RVec<bool>> {

public:
  branch(const std::string &branchName, TTreeReader &tree_reader)
      : m_branchName(branchName) {
    m_tree_reader_array = std::make_unique<TTreeReaderArray<bool>>(
        tree_reader, this->m_branchName.c_str());
  }
  ~branch() = default;

  virtual ::ROOT::RVec<bool> const &
  read(unsigned int, unsigned long long) const final override {
    if (m_tree_reader_array->GetSize()) {
      ::ROOT::RVec<bool> readArray(m_tree_reader_array->begin(),
                                 m_tree_reader_array->end());
      std::swap(m_readArray, readArray);
    } else {
      ::ROOT::RVec<bool> emptyVector{};
      std::swap(m_readArray, emptyVector);
    }
    return m_readArray;
  }

protected:
  std::string m_branchName;
  std::unique_ptr<TTreeReaderArray<bool>> m_tree_reader_array; //!
  mutable ::ROOT::RVec<bool> m_readArray;  //!
};

template <typename... ColumnTypes>
class tree::snapshot
    : public qty::query::definition<std::shared_ptr<TTree>(ColumnTypes...)> {

public:
  static constexpr size_t N = sizeof...(ColumnTypes);
  using branch_array_t = std::array<TBranch *, N>;
  using column_tuple_t = std::tuple<ColumnTypes...>;

public:
  template <typename... Names>
  snapshot(const std::string &tree_name, Names const &...branchNames);
  snapshot(const std::string &tree_name,
           std::vector<std::string> const &branchNames);
  virtual ~snapshot() = default;

  virtual void fill(qty::column::observable<ColumnTypes>...,
                    double) final override;
  virtual std::shared_ptr<TTree> result() const final override;
  virtual std::shared_ptr<TTree> merge(
      std::vector<std::shared_ptr<TTree>> const &results) const final override;

private:
  // Helper function to convert a vector to a tuple (for known size at runtime)
  template <std::size_t... I>
  auto make_branchNamesTupleImpl(const std::vector<std::string> &v,
                                std::index_sequence<I...>) {
    return std::make_tuple(v[I]...);
  }

  auto make_branchNamesTuple(const std::vector<std::string> &v) {
    assert(v.size() == N &&
           "number of branch names must exactly match that of output columns");
    return make_branchNamesTupleImpl(v, std::make_index_sequence<N>{});
  }

  // helper function to make branch of i-th data type with i-th column name
  template <typename T, std::size_t I>
  TBranch *make_branch(const std::string &name) {
    return m_snapshot->Branch<T>(name.c_str(), &std::get<I>(m_columns));
  }

  template <std::size_t I, typename T>
  void fill_branch(const qty::column::observable<T> &column) {
    std::get<I>(m_columns) = column.value();
  }

  // expand the packs ColumnTypes, Indices, and branchNames simultaneously
  template <std::size_t... Is, typename... Names>
  branch_array_t make_branches(std::index_sequence<Is...>,
                             Names const &...branchNames) {
    return {{make_branch<ColumnTypes, Is>(branchNames)...}};
  }

  template <std::size_t... Is, typename... Observables>
  void fill_branches(std::index_sequence<Is...>, Observables... columns) {
    (fill_branch<Is>(columns), ...);
  }

protected:
  std::shared_ptr<TTree> m_snapshot; //!
  branch_array_t m_branches;
  column_tuple_t m_columns;
};

}

}

inline queryosity::ROOT::tree::tree(const std::vector<std::string> &file_paths,
                  const std::string &tree_name)
    : m_file_paths(file_paths), m_tree_name(tree_name) {}

inline queryosity::ROOT::tree::tree(std::initializer_list<std::string> file_paths,
                  const std::string &tree_name)
    : m_file_paths(file_paths), m_tree_name(tree_name) {}

inline void queryosity::ROOT::tree::parallelize(unsigned int nslots) {
  m_trees.clear(); m_trees.resize(nslots);
  m_tree_readers.clear(); m_tree_readers.resize(nslots);
  for (unsigned int islot = 0; islot < nslots; ++islot) {
    auto tree =
        std::make_unique<TChain>(m_tree_name.c_str(), m_tree_name.c_str());
    tree->ResetBit(kMustCleanup);
    for (auto const &file_path : m_file_paths) {
      tree->Add(file_path.c_str());
    }
    auto tree_reader = std::make_unique<TTreeReader>(tree.release());
    m_trees[islot] = std::move(tree);
    m_tree_readers[islot] = std::move(tree_reader);
  }
}

inline std::vector<std::pair<unsigned long long, unsigned long long>>
queryosity::ROOT::tree::partition() {
  ::ROOT::EnableThreadSafety();
  // ::ROOT::EnableImplicitMT(m_nslots);

  TDirectory::TContext c;
  std::vector<std::pair<unsigned long long, unsigned long long>> parts;

  // offset to account for global entry position
  long long offset = 0ll;
  for (auto const &file_path : m_file_paths) {
    // check file
    std::unique_ptr<TFile> file(TFile::Open(file_path.c_str()));
    if (!file) {
      continue;
    } else if (file->IsZombie()) {
      continue;
    }

    // check tree
    auto tree = file->Get<TTree>(m_tree_name.c_str());
    if (!tree) {
      continue;
    }

    // add tree clusters
    auto fileEntries = tree->GetEntries();
    auto clusterIterator = tree->GetClusterIterator(0);
    long long start = 0ll, end = 0ll;
    while ((start = clusterIterator.Next()) < fileEntries) {
      end = clusterIterator.GetNextEntry();
      parts.emplace_back(offset + start, offset + end);
    }
    // remember offset for next file
    offset += fileEntries;
  }

  return parts;
}

inline void queryosity::ROOT::tree::initialize(unsigned int slot, unsigned long long begin,
                             unsigned long long end) {
  m_tree_readers[slot]->SetEntriesRange(begin, end);
}

inline void queryosity::ROOT::tree::execute(unsigned int slot, unsigned long long entry) {
  m_tree_readers[slot]->SetEntry(entry);
}

inline void queryosity::ROOT::tree::finalize(unsigned int) {
  // m_tree_readers[slot].reset(nullptr);
}

template <typename U>
std::unique_ptr<queryosity::ROOT::tree::branch<U>> queryosity::ROOT::tree::read(unsigned int slot,
                                            const std::string &branchName) {
  return std::make_unique<branch<U>>(branchName, *m_tree_readers[slot]);
}

template <typename... ColumnTypes>
template <typename... Names>
queryosity::ROOT::tree::snapshot<ColumnTypes...>::snapshot(const std::string &tree_name,
                                         Names const &...branchNames)
    : m_snapshot(std::make_shared<TTree>(tree_name.c_str(), tree_name.c_str())),
      m_branches(make_branches(std::index_sequence_for<ColumnTypes...>(),
                              branchNames...)) {
  m_snapshot->SetDirectory(0);
}

template <typename... ColumnTypes>
queryosity::ROOT::tree::snapshot<ColumnTypes...>::snapshot(
    const std::string &tree_name, std::vector<std::string> const &branchNames)
    : m_snapshot(std::make_shared<TTree>(tree_name.c_str(), tree_name.c_str())),
      m_branches(std::apply(
          [this](auto &&...args) {
            return make_branches(std::index_sequence_for<ColumnTypes...>(),
                                args...);
          },
          make_branchNamesTuple(branchNames))) {
  m_snapshot->SetDirectory(0);
}

template <typename... ColumnTypes>
void queryosity::ROOT::tree::snapshot<ColumnTypes...>::fill(
    qty::column::observable<ColumnTypes>... columns, double) {
  this->fill_branches(std::index_sequence_for<ColumnTypes...>(), columns...);
  m_snapshot->Fill();
}

template <typename... ColumnTypes>
std::shared_ptr<TTree> queryosity::ROOT::tree::snapshot<ColumnTypes...>::result() const {
  return m_snapshot;
}

template <typename... ColumnTypes>
std::shared_ptr<TTree> queryosity::ROOT::tree::snapshot<ColumnTypes...>::merge(
    std::vector<std::shared_ptr<TTree>> const &results) const {
  TList list;
  for (auto const &result : results) {
    list.Add(result.get());
  }
  auto merged = std::shared_ptr<TTree>(TTree::MergeTrees(&list));
  merged->SetDirectory(0);
  return merged;
}