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

template <int Dim, typename Val> class hgrid_with_toys;

using hgrid_1d_with_toys_t = std::vector<std::shared_ptr<TH1Bootstrap>>;
using hgrid_2d_with_toys_t =
    std::vector<std::vector<std::shared_ptr<TH1Bootstrap>>>;
using hgrid_3d_with_toys_t =
    std::vector<std::vector<std::vector<std::shared_ptr<TH1Bootstrap>>>>;

template <typename Val>
class hgrid_with_toys<1, Val>
    : public qty::query::definition<hgrid_1d_with_toys_t(
          double, Val, unsigned int, unsigned int, unsigned int)> {

public:
  hgrid_with_toys(const std::string &hname, const std::vector<double> &,
                  unsigned int, double, double, unsigned int);
  hgrid_with_toys(const std::string &hname, const std::vector<double> &,
                  const std::vector<double> &, unsigned int);
  virtual ~hgrid_with_toys() = default;

  virtual void fill(qty::column::observable<double>,
                    qty::column::observable<Val>,
                    qty::column::observable<unsigned int>,
                    qty::column::observable<unsigned int>,
                    qty::column::observable<unsigned int>,
                    double) final override;
  virtual hgrid_1d_with_toys_t result() const final override;
  virtual hgrid_1d_with_toys_t
  merge(std::vector<hgrid_1d_with_toys_t> const &results) const final override;

protected:
  // grid histogram
  std::vector<double> m_grid_x;
  hgrid_1d_with_toys_t m_hgrid_with_toys; //!
};

template <typename Val>
class hgrid_with_toys<2, Val>
    : public qty::query::definition<hgrid_2d_with_toys_t(
          double, double, Val, unsigned int, unsigned int, unsigned int)> {

public:
  hgrid_with_toys(const std::string &hname, const std::vector<double> &,
                  const std::vector<double> &, unsigned int, double, double,
                  unsigned int);
  hgrid_with_toys(const std::string &hname, const std::vector<double> &,
                  const std::vector<double> &, const std::vector<double> &,
                  unsigned int);
  virtual ~hgrid_with_toys() = default;

  virtual void
  fill(qty::column::observable<double>, qty::column::observable<double>,
       qty::column::observable<Val>, qty::column::observable<unsigned int>,
       qty::column::observable<unsigned int>,
       qty::column::observable<unsigned int>, double) final override;
  virtual hgrid_2d_with_toys_t result() const final override;
  virtual hgrid_2d_with_toys_t
  merge(std::vector<hgrid_2d_with_toys_t> const &results) const final override;

protected:
  // grid histogram
  std::vector<double> m_grid_x;
  std::vector<double> m_grid_y;
  hgrid_2d_with_toys_t m_hgrid_with_toys; //!
};

template <typename Val>
class hgrid_with_toys<3, Val>
    : public qty::query::definition<hgrid_3d_with_toys_t(
          double, double, double, Val, unsigned int, unsigned int,
          unsigned int)> {

public:
  hgrid_with_toys(const std::string &hname, std::vector<double> const &,
                  std::vector<double> const &, std::vector<double> const &,
                  std::vector<double> const &, unsigned int);
  hgrid_with_toys(const std::string &hname, std::vector<double> const &,
                  std::vector<double> const &, std::vector<double> const &,
                  unsigned int, double, double, unsigned int);
  virtual ~hgrid_with_toys() = default;

  virtual void
  fill(qty::column::observable<double>, qty::column::observable<double>,
       qty::column::observable<double>, qty::column::observable<Val>,
       qty::column::observable<unsigned int>,
       qty::column::observable<unsigned int>,
       qty::column::observable<unsigned int>, double) final override;
  virtual hgrid_3d_with_toys_t result() const final override;
  virtual hgrid_3d_with_toys_t
  merge(std::vector<hgrid_3d_with_toys_t> const &results) const final override;

protected:
  std::vector<double> m_grid_x;
  std::vector<double> m_grid_y;
  std::vector<double> m_grid_z;
  hgrid_3d_with_toys_t m_hgrid_with_toys; //!
};

} // namespace ROOT

} // namespace queryosity

template <typename Val>
queryosity::ROOT::hgrid_with_toys<1, Val>::hgrid_with_toys(
    const std::string &hname, const std::vector<double> &grid_x,
    const std::vector<double> &bins, unsigned int ntoys)
    : m_grid_x(grid_x) {

  const size_t nx = m_grid_x.size() > 0 ? m_grid_x.size() - 1 : 0;

  m_hgrid_with_toys.resize(nx);

  for (size_t i = 0; i < nx; ++i) {
    m_hgrid_with_toys[i] = std::make_shared<TH1FBootstrap>(
        hname.c_str(), hname.c_str(), bins.size() - 1, &bins[0], ntoys);
    m_hgrid_with_toys[i]->SetDirectory(nullptr);
    m_hgrid_with_toys[i]->SetName(hname.c_str());
    m_hgrid_with_toys[i]->Sumw2();
  }
}

template <typename Val>
queryosity::ROOT::hgrid_with_toys<1, Val>::hgrid_with_toys(
    const std::string &hname, const std::vector<double> &grid_x,
    unsigned int nbins, double min, double max, unsigned int ntoys)
    : m_grid_x(grid_x) {

  const size_t nx = m_grid_x.size() > 0 ? m_grid_x.size() - 1 : 0;

  m_hgrid_with_toys.resize(nx);

  for (size_t i = 0; i < nx; ++i) {
    m_hgrid_with_toys[i] = std::make_shared<TH1FBootstrap>(
        hname.c_str(), hname.c_str(), nbins, min, max, ntoys);
    m_hgrid_with_toys[i]->SetDirectory(nullptr);
    m_hgrid_with_toys[i]->SetName(hname.c_str());
    m_hgrid_with_toys[i]->Sumw2();
  }
}

template <typename Val>
void queryosity::ROOT::hgrid_with_toys<1, Val>::fill(
    qty::column::observable<double> x, qty::column::observable<Val> y,
    qty::column::observable<unsigned int> run,
    qty::column::observable<unsigned int> event,
    qty::column::observable<unsigned int> channel, double w) {

  auto itx = std::upper_bound(m_grid_x.begin(), m_grid_x.end(), x.value());
  if (itx == m_grid_x.begin() || itx == m_grid_x.end())
    return;

  size_t i = std::distance(m_grid_x.begin(), itx) - 1;

  m_hgrid_with_toys[i]->Fill(y.value(), w, run.value(), event.value(),
                             channel.value());
}

template <typename Val>
queryosity::ROOT::hgrid_1d_with_toys_t
queryosity::ROOT::hgrid_with_toys<1, Val>::result() const {
  return m_hgrid_with_toys;
}

template <typename Val>
queryosity::ROOT::hgrid_1d_with_toys_t
queryosity::ROOT::hgrid_with_toys<1, Val>::merge(
    std::vector<hgrid_1d_with_toys_t> const &results) const {

  if (results.empty())
    return {};

  hgrid_1d_with_toys_t merged_result;

  // clone structure & first result
  merged_result.resize(results[0].size());
  for (size_t i = 0; i < results[0].size(); ++i) {

    const auto &h = results[0][i];
    merged_result[i] =
        std::shared_ptr<TH1Bootstrap>(static_cast<TH1Bootstrap *>(h->Clone()));
    merged_result[i]->SetDirectory(nullptr);
  }

  // add remaining slots
  for (size_t t = 1; t < results.size(); ++t) {
    for (size_t i = 0; i < merged_result.size(); ++i) {

      const auto &src = results[t][i];
      auto &dst = merged_result[i];

      if (src && dst) {
        dst->Add(src.get());
      }
    }
  }

  return merged_result;
}

template <typename Val>
queryosity::ROOT::hgrid_with_toys<2, Val>::hgrid_with_toys(
    const std::string &hname, std::vector<double> const &grid_x,
    std::vector<double> const &grid_y, std::vector<double> const &bins,
    unsigned int ntoys)
    : m_grid_x(grid_x), m_grid_y(grid_y) {

  const size_t nx = m_grid_x.size() > 0 ? m_grid_x.size() - 1 : 0;
  const size_t ny = m_grid_y.size() > 0 ? m_grid_y.size() - 1 : 0;

  m_hgrid_with_toys.resize(nx);

  for (size_t i = 0; i < nx; ++i) {
    m_hgrid_with_toys[i].resize(ny);

    for (size_t j = 0; j < ny; ++j) {
      m_hgrid_with_toys[i][j] = std::make_shared<TH1FBootstrap>(
          hname.c_str(), hname.c_str(), bins.size() - 1, &bins[0], ntoys);
      m_hgrid_with_toys[i][j]->SetDirectory(nullptr);
      m_hgrid_with_toys[i][j]->SetName(hname.c_str());
      m_hgrid_with_toys[i][j]->Sumw2();
    }
  }
}

template <typename Val>
queryosity::ROOT::hgrid_with_toys<2, Val>::hgrid_with_toys(
    const std::string &hname, std::vector<double> const &grid_x,
    std::vector<double> const &grid_y, unsigned int nbins, double min,
    double max, unsigned int ntoys)
    : m_grid_x(grid_x), m_grid_y(grid_y) {

  const size_t nx = m_grid_x.size() > 0 ? m_grid_x.size() - 1 : 0;
  const size_t ny = m_grid_y.size() > 0 ? m_grid_y.size() - 1 : 0;

  m_hgrid_with_toys.resize(nx);

  for (size_t i = 0; i < nx; ++i) {
    m_hgrid_with_toys[i].resize(ny);

    for (size_t j = 0; j < ny; ++j) {
      m_hgrid_with_toys[i][j] = std::make_shared<TH1FBootstrap>(
          hname.c_str(), hname.c_str(), nbins, min, max, ntoys);
      m_hgrid_with_toys[i][j]->SetDirectory(nullptr);
      m_hgrid_with_toys[i][j]->SetName(hname.c_str());
      m_hgrid_with_toys[i][j]->Sumw2();
    }
  }
}

template <typename Val>
void queryosity::ROOT::hgrid_with_toys<2, Val>::fill(
    qty::column::observable<double> x, qty::column::observable<double> y,
    qty::column::observable<Val> z, qty::column::observable<unsigned int> run,
    qty::column::observable<unsigned int> event,
    qty::column::observable<unsigned int> channel, double w) {
  auto itx = std::upper_bound(m_grid_x.begin(), m_grid_x.end(), x.value());
  if (itx == m_grid_x.begin() || itx == m_grid_x.end())
    return;

  auto ity = std::upper_bound(m_grid_y.begin(), m_grid_y.end(), y.value());
  if (ity == m_grid_y.begin() || ity == m_grid_y.end())
    return;

  size_t i = std::distance(m_grid_x.begin(), itx) - 1;
  size_t j = std::distance(m_grid_y.begin(), ity) - 1;

  m_hgrid_with_toys[i][j]->Fill(z.value(), w, run.value(), event.value(),
                                channel.value());
}

template <typename Val>
queryosity::ROOT::hgrid_2d_with_toys_t
queryosity::ROOT::hgrid_with_toys<2, Val>::merge(
    std::vector<hgrid_2d_with_toys_t> const &results) const {

  if (results.empty())
    return {};

  hgrid_2d_with_toys_t merged_result;

  // clone grid structure & first result
  merged_result.resize(results[0].size());
  for (size_t i = 0; i < results[0].size(); ++i) {
    merged_result[i].resize(results[0][i].size());
    for (size_t j = 0; j < results[0][i].size(); ++j) {

      const auto &h = results[0][i][j];
      merged_result[i][j] = std::shared_ptr<TH1Bootstrap>(
          static_cast<TH1Bootstrap *>(h->Clone()));
      merged_result[i][j]->SetDirectory(nullptr);
    }
  }

  // add remaining slots
  for (size_t t = 1; t < results.size(); ++t) {
    for (size_t i = 0; i < merged_result.size(); ++i) {
      for (size_t j = 0; j < merged_result[i].size(); ++j) {

        const auto &src = results[t][i][j];
        auto &dst = merged_result[i][j];

        if (src && dst) {
          dst->Add(src.get());
        }
      }
    }
  }

  return merged_result;
}

template <typename Val>
queryosity::ROOT::hgrid_2d_with_toys_t
queryosity::ROOT::hgrid_with_toys<2, Val>::result() const {
  return m_hgrid_with_toys;
}

template <typename Val>
queryosity::ROOT::hgrid_with_toys<3, Val>::hgrid_with_toys(
    const std::string &hname, std::vector<double> const &grid_x,
    std::vector<double> const &grid_y, std::vector<double> const &grid_z,
    std::vector<double> const &bins, unsigned int ntoys)
    : m_grid_x(grid_x), m_grid_y(grid_y), m_grid_z(grid_z) {

  const size_t nx = m_grid_x.size() > 0 ? m_grid_x.size() - 1 : 0;
  const size_t ny = m_grid_y.size() > 0 ? m_grid_y.size() - 1 : 0;
  const size_t nz = m_grid_z.size() > 0 ? m_grid_z.size() - 1 : 0;

  m_hgrid_with_toys.resize(nx);

  for (size_t i = 0; i < nx; ++i) {
    m_hgrid_with_toys[i].resize(ny);

    for (size_t j = 0; j < ny; ++j) {
      m_hgrid_with_toys[i][j].resize(nz);

      for (size_t k = 0; k < nz; ++k) {
        m_hgrid_with_toys[i][j][k] = std::make_shared<TH1FBootstrap>(
            hname.c_str(), hname.c_str(), bins.size() - 1, &bins[0], ntoys);
        m_hgrid_with_toys[i][j][k]->SetDirectory(nullptr);
        m_hgrid_with_toys[i][j][k]->SetName(hname.c_str());
        m_hgrid_with_toys[i][j][k]->Sumw2();
      }
    }
  }
}

template <typename Val>
queryosity::ROOT::hgrid_with_toys<3, Val>::hgrid_with_toys(
    const std::string &hname, std::vector<double> const &grid_x,
    std::vector<double> const &grid_y, std::vector<double> const &grid_z,
    unsigned int nbins, double min, double max, unsigned int ntoys)
    : m_grid_x(grid_x), m_grid_y(grid_y), m_grid_z(grid_z) {

  const size_t nx = m_grid_x.size() > 0 ? m_grid_x.size() - 1 : 0;
  const size_t ny = m_grid_y.size() > 0 ? m_grid_y.size() - 1 : 0;
  const size_t nz = m_grid_z.size() > 0 ? m_grid_z.size() - 1 : 0;

  m_hgrid_with_toys.resize(nx);

  for (size_t i = 0; i < nx; ++i) {
    m_hgrid_with_toys[i].resize(ny);

    for (size_t j = 0; j < ny; ++j) {
      m_hgrid_with_toys[i][j].resize(nz);

      for (size_t k = 0; k < nz; ++k) {
        m_hgrid_with_toys[i][j][k] = std::make_shared<TH1FBootstrap>(
            hname.c_str(), hname.c_str(), nbins, min, max, ntoys);
        m_hgrid_with_toys[i][j][k]->SetDirectory(nullptr);
        m_hgrid_with_toys[i][j][k]->SetName(hname.c_str());
        m_hgrid_with_toys[i][j][k]->Sumw2();
      }
    }
  }
}

template <typename Val>
void queryosity::ROOT::hgrid_with_toys<3, Val>::fill(
    qty::column::observable<double> x, qty::column::observable<double> y,
    qty::column::observable<double> z, qty::column::observable<Val> v,
    qty::column::observable<unsigned int> run,
    qty::column::observable<unsigned int> event,
    qty::column::observable<unsigned int> channel, double w) {
  auto itx = std::upper_bound(m_grid_x.begin(), m_grid_x.end(), x.value());
  if (itx == m_grid_x.begin() || itx == m_grid_x.end())
    return;

  auto ity = std::upper_bound(m_grid_y.begin(), m_grid_y.end(), y.value());
  if (ity == m_grid_y.begin() || ity == m_grid_y.end())
    return;

  auto itz = std::upper_bound(m_grid_z.begin(), m_grid_z.end(), z.value());
  if (itz == m_grid_z.begin() || itz == m_grid_z.end())
    return;

  size_t i = std::distance(m_grid_x.begin(), itx) - 1;
  size_t j = std::distance(m_grid_y.begin(), ity) - 1;
  size_t k = std::distance(m_grid_z.begin(), itz) - 1;

  m_hgrid_with_toys[i][j][k]->Fill(v.value(), w, run.value(), event.value(),
                                   channel.value());
}

template <typename Val>
queryosity::ROOT::hgrid_3d_with_toys_t
queryosity::ROOT::hgrid_with_toys<3, Val>::result() const {
  return m_hgrid_with_toys;
}

template <typename Val>
queryosity::ROOT::hgrid_3d_with_toys_t
queryosity::ROOT::hgrid_with_toys<3, Val>::merge(
    std::vector<hgrid_3d_with_toys_t> const &results) const {

  if (results.empty())
    return {};

  hgrid_3d_with_toys_t merged_result;

  // clone grid structure & first result
  merged_result.resize(results[0].size());
  for (size_t i = 0; i < results[0].size(); ++i) {
    merged_result[i].resize(results[0][i].size());
    for (size_t j = 0; j < results[0][i].size(); ++j) {
      merged_result[i][j].resize(results[0][i][j].size());
      for (size_t k = 0; k < results[0][i][j].size(); ++k) {

        const auto &h = results[0][i][j][k];
        merged_result[i][j][k] = std::shared_ptr<TH1Bootstrap>(
            static_cast<TH1Bootstrap *>(h->Clone()));
        merged_result[i][j][k]->SetDirectory(nullptr);
      }
    }
  }

  // add remaining slots
  for (size_t t = 1; t < results.size(); ++t) {
    for (size_t i = 0; i < merged_result.size(); ++i) {
      for (size_t j = 0; j < merged_result[i].size(); ++j) {
        for (size_t k = 0; k < merged_result[i][j].size(); ++k) {

          const auto &src = results[t][i][j][k];
          auto &dst = merged_result[i][j][k];

          if (src && dst) {
            dst->Add(src.get());
          }
        }
      }
    }
  }

  return merged_result;
}