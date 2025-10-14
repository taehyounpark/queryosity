#pragma once

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

namespace queryosity {

namespace ROOT {

template <unsigned int Dim, typename Prec>
std::shared_ptr<TH1Bootstrap>
make_hist_with_toys(std::vector<Prec> const &xbins = {0.0, 1.0},
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
// columns. e.g. make_hist_with_toys<1,float> -> TH1FBootstrap
template <unsigned int Dim, typename Prec>
std::shared_ptr<TH1Bootstrap>
make_hist_with_toys(size_t nxbins, double xmin, double xmax, size_t nybins = 1,
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

std::shared_ptr<TH1Bootstrap>
clone_hist_with_toys(std::shared_ptr<TH1Bootstrap> hist) {
  auto cloned =
      std::shared_ptr<TH1Bootstrap>(static_cast<TH1Bootstrap *>(hist->Clone()));
  cloned->SetDirectory(nullptr);
  return cloned;
}

class toy_generator {

public:
  // HACK: cannot have thread-local variables (yet?)
  // static thread_local std::unique_ptr<BootstrapGenerator> generator; //!
  // static thread_local unsigned int n_toys; //!
  // static thread_local unsigned int run_number; //!
  // static thread_local unsigned int event_number; //!
  // static thread_local unsigned int channel_number; //!
  static std::unique_ptr<BootstrapGenerator> generator; //!
  static unsigned int n_toys; //!
  static unsigned int run_number; //!
  static unsigned int event_number; //!
  static unsigned int channel_number; //!

public:
  toy_generator(unsigned int n_toys = 0);
  virtual ~toy_generator() = default;

  void generate(unsigned int run_number, unsigned int event_number,
                unsigned int channel_number);
};

template <int Dim, typename Prec> class hist_with_toys;

template <typename Prec>
class hist_with_toys<1, Prec>
    : public qty::query::definition<std::shared_ptr<TH1Bootstrap>(
          Prec, unsigned int, unsigned int, unsigned int)>,
      public toy_generator {

public:
  hist_with_toys(const std::string &hname, unsigned int nx, Prec xmin,
                 Prec xmax, unsigned int n_toys);
  hist_with_toys(const std::string &hname, const std::vector<Prec> &,
                 unsigned int n_toys);
  virtual ~hist_with_toys() = default;

  virtual void fill(qty::column::observable<Prec>,
                    qty::column::observable<unsigned int>,
                    qty::column::observable<unsigned int>,
                    qty::column::observable<unsigned int>,
                    double) final override;
  virtual std::shared_ptr<TH1Bootstrap> result() const final override;
  virtual std::shared_ptr<TH1Bootstrap>
  merge(std::vector<std::shared_ptr<TH1Bootstrap>> const &results)
      const final override;

protected:
  // histogram
  std::shared_ptr<TH1Bootstrap> m_hist; //!
};

} // namespace ROOT

} // namespace queryosity

// HACK: cannot have thread-local variables (yet?)
// inline thread_local std::unique_ptr<BootstrapGenerator>
//     queryosity::ROOT::toy_generator::generator =
//         std::make_unique<BootstrapGenerator>("bg", "bg", 0); 
// inline thread_local unsigned int queryosity::ROOT::toy_generator::n_toys = 0; 
// inline thread_local unsigned int queryosity::ROOT::toy_generator::run_number = 0; 
// inline thread_local unsigned int queryosity::ROOT::toy_generator::event_number = 0; 
// inline thread_local unsigned int queryosity::ROOT::toy_generator::channel_number = 0; 
inline std::unique_ptr<BootstrapGenerator>
    queryosity::ROOT::toy_generator::generator =
        std::make_unique<BootstrapGenerator>("bg", "bg", 0); 
inline unsigned int queryosity::ROOT::toy_generator::n_toys = 0; 
inline unsigned int queryosity::ROOT::toy_generator::run_number = 0; 
inline unsigned int queryosity::ROOT::toy_generator::event_number = 0; 
inline unsigned int queryosity::ROOT::toy_generator::channel_number = 0; 

queryosity::ROOT::toy_generator::toy_generator(unsigned int n_toys) {
  if (this->n_toys <= n_toys) {
    this->generator->Set(n_toys);
    this->n_toys = n_toys;
  }
}

void queryosity::ROOT::toy_generator::generate(unsigned int run_number,
                                               unsigned int event_number,
                                               unsigned int channel_number) {
  if (this->run_number != run_number || this->event_number != event_number ||
      this->event_number != channel_number) {
    this->generator->Generate(run_number, event_number, channel_number);
    this->run_number = run_number;
    this->event_number = event_number;
    this->channel_number = channel_number;
  }
}

template <typename Prec>
queryosity::ROOT::hist_with_toys<1, Prec>::hist_with_toys(
    const std::string &hname, unsigned int nx, Prec xmin, Prec xmax,
    unsigned int n_toys)
    : toy_generator(n_toys),
      m_hist(make_hist_with_toys<1, Prec>(nx, xmin, xmax)) {
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
  // m_hist->SetGenerator(this->generator.get());
}

template <typename Prec>
queryosity::ROOT::hist_with_toys<1, Prec>::hist_with_toys(
    const std::string &hname, const std::vector<Prec> &xbins,
    unsigned int n_toys)
    : toy_generator(n_toys), m_hist(make_hist_with_toys<1, Prec>(xbins)) {
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
  // m_hist->SetGenerator(this->generator.get());
}

template <typename Prec>
void queryosity::ROOT::hist_with_toys<1, Prec>::fill(
    qty::column::observable<Prec> x,
    qty::column::observable<unsigned int> runNumber,
    qty::column::observable<unsigned int> eventNumber,
    qty::column::observable<unsigned int> channelNumber, double w) {
  // HACK: cannot generate toys thread-locally once per-event (yet?)
  // this->generate(runNumber.value(), eventNumber.value(),
  // channelNumber.value()); m_hist->Fill(x.value(), w);
  // generate (same) toys per-event, per-histogram
  m_hist->Fill(x.value(), w, runNumber.value(), eventNumber.value(),
               channelNumber.value());
}

template <typename Prec>
std::shared_ptr<TH1Bootstrap>
queryosity::ROOT::hist_with_toys<1, Prec>::result() const {
  return m_hist;
}

template <typename Prec>
std::shared_ptr<TH1Bootstrap> queryosity::ROOT::hist_with_toys<1, Prec>::merge(
    std::vector<std::shared_ptr<TH1Bootstrap>> const &results) const {
  auto merged_result = clone_hist_with_toys(results[0]);
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  merged_result->SetDirectory(0);
  return merged_result;
}