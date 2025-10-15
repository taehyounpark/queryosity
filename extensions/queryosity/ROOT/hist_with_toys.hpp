#pragma once

#include "BootstrapGenerator/BootstrapGenerator.h"
#include "BootstrapGenerator/TH1DBootstrap.h"
#include "BootstrapGenerator/TH1FBootstrap.h"
#include "BootstrapGenerator/TH2DBootstrap.h"
#include "BootstrapGenerator/TH2FBootstrap.h"

#include "TROOT.h"
#include <ROOT/RVec.hxx>

#include <queryosity.hpp>

#include <memory>
#include <string>
#include <vector>

namespace queryosity {

namespace ROOT {

class toy_generator {

public:
  // FIXME: thread-local, per-event, histogram-global generator doesn't work
  // inline static thread_local BootstrapGenerator generator{"bg","bg",1};
  // inline static thread_local unsigned int ntoy = 0;       
  // inline static thread_local unsigned int run_number = 0;   
  // inline static thread_local unsigned int event_number = 0;  
  // inline static thread_local unsigned int channel_number = 0;

public:
  toy_generator(unsigned int ntoy = 0);
  virtual ~toy_generator() = default;

  // FIXME: thread-local, per-event, histogram-global generator doesn't work
  // void generate(unsigned int run_number, unsigned int event_number,
  //               unsigned int channel_number);

protected:
  unsigned int m_ntoy;
};

std::shared_ptr<TH1Bootstrap>
clone_hist_with_toys(std::shared_ptr<TH1Bootstrap> hist) {
  auto cloned =
      std::shared_ptr<TH1Bootstrap>(static_cast<TH1Bootstrap *>(hist->Clone()));
  cloned->SetDirectory(nullptr);
  return cloned;
}

template <int Dim, typename Prec> class hist_with_toys;

template <typename Prec>
class hist_with_toys<1, Prec>
    : public qty::query::definition<std::shared_ptr<TH1Bootstrap>(
          Prec, unsigned int, unsigned int, unsigned int)>,
      public toy_generator {

public:
  hist_with_toys(const std::string &hname = "", unsigned int nx = 1,
                 Prec xmin = 0, Prec xmax = 1, unsigned int ntoy = 0);
  hist_with_toys(const std::string &hname, const std::vector<Prec> &,
                 unsigned int ntoy);
  virtual ~hist_with_toys() = default;

  virtual void initialize(unsigned int, unsigned long long,  unsigned long long) override;
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
  std::string m_hname;
  std::vector<Prec> m_xbins;
};

} // namespace ROOT

} // namespace queryosity

queryosity::ROOT::toy_generator::toy_generator(unsigned int ntoy) :
  m_ntoy(ntoy)
{
  // ::ROOT::EnableThreadSafety();
  // FIXME: thread-local, per-event, histogram-global generator doesn't work
  // if (ntoy > this->m_ntoy) {
  //   this->generator.Set(ntoy);
  //   this->m_ntoy = ntoy;
  // }
}

// FIXME: thread-local, per-event, histogram-global generator doesn't work
// void queryosity::ROOT::toy_generator::generate(unsigned int run_number,
//                                                unsigned int event_number,
//                                                unsigned int channel_number) {
//   if (this->run_number != run_number || this->event_number != event_number ||
//       this->event_number != channel_number) {
//     this->generator.Generate(run_number, event_number, channel_number);
//     this->run_number = run_number;
//     this->event_number = event_number;
//     this->channel_number = channel_number;
//   }
// }

template <typename Prec>
queryosity::ROOT::hist_with_toys<1, Prec>::hist_with_toys(
    const std::string &hname, unsigned int nx, Prec xmin, Prec xmax,
    unsigned int ntoy)
    : toy_generator(ntoy),
      m_hist(nullptr),
      m_hname(hname)
       {

  m_xbins.clear(); m_xbins.reserve(nx+1);
  auto dx = (xmax - xmin) / static_cast<Prec>(nx);
  for (unsigned int ix = 0 ; ix<nx+1 ; ++ix) {
    m_xbins.emplace_back(xmin + ix*dx);
  }
   
  if constexpr(std::is_same_v<Prec, float>) {
    using hist_t = TH1FBootstrap;
    m_hist = std::make_shared<hist_t>(
      m_hname.c_str(), m_hname.c_str(),
      m_xbins.size()-1, &m_xbins[0],
      m_ntoy
    );
  } else if constexpr(std::is_same_v<Prec, double>) {
    using hist_t = TH1DBootstrap;
    m_hist = std::make_shared<hist_t>(
      m_hname.c_str(), m_hname.c_str(),
      m_xbins.size()-1, &m_xbins[0],
      m_ntoy
    );
  }
  m_hist->SetDirectory(nullptr);
}

template <typename Prec>
queryosity::ROOT::hist_with_toys<1, Prec>::hist_with_toys(
    const std::string &hname, const std::vector<Prec> &xbins,
    unsigned int ntoy)
    : toy_generator(ntoy), m_hist(nullptr), m_hname(hname), m_xbins(xbins) {
  if constexpr(std::is_same_v<Prec, float>) {
    using hist_t = TH1FBootstrap;
    m_hist = std::make_shared<hist_t>(
      m_hname.c_str(), m_hname.c_str(),
      m_xbins.size()-1, &m_xbins[0],
      m_ntoy
    );
  } else if constexpr(std::is_same_v<Prec, double>) {
    using hist_t = TH1DBootstrap;
    m_hist = std::make_shared<hist_t>(
      m_hname.c_str(), m_hname.c_str(),
      m_xbins.size()-1, &m_xbins[0],
      m_ntoy
    );
  }
  m_hist->SetDirectory(nullptr);
}

template <typename Prec>
void queryosity::ROOT::hist_with_toys<1, Prec>::initialize(
  unsigned int slot,
  unsigned long long,
  unsigned long long) {
  }

template <typename Prec>
void queryosity::ROOT::hist_with_toys<1, Prec>::fill(
    qty::column::observable<Prec> x,
    qty::column::observable<unsigned int> runNumber,
    qty::column::observable<unsigned int> eventNumber,
    qty::column::observable<unsigned int> channelNumber, double w) {

  // FIXME: thread-local, per-event, histogram-global generator doesn't work
  // this->generate(runNumber.value(), eventNumber.value(), channelNumber.value());
  // m_hist->Fill(x.value(), w);

  // generate per-thread, per-event, per-histogram
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
  return merged_result;
}