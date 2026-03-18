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

#include "hist.hpp"

#include <ROOT/RVec.hxx>

#include <queryosity.hpp>

#include <memory>
#include <string>
#include <vector>

namespace queryosity {

namespace ROOT {

template <int Dim, typename Val> class hgrid;

using hgrid_2d_t = std::vector<std::vector<std::shared_ptr<TH1>>>;
using hgrid_3d_t = std::vector<std::vector<std::vector<std::shared_ptr<TH1>>>>;

template <typename Val>
class hgrid<2, Val>
    : public qty::query::definition<hgrid_2d_t(double, double, Val)> {

public:
  hgrid(const std::string &hname, const std::vector<double> &,
        const std::vector<double> &, unsigned int, double, double);
  hgrid(const std::string &hname, const std::vector<double> &,
        const std::vector<double> &, const std::vector<double> &);
  virtual ~hgrid() = default;

  virtual void fill(qty::column::observable<double>,
                    qty::column::observable<double>,
                    qty::column::observable<Val>, double) final override;
  virtual hgrid_2d_t result() const final override;
  virtual hgrid_2d_t
  merge(std::vector<hgrid_2d_t> const &results) const final override;

protected:
  // grid histogram
  std::vector<double> m_grid_x;
  std::vector<double> m_grid_y;
  hgrid_2d_t m_hgrid; //!
};

template <typename Val>
class hgrid<3, Val>
    : public qty::query::definition<hgrid_3d_t(double, double, double, Val)> {

public:
  hgrid(const std::string &hname, std::vector<double> const &,
        std::vector<double> const &, std::vector<double> const &,
        std::vector<double> const &);
  hgrid(const std::string &hname, std::vector<double> const &,
        std::vector<double> const &, std::vector<double> const &, unsigned int,
        double, double);
  virtual ~hgrid() = default;

  virtual void fill(qty::column::observable<double>,
                    qty::column::observable<double>,
                    qty::column::observable<double>,
                    qty::column::observable<Val>, double) final override;
  virtual hgrid_3d_t result() const final override;
  virtual hgrid_3d_t
  merge(std::vector<hgrid_3d_t> const &results) const final override;

protected:
  std::vector<double> m_grid_x;
  std::vector<double> m_grid_y;
  std::vector<double> m_grid_z;
  hgrid_3d_t m_hgrid; //!
};

} // namespace ROOT

} // namespace queryosity

template <typename Val>
queryosity::ROOT::hgrid<2, Val>::hgrid(const std::string &hname,
                                       std::vector<double> const &grid_x,
                                       std::vector<double> const &grid_y,
                                       std::vector<double> const &bins)
    : m_grid_x(grid_x), m_grid_y(grid_y) {

  const size_t nx = m_grid_x.size() > 0 ? m_grid_x.size() - 1 : 0;
  const size_t ny = m_grid_y.size() > 0 ? m_grid_y.size() - 1 : 0;

  m_hgrid.resize(nx);

  for (size_t i = 0; i < nx; ++i) {
    m_hgrid[i].resize(ny);

    for (size_t j = 0; j < ny; ++j) {
      m_hgrid[i][j] = make_hist<1, Val>(bins);

      if (m_hgrid[i][j]) {
        m_hgrid[i][j]->SetDirectory(nullptr);
      }
    }
  }
}

template <typename Val>
queryosity::ROOT::hgrid<2, Val>::hgrid(const std::string &hname,
                                       std::vector<double> const &grid_x,
                                       std::vector<double> const &grid_y,
                                       unsigned int n, double min, double max)
    : m_grid_x(grid_x), m_grid_y(grid_y) {

  const size_t nx = m_grid_x.size() > 0 ? m_grid_x.size() - 1 : 0;
  const size_t ny = m_grid_y.size() > 0 ? m_grid_y.size() - 1 : 0;

  m_hgrid.resize(nx);

  for (size_t i = 0; i < nx; ++i) {
    m_hgrid[i].resize(ny);

    for (size_t j = 0; j < ny; ++j) {
      m_hgrid[i][j] = make_hist<1, Val>(n, min, max);
      m_hgrid[i][j]->SetDirectory(nullptr);
      m_hgrid[i][j]->SetName(hname.c_str());
    }
  }
}

template <typename Val>
void queryosity::ROOT::hgrid<2, Val>::fill(qty::column::observable<double> x,
                                           qty::column::observable<double> y,
                                           qty::column::observable<Val> z,
                                           double w) {
  auto itx = std::upper_bound(m_grid_x.begin(), m_grid_x.end(), x.value());
  if (itx == m_grid_x.begin() || itx == m_grid_x.end())
    return;

  auto ity = std::upper_bound(m_grid_y.begin(), m_grid_y.end(), y.value());
  if (ity == m_grid_y.begin() || ity == m_grid_y.end())
    return;

  size_t i = std::distance(m_grid_x.begin(), itx) - 1;
  size_t j = std::distance(m_grid_y.begin(), ity) - 1;

  m_hgrid[i][j]->Fill(z.value(), w);
}

template <typename Val>
queryosity::ROOT::hgrid_2d_t queryosity::ROOT::hgrid<2, Val>::merge(
    std::vector<hgrid_2d_t> const &results) const {

  if (results.empty())
    return {};

  hgrid_2d_t merged_result;

  // clone grid structure & first result
  merged_result.resize(results[0].size());
  for (size_t i = 0; i < results[0].size(); ++i) {
    merged_result[i].resize(results[0][i].size());
    for (size_t j = 0; j < results[0][i].size(); ++j) {

      const auto &h = results[0][i][j];
      if (h) {
        merged_result[i][j] =
            std::shared_ptr<TH1>(static_cast<TH1 *>(h->Clone()));
        merged_result[i][j]->SetDirectory(nullptr);
      }
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
queryosity::ROOT::hgrid_2d_t queryosity::ROOT::hgrid<2, Val>::result() const {
  return m_hgrid;
}

template <typename Val>
queryosity::ROOT::hgrid<3, Val>::hgrid(const std::string &hname,
                                       std::vector<double> const &grid_x,
                                       std::vector<double> const &grid_y,
                                       std::vector<double> const &grid_z,
                                       std::vector<double> const &bins)
    : m_grid_x(grid_x), m_grid_y(grid_y), m_grid_z(grid_z) {

  const size_t nx = m_grid_x.size() > 0 ? m_grid_x.size() - 1 : 0;
  const size_t ny = m_grid_y.size() > 0 ? m_grid_y.size() - 1 : 0;
  const size_t nz = m_grid_z.size() > 0 ? m_grid_z.size() - 1 : 0;

  m_hgrid.resize(nx);

  for (size_t i = 0; i < nx; ++i) {
    m_hgrid[i].resize(ny);

    for (size_t j = 0; j < ny; ++j) {
      m_hgrid[i][j].resize(nz);

      for (size_t k = 0; k < nz; ++k) {
        m_hgrid[i][j][k] = make_hist<1, Val>(bins);
        m_hgrid[i][j][k]->SetDirectory(nullptr);
        m_hgrid[i][j][k]->SetName(hname.c_str());
      }
    }
  }
}

template <typename Val>
queryosity::ROOT::hgrid<3, Val>::hgrid(const std::string &hname,
                                       std::vector<double> const &grid_x,
                                       std::vector<double> const &grid_y,
                                       std::vector<double> const &grid_z,
                                       unsigned int n, double min, double max)
    : m_grid_x(grid_x), m_grid_y(grid_y), m_grid_z(grid_z) {

  const size_t nx = m_grid_x.size() > 0 ? m_grid_x.size() - 1 : 0;
  const size_t ny = m_grid_y.size() > 0 ? m_grid_y.size() - 1 : 0;
  const size_t nz = m_grid_z.size() > 0 ? m_grid_z.size() - 1 : 0;

  m_hgrid.resize(nx);

  for (size_t i = 0; i < nx; ++i) {
    m_hgrid[i].resize(ny);

    for (size_t j = 0; j < ny; ++j) {
      m_hgrid[i][j].resize(nz);

      for (size_t k = 0; k < nz; ++k) {
        m_hgrid[i][j][k] = make_hist<1, Val>(n, min, max);
        m_hgrid[i][j][k]->SetDirectory(nullptr);
        m_hgrid[i][j][k]->SetName(hname.c_str());
      }
    }
  }
}

template <typename Val>
void queryosity::ROOT::hgrid<3, Val>::fill(qty::column::observable<double> x,
                                           qty::column::observable<double> y,
                                           qty::column::observable<double> z,
                                           qty::column::observable<Val> v,
                                           double w) {
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

  m_hgrid[i][j][k]->Fill(v.value(), w);
}

template <typename Val>
queryosity::ROOT::hgrid_3d_t queryosity::ROOT::hgrid<3, Val>::result() const {
  return m_hgrid;
}

template <typename Val>
queryosity::ROOT::hgrid_3d_t queryosity::ROOT::hgrid<3, Val>::merge(
    std::vector<hgrid_3d_t> const &results) const {

  if (results.empty())
    return {};

  hgrid_3d_t merged_result;

  // clone grid structure & first result
  merged_result.resize(results[0].size());
  for (size_t i = 0; i < results[0].size(); ++i) {
    merged_result[i].resize(results[0][i].size());
    for (size_t j = 0; j < results[0][i].size(); ++j) {
      merged_result[i][j].resize(results[0][i][j].size());
      for (size_t k = 0; k < results[0][i][j].size(); ++k) {

        const auto &h = results[0][i][j][k];
        if (h) {
          merged_result[i][j][k] =
              std::shared_ptr<TH1>(static_cast<TH1 *>(h->Clone()));
          merged_result[i][j][k]->SetDirectory(nullptr);
        }
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