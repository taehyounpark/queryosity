#pragma once

#include "TH1F.h"
#include "TH2F.h"
#include "TH3F.h"

#include <ROOT/RVec.hxx>

#include <queryosity.hpp>

#include <memory>
#include <string>
#include <vector>

namespace queryosity {

namespace ROOT {

template <unsigned int Dim, typename Prec>
std::shared_ptr<TH1> make_hist(std::vector<Prec> const &xbins = {0.0, 1.0},
                              std::vector<Prec> const &ybins = {0.0, 1.0},
                              std::vector<Prec> const &zbins = {0.0, 1.0}) {
  std::shared_ptr<TH1> hist;
  if constexpr (Dim == 1) {
    (void)ybins;
    (void)zbins;
    if constexpr (std::is_same_v<Prec, char> || std::is_same_v<Prec, bool>) {
      hist =
          std::shared_ptr<TH1C>(new TH1C("", "", xbins.size() - 1, &xbins[0]));
    } else if constexpr (std::is_same_v<Prec, int>) {
      hist =
          std::shared_ptr<TH1I>(new TH1I("", "", xbins.size() - 1, &xbins[0]));
    } else if constexpr (std::is_same_v<Prec, float>) {
      hist =
          std::shared_ptr<TH1F>(new TH1F("", "", xbins.size() - 1, &xbins[0]));
    } else if constexpr (std::is_same_v<Prec, double>) {
      hist =
          std::shared_ptr<TH1D>(new TH1D("", "", xbins.size() - 1, &xbins[0]));
    }
  } else if constexpr (Dim == 2) {
    (void)zbins;
    if constexpr (std::is_same_v<Prec, char> || std::is_same_v<Prec, bool>) {
      hist = std::shared_ptr<TH2C>(new TH2C("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0]));
    } else if constexpr (std::is_same_v<Prec, int>) {
      hist = std::shared_ptr<TH2I>(new TH2I("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0]));
    } else if constexpr (std::is_same_v<Prec, float>) {
      hist = std::shared_ptr<TH2F>(new TH2F("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0]));
    } else if constexpr (std::is_same_v<Prec, double>) {
      hist = std::shared_ptr<TH2D>(new TH2D("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0]));
    }
  } else if constexpr (Dim == 3) {
    if constexpr (std::is_same_v<Prec, char> || std::is_same_v<Prec, bool>) {
      hist = std::shared_ptr<TH3C>(new TH3C("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0],
                                            zbins.size() - 1, &zbins[0]));
    } else if constexpr (std::is_same_v<Prec, int>) {
      hist = std::shared_ptr<TH3I>(new TH3I("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0],
                                            zbins.size() - 1, &zbins[0]));
    } else if constexpr (std::is_same_v<Prec, float>) {
      hist = std::shared_ptr<TH3F>(new TH3F("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0],
                                            zbins.size() - 1, &zbins[0]));
    } else if constexpr (std::is_same_v<Prec, double>) {
      hist = std::shared_ptr<TH3D>(new TH3D("", "", xbins.size() - 1, &xbins[0],
                                            ybins.size() - 1, &ybins[0],
                                            zbins.size() - 1, &zbins[0]));
    }
  }
  hist->SetDirectory(nullptr);
  return hist;
}

// create the appropriate histogram given the dimensionality and precision of
// columns. e.g. make_hist<1,float> -> TH1F
template <unsigned int Dim, typename Prec>
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
    if constexpr (std::is_same_v<Prec, char> || std::is_same_v<Prec, bool>) {
      hist = std::shared_ptr<TH1C>(new TH1C("", "", nxbins, xmin, xmax));
    } else if constexpr (std::is_same_v<Prec, int>) {
      hist = std::shared_ptr<TH1I>(new TH1I("", "", nxbins, xmin, xmax));
    } else if constexpr (std::is_same_v<Prec, float>) {
      hist = std::shared_ptr<TH1F>(new TH1F("", "", nxbins, xmin, xmax));
    } else if constexpr (std::is_same_v<Prec, double>) {
      hist = std::shared_ptr<TH1D>(new TH1D("", "", nxbins, xmin, xmax));
    }
  } else if constexpr (Dim == 2) {
    (void)nzbins;
    (void)zmin;
    (void)zmax;
    if constexpr (std::is_same_v<Prec, char> || std::is_same_v<Prec, bool>) {
      hist = std::shared_ptr<TH2C>(
          new TH2C("", "", nxbins, xmin, xmax, nybins, ymin, ymax));
    } else if constexpr (std::is_same_v<Prec, int>) {
      hist = std::shared_ptr<TH2I>(
          new TH2I("", "", nxbins, xmin, xmax, nybins, ymin, ymax));
    } else if constexpr (std::is_same_v<Prec, float>) {
      hist = std::shared_ptr<TH2F>(
          new TH2F("", "", nxbins, xmin, xmax, nybins, ymin, ymax));
    } else if constexpr (std::is_same_v<Prec, double>) {
      hist = std::shared_ptr<TH2D>(
          new TH2D("", "", nxbins, xmin, xmax, nybins, ymin, ymax));
    }
  } else if constexpr (Dim == 3) {
    if constexpr (std::is_same_v<Prec, char> || std::is_same_v<Prec, bool>) {
      hist = std::shared_ptr<TH3C>(new TH3C("", "", nxbins, xmin, xmax, nybins,
                                            ymin, ymax, nzbins, zmin, zmax));
    } else if constexpr (std::is_same_v<Prec, int>) {
      hist = std::shared_ptr<TH3I>(new TH3I("", "", nxbins, xmin, xmax, nybins,
                                            ymin, ymax, nzbins, zmin, zmax));
    } else if constexpr (std::is_same_v<Prec, float>) {
      hist = std::shared_ptr<TH3F>(new TH3F("", "", nxbins, xmin, xmax, nybins,
                                            ymin, ymax, nzbins, zmin, zmax));
    } else if constexpr (std::is_same_v<Prec, double>) {
      hist = std::shared_ptr<TH3D>(new TH3D("", "", nxbins, xmin, xmax, nybins,
                                            ymin, ymax, nzbins, zmin, zmax));
    }
  }
  hist->SetDirectory(nullptr);
  return hist;
}

std::shared_ptr<TH1> clone_hist(std::shared_ptr<TH1> hist) {
  auto cloned = std::shared_ptr<TH1>(static_cast<TH1 *>(hist->Clone()));
  cloned->SetDirectory(nullptr);
  return cloned;
}

template <int Dim, typename Prec> class hist;

template <typename Prec>
class hist<1, Prec>
    : public qty::query::definition<std::shared_ptr<TH1>(Prec)> {

public:
  hist(const std::string &hname = "", unsigned int = 1, double = 0.0,
       double = 1.0);
  hist(const std::string &hname, const std::vector<Prec> &);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<Prec>, double) final override;
  virtual std::shared_ptr<TH1> result() const final override;
  virtual std::shared_ptr<TH1>
  merge(std::vector<std::shared_ptr<TH1>> const &results) const final override;

protected:
  // histogram
  std::shared_ptr<TH1> m_hist; //!
  std::vector<Prec> m_xbins;
};

template <typename Prec>
class hist<2, Prec>
    : public qty::query::definition<std::shared_ptr<TH2>(Prec, Prec)> {

public:
  hist(const std::string &hname, std::vector<Prec> const &,
       std::vector<Prec> const &);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<Prec>,
                    qty::column::observable<Prec>, double) final override;
  virtual std::shared_ptr<TH2> result() const final override;
  virtual std::shared_ptr<TH2>
  merge(std::vector<std::shared_ptr<TH2>> const &results) const final override;

protected:
  std::shared_ptr<TH2> m_hist; //!
};

template <typename Prec>
class hist<3, Prec>
    : public qty::query::definition<std::shared_ptr<TH3>(Prec, Prec, Prec)> {

public:
  hist(const std::string &hname, std::vector<Prec> const &,
       std::vector<Prec> const &, std::vector<Prec> const &);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<Prec>,
                    qty::column::observable<Prec>,
                    qty::column::observable<Prec>, double) final override;
  virtual std::shared_ptr<TH3> result() const final override;
  virtual std::shared_ptr<TH3>
  merge(std::vector<std::shared_ptr<TH3>> const &results) const final override;

protected:
  std::shared_ptr<TH3> m_hist; //!
};

template <typename Prec>
class hist<1, ::ROOT::RVec<Prec>>
    : public qty::query::definition<std::shared_ptr<TH1>(::ROOT::RVec<Prec>)> {

public:
  hist(const std::string &hname = "", unsigned int nbins = 1, double min = 0.0,
       double xmax = 1.0);
  hist(const std::string &hname, std::vector<Prec> const &xbins);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<::ROOT::RVec<Prec>>,
                    double) final override;
  virtual std::shared_ptr<TH1> result() const final override;
  virtual std::shared_ptr<TH1>
  merge(std::vector<std::shared_ptr<TH1>> const &results) const final override;

protected:
  // histogram
  std::shared_ptr<TH1> m_hist; //!
};

template <typename Prec>
class hist<2, ::ROOT::RVec<Prec>>
    : public qty::query::definition<std::shared_ptr<TH2>(::ROOT::RVec<Prec>,
                                                         ::ROOT::RVec<Prec>)> {

public:
  hist(const std::string &hname, std::vector<Prec> const &,
       std::vector<Prec> const &);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<::ROOT::RVec<Prec>>,
                    qty::column::observable<::ROOT::RVec<Prec>>,
                    double) final override;
  virtual std::shared_ptr<TH2> result() const final override;
  virtual std::shared_ptr<TH2>
  merge(std::vector<std::shared_ptr<TH2>> const &results) const final override;

protected:
  // histogram
  std::shared_ptr<TH2> m_hist; //!
};

template <typename Prec>
class hist<3, ::ROOT::RVec<Prec>>
    : public qty::query::definition<std::shared_ptr<TH3>(
          ::ROOT::RVec<Prec>, ::ROOT::RVec<Prec>, ::ROOT::RVec<Prec>)> {

public:
  hist(const std::string &hname, std::vector<Prec> const &,
       std::vector<Prec> const &, std::vector<Prec> const &);
  virtual ~hist() = default;

  virtual void fill(qty::column::observable<::ROOT::RVec<Prec>>,
                    qty::column::observable<::ROOT::RVec<Prec>>,
                    qty::column::observable<::ROOT::RVec<Prec>>,
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

template <typename Prec>
queryosity::ROOT::hist<1, Prec>::hist(const std::string &hname,
                                      unsigned int nbins, double xmin,
                                      double xmax) {
  m_hist = make_hist<1, Prec>(nbins, xmin, xmax);
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Prec>
queryosity::ROOT::hist<1, Prec>::hist(const std::string &hname,
                                      const std::vector<Prec> &xbins)
    : m_xbins(xbins) {
  if constexpr (std::is_same_v<Prec, std::string>) {
    m_hist = make_hist<1, int>(m_xbins.size(), 0, m_xbins.size());
    // note: we do not set bin labels here (see merging)
  } else {
    m_hist =
        make_hist<1, Prec>(std::vector<Prec>(m_xbins.begin(), m_xbins.end()));
  }
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Prec>
void queryosity::ROOT::hist<1, Prec>::fill(qty::column::observable<Prec> x,
                                           double w) {
  if constexpr (std::is_same_v<Prec, std::string>) {
    auto xpos = std::find(m_xbins.begin(), m_xbins.end(), x.value());
    if (xpos == m_xbins.end())
      m_hist->Fill(-1, w);
    else
      m_hist->Fill(std::distance(m_xbins.begin(), xpos), w);
  } else {
    m_hist->Fill(x.value(), w);
  }
}

template <typename Prec>
std::shared_ptr<TH1> queryosity::ROOT::hist<1, Prec>::result() const {
  return m_hist;
}

template <typename Prec>
std::shared_ptr<TH1> queryosity::ROOT::hist<1, Prec>::merge(
    std::vector<std::shared_ptr<TH1>> const &results) const {
  auto merged_result = clone_hist(results[0]);
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  // set bin labels for string histogram
  // note: this has to be done after merging because otherwise TH1 complains
  // that the binnings are inconsistent
  if constexpr (std::is_same_v<Prec, std::string>) {
    for (unsigned int i = 0; i < m_xbins.size(); ++i) {
      merged_result->GetXaxis()->SetBinLabel(i + 1, m_xbins[i].c_str());
    }
  }
  merged_result->SetDirectory(0);
  return merged_result;
}

template <typename Prec>
queryosity::ROOT::hist<2, Prec>::hist(const std::string &hname,
                                      std::vector<Prec> const &xbins,
                                      std::vector<Prec> const &ybins) {
  m_hist = std::static_pointer_cast<TH2>(make_hist<2, Prec>(xbins, ybins));
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Prec>
void queryosity::ROOT::hist<2, Prec>::fill(qty::column::observable<Prec> x,
                                           qty::column::observable<Prec> y,
                                           double w) {
  m_hist->Fill(x.value(), y.value(), w);
}

template <typename Prec>
std::shared_ptr<TH2> queryosity::ROOT::hist<2, Prec>::merge(
    std::vector<std::shared_ptr<TH2>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH2>(static_cast<TH2 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  return merged_result;
}

template <typename Prec>
std::shared_ptr<TH2> queryosity::ROOT::hist<2, Prec>::result() const {
  return m_hist;
}

template <typename Prec>
queryosity::ROOT::hist<3, Prec>::hist(const std::string &hname,
                                      std::vector<Prec> const &xbins,
                                      std::vector<Prec> const &ybins,
                                      std::vector<Prec> const &zbins)
    : qty::query::definition<std::shared_ptr<TH3>(Prec, Prec, Prec)>() {
  m_hist =
      std::static_pointer_cast<TH3>(make_hist<3, Prec>(xbins, ybins, zbins));
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Prec>
void queryosity::ROOT::hist<3, Prec>::fill(qty::column::observable<Prec> x,
                                           qty::column::observable<Prec> y,
                                           qty::column::observable<Prec> z,
                                           double w) {
  m_hist->Fill(x.value(), y.value(), z.value(), w);
}

template <typename Prec>
std::shared_ptr<TH3> queryosity::ROOT::hist<3, Prec>::merge(
    std::vector<std::shared_ptr<TH3>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH3>(static_cast<TH3 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  return merged_result;
}

template <typename Prec>
std::shared_ptr<TH3> queryosity::ROOT::hist<3, Prec>::result() const {
  return m_hist;
}

// vector<T>

template <typename Prec>
queryosity::ROOT::hist<1, ::ROOT::RVec<Prec>>::hist(const std::string &hname,
                                                    unsigned int nbins,
                                                    double xmin, double xmax) {
  m_hist = make_hist<1, Prec>(nbins, xmin, xmax);
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Prec>
queryosity::ROOT::hist<1, ::ROOT::RVec<Prec>>::hist(
    const std::string &hname, std::vector<Prec> const &xbins) {
  m_hist = make_hist<1, Prec>(std::vector<Prec>(xbins.begin(), xbins.end()));
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Prec>
void queryosity::ROOT::hist<1, ::ROOT::RVec<Prec>>::fill(
    qty::column::observable<::ROOT::RVec<Prec>> xs, double w) {
  for (size_t ix = 0; ix < xs->size(); ++ix) {
    m_hist->Fill(xs.value()[ix], w);
  }
}

template <typename Prec>
std::shared_ptr<TH1> queryosity::ROOT::hist<1, ::ROOT::RVec<Prec>>::merge(
    std::vector<std::shared_ptr<TH1>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH1>(static_cast<TH1 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  return merged_result;
}

template <typename Prec>
std::shared_ptr<TH1>
queryosity::ROOT::hist<1, ::ROOT::RVec<Prec>>::result() const {
  return m_hist;
}

template <typename Prec>
queryosity::ROOT::hist<2, ::ROOT::RVec<Prec>>::hist(
    const std::string &hname, std::vector<Prec> const &xbins,
    std::vector<Prec> const &ybins) {
  m_hist = std::static_pointer_cast<TH2>(make_hist<2, Prec>(xbins, ybins));
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Prec>
void queryosity::ROOT::hist<2, ::ROOT::RVec<Prec>>::fill(
    qty::column::observable<::ROOT::RVec<Prec>> xs,
    qty::column::observable<::ROOT::RVec<Prec>> ys, double w) {
  if (xs->size() != ys->size()) {
    throw std::runtime_error("x- and y-arrays do not share the same size");
  }
  for (size_t ix = 0; ix < xs->size(); ++ix) {
    m_hist->Fill(xs.value()[ix], ys.value()[ix], w);
  }
}

template <typename Prec>
std::shared_ptr<TH2> queryosity::ROOT::hist<2, ::ROOT::RVec<Prec>>::merge(
    std::vector<std::shared_ptr<TH2>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH2>(static_cast<TH2 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  return merged_result;
}

template <typename Prec>
std::shared_ptr<TH2>
queryosity::ROOT::hist<2, ::ROOT::RVec<Prec>>::result() const {
  return m_hist;
}

template <typename Prec>
queryosity::ROOT::hist<3, ::ROOT::RVec<Prec>>::hist(
    const std::string &hname, std::vector<Prec> const &xbins,
    std::vector<Prec> const &ybins, std::vector<Prec> const &zbins) {
  m_hist =
      std::static_pointer_cast<TH3>(make_hist<3, Prec>(xbins, ybins, zbins));
  m_hist->SetNameTitle(hname.c_str(), hname.c_str());
}

template <typename Prec>
void queryosity::ROOT::hist<3, ::ROOT::RVec<Prec>>::fill(
    qty::column::observable<::ROOT::RVec<Prec>> xs,
    qty::column::observable<::ROOT::RVec<Prec>> ys,
    qty::column::observable<::ROOT::RVec<Prec>> zs, double w) {
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

template <typename Prec>
std::shared_ptr<TH3> queryosity::ROOT::hist<3, ::ROOT::RVec<Prec>>::merge(
    std::vector<std::shared_ptr<TH3>> const &results) const {
  auto merged_result =
      std::shared_ptr<TH3>(static_cast<TH3 *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  return merged_result;
}

template <typename Prec>
std::shared_ptr<TH3>
queryosity::ROOT::hist<3, ::ROOT::RVec<Prec>>::result() const {
  return m_hist;
}