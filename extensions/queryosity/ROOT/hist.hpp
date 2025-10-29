#pragma once

#include "TH1C.h"
#include "TH1D.h"
#include "TH1F.h"
#include "TH1I.h"
#include "TH2C.h"
#include "TH2D.h"
#include "TH2F.h"
#include "TH2I.h"
#include "TH3C.h"
#include "TH3D.h"
#include "TH3F.h"
#include "TH3I.h"

#include <ROOT/RVec.hxx>

#include <queryosity.hpp>

#include <memory>
#include <string>
#include <vector>

namespace queryosity {

namespace ROOT {

template <unsigned int Dim, typename Val>
std::shared_ptr<TH1> make_hist(std::vector<double> const &xbins = {0.0, 1.0},
                               std::vector<double> const &ybins = {0.0, 1.0},
                               std::vector<double> const &zbins = {0.0, 1.0}) {
  std::shared_ptr<TH1> hist;
  if constexpr (Dim == 1) {
    (void)ybins;
    (void)zbins;
    if constexpr (std::is_same_v<Val, char> || std::is_same_v<Val, bool>) {
      hist =
          std::shared_ptr<TH1C>(new TH1C("", "", xbins.size() - 1, &xbins[0]));
    } else if constexpr (std::is_same_v<Val, int>) {
      hist =
          std::shared_ptr<TH1I>(new TH1I("", "", xbins.size() - 1, &xbins[0]));
    } else if constexpr (std::is_same_v<Val, float>) {
      hist =
          std::shared_ptr<TH1F>(new TH1F("", "", xbins.size() - 1, &xbins[0]));
    } else if constexpr (std::is_same_v<Val, double>) {
      hist =
          std::shared_ptr<TH1D>(new TH1D("", "", xbins.size() - 1, &xbins[0]));
    }
  } else if constexpr (Dim == 2) {
    (void)zbins;
    if constexpr (std::is_same_v<Val, char> || std::is_same_v<Val, bool>) {
      hist = std::shared_ptr<TH2C>(new TH2C("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0]));
    } else if constexpr (std::is_same_v<Val, int>) {
      hist = std::shared_ptr<TH2I>(new TH2I("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0]));
    } else if constexpr (std::is_same_v<Val, float>) {
      hist = std::shared_ptr<TH2F>(new TH2F("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0]));
    } else if constexpr (std::is_same_v<Val, double>) {
      hist = std::shared_ptr<TH2D>(new TH2D("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0]));
    }
  } else if constexpr (Dim == 3) {
    if constexpr (std::is_same_v<Val, char> || std::is_same_v<Val, bool>) {
      hist = std::shared_ptr<TH3C>(new TH3C("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0],
                                            zbins.size() - 1, &zbins[0]));
    } else if constexpr (std::is_same_v<Val, int>) {
      hist = std::shared_ptr<TH3I>(new TH3I("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0],
                                            zbins.size() - 1, &zbins[0]));
    } else if constexpr (std::is_same_v<Val, float>) {
      hist = std::shared_ptr<TH3F>(new TH3F("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0],
                                            zbins.size() - 1, &zbins[0]));
    } else if constexpr (std::is_same_v<Val, double>) {
      hist = std::shared_ptr<TH3D>(new TH3D("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0],
                                            zbins.size() - 1, &zbins[0]));
    }
  }
  hist->SetDirectory(nullptr);
  return hist;
}

// create the appropriate histogram given the dimensionality and Valision of
// columns. e.g. make_hist<1,float> -> TH1F
template <unsigned int Dim, typename Val>
std::shared_ptr<TH1> make_hist(size_t nxbins, double xmin, double xmax,
                               size_t nybins = 1, double ymin = 0,
                               double ymax = 1, size_t nzbins = 1,
                               double zmin = 0, double zmax = 1) {
  std::shared_ptr<TH1> hist = nullptr;
  if constexpr (Dim == 1) {
    (void)nybins;
    (void)ymin;
    (void)ymax;
    (void)nzbins;
    (void)zmin;
    (void)zmax;
    if constexpr (std::is_same_v<Val, char> || std::is_same_v<Val, bool>) {
      hist = std::shared_ptr<TH1C>(new TH1C("", "", nxbins, xmin, xmax));
    } else if constexpr (std::is_same_v<Val, int>) {
      hist = std::shared_ptr<TH1I>(new TH1I("", "", nxbins, xmin, xmax));
    } else if constexpr (std::is_same_v<Val, float>) {
      hist = std::shared_ptr<TH1F>(new TH1F("", "", nxbins, xmin, xmax));
    } else if constexpr (std::is_same_v<Val, double>) {
      hist = std::shared_ptr<TH1D>(new TH1D("", "", nxbins, xmin, xmax));
    }
  } else if constexpr (Dim == 2) {
    (void)nzbins;
    (void)zmin;
    (void)zmax;
    if constexpr (std::is_same_v<Val, char> || std::is_same_v<Val, bool>) {
      hist = std::shared_ptr<TH2C>(
          new TH2C("", "", nxbins, xmin, xmax, nybins, ymin, ymax));
    } else if constexpr (std::is_same_v<Val, int>) {
      hist = std::shared_ptr<TH2I>(
          new TH2I("", "", nxbins, xmin, xmax, nybins, ymin, ymax));
    } else if constexpr (std::is_same_v<Val, float>) {
      hist = std::shared_ptr<TH2F>(
          new TH2F("", "", nxbins, xmin, xmax, nybins, ymin, ymax));
    } else if constexpr (std::is_same_v<Val, double>) {
      hist = std::shared_ptr<TH2D>(
          new TH2D("", "", nxbins, xmin, xmax, nybins, ymin, ymax));
    }
  } else if constexpr (Dim == 3) {
    if constexpr (std::is_same_v<Val, char> || std::is_same_v<Val, bool>) {
      hist = std::shared_ptr<TH3C>(new TH3C("", "", nxbins, xmin, xmax, nybins,
                                            ymin, ymax, nzbins, zmin, zmax));
    } else if constexpr (std::is_same_v<Val, int>) {
      hist = std::shared_ptr<TH3I>(new TH3I("", "", nxbins, xmin, xmax, nybins,
                                            ymin, ymax, nzbins, zmin, zmax));
    } else if constexpr (std::is_same_v<Val, float>) {
      hist = std::shared_ptr<TH3F>(new TH3F("", "", nxbins, xmin, xmax, nybins,
                                            ymin, ymax, nzbins, zmin, zmax));
    } else if constexpr (std::is_same_v<Val, double>) {
      hist = std::shared_ptr<TH3D>(new TH3D("", "", nxbins, xmin, xmax, nybins,
                                            ymin, ymax, nzbins, zmin, zmax));
    }
  }
  hist->SetDirectory(nullptr);
  return hist;
}

template <int Dim, typename Val> class hist;

template <typename Val>
class hist<1, Val> : public qty::query::definition<std::shared_ptr<TH1>(Val)> {

public:
  hist(const std::string &hname = "", unsigned int = 1, double = 0.0,
       double = 1.0);
  hist(const std::string &hname, const std::vector<Val> &);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<Val>, double) final override;
  virtual std::shared_ptr<TH1> result() const final override;
  virtual std::shared_ptr<TH1>
  merge(std::vector<std::shared_ptr<TH1>> const &results) const final override;

protected:
  // histogram
  std::shared_ptr<TH1> m_hist; //!
};

template <typename Val>
class hist<2, Val>
    : public qty::query::definition<std::shared_ptr<TH2>(Val, Val)> {

public:
  hist(const std::string &hname, std::vector<Val> const &,
       std::vector<Val> const &);
  hist(const std::string &hname = "", unsigned int nx = 1, double xmin = 0.0,
       double xmax = 1.0, unsigned int ny = 1, double ymin = 0,
       double ymax = 1.0);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<Val>, qty::column::observable<Val>,
                    double) final override;
  virtual std::shared_ptr<TH2> result() const final override;
  virtual std::shared_ptr<TH2>
  merge(std::vector<std::shared_ptr<TH2>> const &results) const final override;

protected:
  std::shared_ptr<TH2> m_hist; //!
};

template <typename Val>
class hist<3, Val>
    : public qty::query::definition<std::shared_ptr<TH3>(Val, Val, Val)> {

public:
  hist(const std::string &hname, std::vector<Val> const &,
       std::vector<Val> const &, std::vector<Val> const &);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<Val>, qty::column::observable<Val>,
                    qty::column::observable<Val>, double) final override;
  virtual std::shared_ptr<TH3> result() const final override;
  virtual std::shared_ptr<TH3>
  merge(std::vector<std::shared_ptr<TH3>> const &results) const final override;

protected:
  std::shared_ptr<TH3> m_hist; //!
};

template <typename Val>
class hist<1, ::ROOT::RVec<Val>>
    : public qty::query::definition<std::shared_ptr<TH1>(::ROOT::RVec<Val>)> {

public:
  hist(const std::string &hname = "", unsigned int nbins = 1, double min = 0.0,
       double xmax = 1.0);
  hist(const std::string &hname, std::vector<Val> const &xbins);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<::ROOT::RVec<Val>>,
                    double) final override;
  virtual std::shared_ptr<TH1> result() const final override;
  virtual std::shared_ptr<TH1>
  merge(std::vector<std::shared_ptr<TH1>> const &results) const final override;

protected:
  // histogram
  std::shared_ptr<TH1> m_hist; //!
};

template <typename Val>
class hist<2, ::ROOT::RVec<Val>>
    : public qty::query::definition<std::shared_ptr<TH2>(::ROOT::RVec<Val>,
                                                         ::ROOT::RVec<Val>)> {

public:
  hist(const std::string &hname, std::vector<Val> const &,
       std::vector<Val> const &);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<::ROOT::RVec<Val>>,
                    qty::column::observable<::ROOT::RVec<Val>>,
                    double) final override;
  virtual std::shared_ptr<TH2> result() const final override;
  virtual std::shared_ptr<TH2>
  merge(std::vector<std::shared_ptr<TH2>> const &results) const final override;

protected:
  // histogram
  std::shared_ptr<TH2> m_hist; //!
};

template <typename Val>
class hist<3, ::ROOT::RVec<Val>>
    : public qty::query::definition<std::shared_ptr<TH3>(
          ::ROOT::RVec<Val>, ::ROOT::RVec<Val>, ::ROOT::RVec<Val>)> {

public:
  hist(const std::string &hname, std::vector<Val> const &,
       std::vector<Val> const &, std::vector<Val> const &);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<::ROOT::RVec<Val>>,
                    qty::column::observable<::ROOT::RVec<Val>>,
                    qty::column::observable<::ROOT::RVec<Val>>,
                    double) final override;
  virtual std::shared_ptr<TH3> result() const final override;
  virtual std::shared_ptr<TH3>
  merge(std::vector<std::shared_ptr<TH3>> const &results) const final override;

protected:
  // histogram
  std::shared_ptr<TH3> m_hist; //!
};

} // namespace ROOT

} // namespace queryosity

template <typename Val>
queryosity::ROOT::hist<1, Val>::hist(const std::string &hname,
                                     unsigned int nbins, double xmin,
                                     double xmax) {
  m_hist = make_hist<1, float>(nbins, xmin, xmax);
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Val>
queryosity::ROOT::hist<1, Val>::hist(const std::string &hname,
                                     const std::vector<Val> &xbins) {
  if constexpr (std::is_same_v<Val, std::string>) {
    m_hist = make_hist<1, float>(xbins.size(), 0, xbins.size());
    for (unsigned int ix = 0; ix < xbins.size(); ++ix) {
      m_hist->GetXaxis()->SetBinLabel(ix + 1, xbins[ix].c_str());
    }
  } else {
    m_hist = make_hist<1, float>(xbins);
  }
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Val>
void queryosity::ROOT::hist<1, Val>::fill(qty::column::observable<Val> x,
                                          double w) {
  m_hist->Fill(x.value(), w);
}

template <typename Val>
std::shared_ptr<TH1> queryosity::ROOT::hist<1, Val>::result() const {
  return m_hist;
}

template <typename Val>
std::shared_ptr<TH1> queryosity::ROOT::hist<1, Val>::merge(
    std::vector<std::shared_ptr<TH1>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH1>(static_cast<TH1 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  merged_result->SetDirectory(nullptr);
  return merged_result;
}

template <typename Val>
queryosity::ROOT::hist<2, Val>::hist(const std::string &hname,
                                     std::vector<Val> const &xbins,
                                     std::vector<Val> const &ybins) {
  if constexpr (std::is_same_v<Val, std::string>) {
    m_hist = std::static_pointer_cast<TH2>(make_hist<2, float>(
        xbins.size(), 0, xbins.size(), ybins.size(), 0, ybins.size()));
    for (unsigned int ix = 0; ix < xbins.size(); ++ix) {
      m_hist->GetXaxis()->SetBinLabel(ix + 1, xbins[ix].c_str());
    }
    for (unsigned int iy = 0; iy < xbins.size(); ++iy) {
      m_hist->GetYaxis()->SetBinLabel(iy + 1, ybins[iy].c_str());
    }
  } else {
    m_hist = std::static_pointer_cast<TH2>(make_hist<2, Val>(xbins, ybins));
    m_hist->SetNameTitle(hname.c_str(), hname.c_str());
  }
}

template <typename Val>
queryosity::ROOT::hist<2, Val>::hist(const std::string &hname, unsigned int nx,
                                     double xmin, double xmax, unsigned int ny,
                                     double ymin, double ymax) {
  m_hist = std::static_pointer_cast<TH2>(
      make_hist<2, Val>(nx, xmin, xmax, ny, ymin, ymax));
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Val>
void queryosity::ROOT::hist<2, Val>::fill(qty::column::observable<Val> x,
                                          qty::column::observable<Val> y,
                                          double w) {
  m_hist->Fill(x.value(), y.value(), w);
}

template <typename Val>
std::shared_ptr<TH2> queryosity::ROOT::hist<2, Val>::merge(
    std::vector<std::shared_ptr<TH2>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH2>(static_cast<TH2 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  merged_result->SetDirectory(nullptr);
  return merged_result;
}

template <typename Val>
std::shared_ptr<TH2> queryosity::ROOT::hist<2, Val>::result() const {
  return m_hist;
}

template <typename Val>
queryosity::ROOT::hist<3, Val>::hist(const std::string &hname,
                                     std::vector<Val> const &xbins,
                                     std::vector<Val> const &ybins,
                                     std::vector<Val> const &zbins)
    : qty::query::definition<std::shared_ptr<TH3>(Val, Val, Val)>() {
  m_hist =
      std::static_pointer_cast<TH3>(make_hist<3, Val>(xbins, ybins, zbins));
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Val>
void queryosity::ROOT::hist<3, Val>::fill(qty::column::observable<Val> x,
                                          qty::column::observable<Val> y,
                                          qty::column::observable<Val> z,
                                          double w) {
  m_hist->Fill(x.value(), y.value(), z.value(), w);
}

template <typename Val>
std::shared_ptr<TH3> queryosity::ROOT::hist<3, Val>::merge(
    std::vector<std::shared_ptr<TH3>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH3>(static_cast<TH3 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  merged_result->SetDirectory(nullptr);
  return merged_result;
}

template <typename Val>
std::shared_ptr<TH3> queryosity::ROOT::hist<3, Val>::result() const {
  return m_hist;
}

// vector<T>

template <typename Val>
queryosity::ROOT::hist<1, ::ROOT::RVec<Val>>::hist(const std::string &hname,
                                                   unsigned int nbins,
                                                   double xmin, double xmax) {
  m_hist = make_hist<1, float>(nbins, xmin, xmax);
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Val>
queryosity::ROOT::hist<1, ::ROOT::RVec<Val>>::hist(
    const std::string &hname, std::vector<Val> const &xbins) {
  m_hist = make_hist<1, float>(std::vector<Val>(xbins.begin(), xbins.end()));
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Val>
void queryosity::ROOT::hist<1, ::ROOT::RVec<Val>>::fill(
    qty::column::observable<::ROOT::RVec<Val>> xs, double w) {
  for (size_t ix = 0; ix < xs->size(); ++ix) {
    m_hist->Fill(xs.value()[ix], w);
  }
}

template <typename Val>
std::shared_ptr<TH1> queryosity::ROOT::hist<1, ::ROOT::RVec<Val>>::merge(
    std::vector<std::shared_ptr<TH1>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH1>(static_cast<TH1 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  merged_result->SetDirectory(nullptr);
  return merged_result;
}

template <typename Val>
std::shared_ptr<TH1>
queryosity::ROOT::hist<1, ::ROOT::RVec<Val>>::result() const {
  return m_hist;
}

template <typename Val>
queryosity::ROOT::hist<2, ::ROOT::RVec<Val>>::hist(
    const std::string &hname, std::vector<Val> const &xbins,
    std::vector<Val> const &ybins) {
  m_hist = std::static_pointer_cast<TH2>(make_hist<2, Val>(xbins, ybins));
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Val>
void queryosity::ROOT::hist<2, ::ROOT::RVec<Val>>::fill(
    qty::column::observable<::ROOT::RVec<Val>> xs,
    qty::column::observable<::ROOT::RVec<Val>> ys, double w) {
  if (xs->size() != ys->size()) {
    throw std::runtime_error("x- and y-arrays do not share the same size");
  }
  for (size_t ix = 0; ix < xs->size(); ++ix) {
    m_hist->Fill(xs.value()[ix], ys.value()[ix], w);
  }
}

template <typename Val>
std::shared_ptr<TH2> queryosity::ROOT::hist<2, ::ROOT::RVec<Val>>::merge(
    std::vector<std::shared_ptr<TH2>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH2>(static_cast<TH2 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  merged_result->SetDirectory(nullptr);
  return merged_result;
}

template <typename Val>
std::shared_ptr<TH2>
queryosity::ROOT::hist<2, ::ROOT::RVec<Val>>::result() const {
  return m_hist;
}

template <typename Val>
queryosity::ROOT::hist<3, ::ROOT::RVec<Val>>::hist(
    const std::string &hname, std::vector<Val> const &xbins,
    std::vector<Val> const &ybins, std::vector<Val> const &zbins) {
  m_hist =
      std::static_pointer_cast<TH3>(make_hist<3, Val>(xbins, ybins, zbins));
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Val>
void queryosity::ROOT::hist<3, ::ROOT::RVec<Val>>::fill(
    qty::column::observable<::ROOT::RVec<Val>> xs,
    qty::column::observable<::ROOT::RVec<Val>> ys,
    qty::column::observable<::ROOT::RVec<Val>> zs, double w) {
  if (xs->size() != ys->size()) {
    throw std::runtime_error("x- and y-arrays do not share the same size");
  }
  if (xs->size() != zs->size()) {
    throw std::runtime_error("x- and z-arrays do not share the same size");
  }
  for (size_t ix = 0; ix < xs->size(); ++ix) {
    m_hist->Fill(xs.value()[ix], ys.value()[ix], zs.value()[ix], w);
  }
}

template <typename Val>
std::shared_ptr<TH3> queryosity::ROOT::hist<3, ::ROOT::RVec<Val>>::merge(
    std::vector<std::shared_ptr<TH3>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH3>(static_cast<TH3 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  merged_result->SetDirectory(nullptr);
  return merged_result;
}

template <typename Val>
std::shared_ptr<TH3>
queryosity::ROOT::hist<3, ::ROOT::RVec<Val>>::result() const {
  return m_hist;
}