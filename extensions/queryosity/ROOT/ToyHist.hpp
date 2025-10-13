#pragma once

#include "ToySeed.hpp"

#include "BootstrapGenerator/BootstrapGenerator.h"
#include "BootstrapGenerator/TH1DBootstrap.h"
#include "BootstrapGenerator/TH1FBootstrap.h"
#include "BootstrapGenerator/TH2DBootstrap.h"
#include "BootstrapGenerator/TH2FBootstrap.h"

#include <ROOT/RVec.hxx>

#include <queryosity.hpp>

#include <memory>
#include <string>
#include <vector>

namespace {

template <unsigned int Dim, typename Prec>
std::shared_ptr<TH1Bootstrap>
makeToyHist(std::vector<Prec> const &xbins = {0.0, 1.0},
            std::vector<Prec> const &ybins = {0.0, 1.0},
            unsigned int nrep = 100) {
  std::shared_ptr<TH1Bootstrap> hist;
  if constexpr (Dim == 1) {
    (void)ybins;
    if constexpr (std::is_same_v<Prec, float>) {
      hist = std::shared_ptr<TH1FBootstrap>(
          new TH1FBootstrap("", "", xbins.size() - 1, &xbins[0], nrep));
    } else if constexpr (std::is_same_v<Prec, double>) {
      hist = std::shared_ptr<TH1DBootstrap>(
          new TH1DBootstrap("", "", xbins.size() - 1, &xbins[0], nrep));
    }
  } else if constexpr (Dim == 2) {
    if constexpr (std::is_same_v<Prec, float>) {
      hist = std::shared_ptr<TH2FBootstrap>(
          new TH2FBootstrap("", "", xbins.size() - 1, &xbins[0],
                            ybins.size() - 1, &ybins[0], nrep));
    } else if constexpr (std::is_same_v<Prec, double>) {
      hist = std::shared_ptr<TH2DBootstrap>(
          new TH2DBootstrap("", "", xbins.size() - 1, &xbins[0],
                            ybins.size() - 1, &ybins[0], nrep));
    }
  }
  hist->SetDirectory(nullptr);
  return hist;
}

// create the appropriate histogram given the dimensionality and precision of
// columns. e.g. makeToyHist<1,float> -> TH1FBootstrap
template <unsigned int Dim, typename Prec>
std::shared_ptr<TH1Bootstrap>
makeToyHist(size_t nxbins, double xmin, double xmax, size_t nybins = 1,
            double ymin = 0, double ymax = 1, unsigned int nrep = 100) {
  std::shared_ptr<TH1Bootstrap> hist = nullptr;
  if constexpr (Dim == 1) {
    (void)nybins;
    (void)ymin;
    (void)ymax;
    if constexpr (std::is_same_v<Prec, float>) {
      hist = std::shared_ptr<TH1FBootstrap>(
          new TH1FBootstrap("", "", nxbins, xmin, xmax, nrep));
    } else if constexpr (std::is_same_v<Prec, double>) {
      hist = std::shared_ptr<TH1DBootstrap>(
          new TH1DBootstrap("", "", nxbins, xmin, xmax, nrep));
    }
  } else if constexpr (Dim == 2) {
    if constexpr (std::is_same_v<Prec, float>) {
      hist = std::shared_ptr<TH2FBootstrap>(new TH2FBootstrap(
          "", "", nxbins, xmin, xmax, nybins, ymin, ymax, nrep));
    } else if constexpr (std::is_same_v<Prec, double>) {
      hist = std::shared_ptr<TH2DBootstrap>(new TH2DBootstrap(
          "", "", nxbins, xmin, xmax, nybins, ymin, ymax, nrep));
    }
  }
  hist->SetDirectory(nullptr);
  return hist;
}

std::shared_ptr<TH1Bootstrap> cloneToyHist(std::shared_ptr<TH1Bootstrap> hist) {
  auto cloned =
      std::shared_ptr<TH1Bootstrap>(static_cast<TH1Bootstrap *>(hist->Clone()));
  cloned->SetDirectory(nullptr);
  return cloned;
}

} // namespace

namespace queryosity {

namespace ROOT {

template <int Dim, typename Prec> class ToyHist;

template <typename Prec>
class ToyHist<1, Prec>
    : public qty::query::definition<std::shared_ptr<TH1Bootstrap>(Prec)> {

public:
  static thread_local std::unique_ptr<BootstrapGenerator> m_bootgen;

public:
  ToyHist(const std::string &hname, const std::vector<Prec> &);
  virtual ~ToyHist() = default;

  virtual void initialize(unsigned int, unsigned long long,
                          unsigned long long) override;

  virtual void fill(qty::column::observable<Prec>, double) final override;
  virtual std::shared_ptr<TH1Bootstrap> result() const final override;
  virtual std::shared_ptr<TH1Bootstrap>
  merge(std::vector<std::shared_ptr<TH1Bootstrap>> const &results)
      const final override;

protected:
  // seed
  lazy<ToySeed> m_seed;

  // histogram
  std::shared_ptr<TH1Bootstrap> m_hist; //!
  std::vector<Prec> m_xbins;
};

} // namespace ROOT

} // namespace queryosity

template <typename Prec>
queryosity::ROOT::ToyHist<1, Prec>::ToyHist(const std::string &hname,
                                            const std::vector<Prec> &xbins)
    : m_xbins(xbins) {
  m_hist =
      makeToyHist<1, Prec>(m_xbins);
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Prec>
void queryosity::ROOT::ToyHist<1, Prec>::initialize(unsigned int slot,
                                                    unsigned long long,
                                                    unsigned long long) {
  m_hist->SetGenerator(m_seed.get_slot(0)->get_generator());
}

template <typename Prec>
void queryosity::ROOT::ToyHist<1, Prec>::fill(qty::column::observable<Prec> x,
                                              double w) {
  m_hist->Fill(x.value(), w);
}

template <typename Prec>
std::shared_ptr<TH1Bootstrap>
queryosity::ROOT::ToyHist<1, Prec>::result() const {
  return m_hist;
}

template <typename Prec>
std::shared_ptr<TH1Bootstrap> queryosity::ROOT::ToyHist<1, Prec>::merge(
    std::vector<std::shared_ptr<TH1Bootstrap>> const &results) const {
  auto merged_result = cloneToyHist(results[0]);
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  merged_result->SetDirectory(0);
  return merged_result;
}