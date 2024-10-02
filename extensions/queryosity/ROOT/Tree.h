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

class Tree : public queryosity::dataset::reader<Tree> {

public:
  class Reader;

  template <typename T> class Branch;

  template <typename... Ts> class Snapshot;

public:
  Tree(const std::vector<std::string> &filePaths, const std::string &treeName);
  Tree(std::initializer_list<std::string> filePaths,
       const std::string &treeName);
  virtual ~Tree() = default;

  virtual void parallelize(unsigned int nslots) final override;

  virtual std::vector<std::pair<unsigned long long, unsigned long long>>
  partition() final override;

  template <typename U>
  std::unique_ptr<Branch<U>> read(unsigned int slot,
                                  const std::string &branchName);

  virtual void initialize(unsigned int slot, unsigned long long begin,
                          unsigned long long end) final override;
  virtual void execute(unsigned int slot,
                       unsigned long long entry) final override;
  virtual void finalize(unsigned int slot) final override;
  using queryosity::dataset::source::finalize;
  using queryosity::dataset::source::initialize;

protected:
  std::vector<std::string> m_inputFiles;
  std::string m_treeName;

  std::vector<std::unique_ptr<TTree>> m_trees;             //!
  std::vector<std::unique_ptr<TTreeReader>> m_treeReaders; //!
};

template <typename T>
class Tree::Branch : public queryosity::column::reader<T> {

public:
  Branch(const std::string &branchName, TTreeReader &treeReader)
      : m_branchName(branchName) {
    m_treeReaderValue = std::make_unique<TTreeReaderValue<T>>(
        treeReader, this->m_branchName.c_str());
  }
  ~Branch() = default;

  virtual T const &read(unsigned int, unsigned long long) const final override {
    return **m_treeReaderValue;
  }

protected:
  std::string m_branchName;
  std::unique_ptr<TTreeReaderValue<T>> m_treeReaderValue;
};

template <typename T>
class Tree::Branch<ROOT::RVec<T>>
    : public queryosity::column::reader<ROOT::RVec<T>> {

public:
  Branch(const std::string &branchName, TTreeReader &treeReader)
      : m_branchName(branchName) {
    m_treeReaderArray = std::make_unique<TTreeReaderArray<T>>(
        treeReader, this->m_branchName.c_str());
  }
  ~Branch() = default;

  virtual void initialize(unsigned int, unsigned long long,
                          unsigned long long) final override {}

  virtual ROOT::RVec<T> const &read(unsigned int,
                                    unsigned long long) const final override {
    if (auto arraySize = m_treeReaderArray->GetSize()) {
      ROOT::RVec<T> readArray(&m_treeReaderArray->At(0), arraySize);
      std::swap(m_readArray, readArray);
    } else {
      ROOT::RVec<T> emptyVector{};
      std::swap(m_readArray, emptyVector);
    }
    return m_readArray;
  }

protected:
  std::string m_branchName;
  std::unique_ptr<TTreeReaderArray<T>> m_treeReaderArray;
  mutable ROOT::RVec<T> m_readArray;
};

template <>
class Tree::Branch<ROOT::RVec<bool>>
    : public queryosity::column::reader<ROOT::RVec<bool>> {

public:
  Branch(const std::string &branchName, TTreeReader &treeReader)
      : m_branchName(branchName) {
    m_treeReaderArray = std::make_unique<TTreeReaderArray<bool>>(
        treeReader, this->m_branchName.c_str());
  }
  ~Branch() = default;

  virtual ROOT::RVec<bool> const &
  read(unsigned int, unsigned long long) const final override {
    if (m_treeReaderArray->GetSize()) {
      ROOT::RVec<bool> readArray(m_treeReaderArray->begin(),
                                 m_treeReaderArray->end());
      std::swap(m_readArray, readArray);
    } else {
      ROOT::RVec<bool> emptyVector{};
      std::swap(m_readArray, emptyVector);
    }
    return m_readArray;
  }

protected:
  std::string m_branchName;
  std::unique_ptr<TTreeReaderArray<bool>> m_treeReaderArray;
  mutable ROOT::RVec<bool> m_readArray;
};

template <typename... ColumnTypes>
class Tree::Snapshot
    : public qty::query::definition<std::shared_ptr<TTree>(ColumnTypes...)> {

public:
  static constexpr size_t N = sizeof...(ColumnTypes);
  using BranchArray_t = std::array<TBranch *, N>;
  using ColumnTuple_t = std::tuple<ColumnTypes...>;

public:
  template <typename... Names>
  Snapshot(const std::string &treeName, Names const &...branchNames);
  Snapshot(const std::string &treeName,
           std::vector<std::string> const &branchNames);
  virtual ~Snapshot() = default;

  virtual void fill(qty::column::observable<ColumnTypes>...,
                    double) final override;
  virtual std::shared_ptr<TTree> result() const final override;
  virtual std::shared_ptr<TTree> merge(
      std::vector<std::shared_ptr<TTree>> const &results) const final override;

private:
  // Helper function to convert a vector to a tuple (for known size at runtime)
  template <std::size_t... I>
  auto makeBranchNamesTupleImpl(const std::vector<std::string> &v,
                                std::index_sequence<I...>) {
    return std::make_tuple(v[I]...);
  }

  auto makeBranchNamesTuple(const std::vector<std::string> &v) {
    assert(v.size() == N &&
           "number of branch names must exactly match that of output columns");
    return makeBranchNamesTupleImpl(v, std::make_index_sequence<N>{});
  }

  // helper function to make branch of i-th data type with i-th column name
  template <typename T, std::size_t I>
  TBranch *makeBranch(const std::string &name) {
    return m_snapshot->Branch<T>(name.c_str(), &std::get<I>(m_columns));
  }

  template <std::size_t I, typename T>
  void fillBranch(const qty::column::observable<T> &column) {
    std::get<I>(m_columns) = column.value();
  }

  // expand the packs ColumnTypes, Indices, and branchNames simultaneously
  template <std::size_t... Is, typename... Names>
  BranchArray_t makeBranches(std::index_sequence<Is...>,
                             Names const &...branchNames) {
    return {{makeBranch<ColumnTypes, Is>(branchNames)...}};
  }

  template <std::size_t... Is, typename... Observables>
  void fillBranches(std::index_sequence<Is...>, Observables... columns) {
    (fillBranch<Is>(columns), ...);
  }

protected:
  std::shared_ptr<TTree> m_snapshot; //!
  BranchArray_t m_branches;
  ColumnTuple_t m_columns;
};

inline Tree::Tree(const std::vector<std::string> &inputFiles,
                  const std::string &treeName)
    : m_inputFiles(inputFiles), m_treeName(treeName) {}

inline Tree::Tree(std::initializer_list<std::string> inputFiles,
                  const std::string &treeName)
    : m_inputFiles(inputFiles), m_treeName(treeName) {}

inline void Tree::parallelize(unsigned int nslots) {
  m_trees.resize(nslots);
  m_treeReaders.resize(nslots);
  for (unsigned int islot = 0; islot < nslots; ++islot) {
    auto tree =
        std::make_unique<TChain>(m_treeName.c_str(), m_treeName.c_str());
    tree->ResetBit(kMustCleanup);
    for (auto const &filePath : m_inputFiles) {
      tree->Add(filePath.c_str());
    }
    auto treeReader = std::make_unique<TTreeReader>(tree.get());
    m_trees[islot] = std::move(tree);
    m_treeReaders[islot] = std::move(treeReader);
  }
}

inline std::vector<std::pair<unsigned long long, unsigned long long>>
Tree::partition() {
  ROOT::EnableThreadSafety();
  // ROOT::EnableImplicitMT(m_nslots);

  TDirectory::TContext c;
  std::vector<std::pair<unsigned long long, unsigned long long>> parts;

  // offset to account for global entry position
  long long offset = 0ll;
  for (auto const &filePath : m_inputFiles) {
    // check file
    std::unique_ptr<TFile> file(TFile::Open(filePath.c_str()));
    if (!file) {
      continue;
    } else if (file->IsZombie()) {
      continue;
    }

    // check tree
    auto tree = file->Get<TTree>(m_treeName.c_str());
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

inline void Tree::initialize(unsigned int slot, unsigned long long begin,
                             unsigned long long end) {
  m_treeReaders[slot]->SetEntriesRange(begin, end);
}

inline void Tree::execute(unsigned int slot, unsigned long long entry) {
  m_treeReaders[slot]->SetEntry(entry);
}

inline void Tree::finalize(unsigned int) {
  // m_treeReaders[slot].reset(nullptr);
}

template <typename U>
std::unique_ptr<Tree::Branch<U>> Tree::read(unsigned int slot,
                                            const std::string &branchName) {
  return std::make_unique<Branch<U>>(branchName, *m_treeReaders[slot]);
}

template <typename... ColumnTypes>
template <typename... Names>
Tree::Snapshot<ColumnTypes...>::Snapshot(const std::string &treeName,
                                         Names const &...branchNames)
    : m_snapshot(std::make_shared<TTree>(treeName.c_str(), treeName.c_str())),
      m_branches(makeBranches(std::index_sequence_for<ColumnTypes...>(),
                              branchNames...)) {
  m_snapshot->SetDirectory(0);
}

template <typename... ColumnTypes>
Tree::Snapshot<ColumnTypes...>::Snapshot(
    const std::string &treeName, std::vector<std::string> const &branchNames)
    : m_snapshot(std::make_shared<TTree>(treeName.c_str(), treeName.c_str())),
      m_branches(std::apply(
          [this](auto &&...args) {
            return makeBranches(std::index_sequence_for<ColumnTypes...>(),
                                args...);
          },
          makeBranchNamesTuple(branchNames))) {
  m_snapshot->SetDirectory(0);
}

template <typename... ColumnTypes>
void Tree::Snapshot<ColumnTypes...>::fill(
    qty::column::observable<ColumnTypes>... columns, double) {
  this->fillBranches(std::index_sequence_for<ColumnTypes...>(), columns...);
  m_snapshot->Fill();
}

template <typename... ColumnTypes>
std::shared_ptr<TTree> Tree::Snapshot<ColumnTypes...>::result() const {
  return m_snapshot;
}

template <typename... ColumnTypes>
std::shared_ptr<TTree> Tree::Snapshot<ColumnTypes...>::merge(
    std::vector<std::shared_ptr<TTree>> const &results) const {
  TList list;
  for (auto const &result : results) {
    list.Add(result.get());
  }
  auto merged = std::shared_ptr<TTree>(TTree::MergeTrees(&list));
  merged->SetDirectory(0);
  return merged;
}