#pragma once

#include "TProfile.h"
#include "TProfile2D.h"

#include <ROOT/RVec.hxx>

#include <queryosity.hpp>

#include <memory>
#include <string>
#include <vector>

namespace queryosity {

namespace ROOT {

template <unsigned int Dim, typename Val> class prof;

template <typename Val>
class prof<1, Val>
    : public qty::query::definition<std::shared_ptr<TProfile>(Val, Val)> {

public:
  prof(const std::string &hname, const std::vector<double> &);
  virtual ~prof() = default;

  virtual void fill(qty::column::observable<Val>, qty::column::observable<Val>,
                    double) final override;
  virtual std::shared_ptr<TProfile> result() const final override;
  virtual std::shared_ptr<TProfile>
  merge(std::vector<std::shared_ptr<TProfile>> const &results)
      const final override;

protected:
  std::shared_ptr<TProfile> m_prof; //!
};

template <typename Val>
class prof<2, Val> : public qty::query::definition<std::shared_ptr<TProfile2D>(
                         Val, Val, Val)> {

public:
  prof(const std::string &hname, const std::vector<double> &,
       const std::vector<double> &);
  virtual ~prof() = default;

  virtual void fill(qty::column::observable<Val>, qty::column::observable<Val>,
                    qty::column::observable<Val>, double) final override;
  virtual std::shared_ptr<TProfile2D> result() const final override;
  virtual std::shared_ptr<TProfile2D>
  merge(std::vector<std::shared_ptr<TProfile2D>> const &results)
      const final override;

protected:
  std::shared_ptr<TProfile2D> m_prof; //!
};

} // namespace ROOT

} // namespace queryosity

template <typename Val>
queryosity::ROOT::prof<1, Val>::prof(const std::string &hname,
                                     const std::vector<double> &xbins) {
  m_prof = std::make_shared<TProfile>(hname.c_str(), hname.c_str(),
                                      xbins.size() - 1, &xbins[0]);
  m_prof->SetDirectory(0);
}

template <typename Val>
void queryosity::ROOT::prof<1, Val>::fill(qty::column::observable<Val> x,
                                          qty::column::observable<Val> y,
                                          double w) {
  m_prof->Fill(x.value(), y.value(), w);
}

template <typename Val>
std::shared_ptr<TProfile> queryosity::ROOT::prof<1, Val>::result() const {
  return m_prof;
}

template <typename Val>
std::shared_ptr<TProfile> queryosity::ROOT::prof<1, Val>::merge(
    std::vector<std::shared_ptr<TProfile>> const &results) const {
  auto merged_result =
      std::shared_ptr<TProfile>(static_cast<TProfile *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  merged_result->SetDirectory(nullptr);
  return merged_result;
}

template <typename Val>
queryosity::ROOT::prof<2, Val>::prof(const std::string &hname,
                                     std::vector<double> const &xbins,
                                     std::vector<double> const &ybins) {
  m_prof = std::make_shared<TProfile2D>(hname.c_str(), hname.c_str(),
                                        xbins.size() - 1, &xbins[0],
                                        ybins.size() - 1, &ybins[0]);
  m_prof->SetDirectory(0);
}

template <typename Val>
void queryosity::ROOT::prof<2, Val>::fill(qty::column::observable<Val> x,
                                          qty::column::observable<Val> y,
                                          qty::column::observable<Val> z,
                                          double w) {
  m_prof->Fill(x.value(), y.value(), z.value(), w);
}

template <typename Val>
std::shared_ptr<TProfile2D> queryosity::ROOT::prof<2, Val>::result() const {
  return m_prof;
}

template <typename Val>
std::shared_ptr<TProfile2D> queryosity::ROOT::prof<2, Val>::merge(
    std::vector<std::shared_ptr<TProfile2D>> const &results) const {
  auto merged_result = std::shared_ptr<TProfile2D>(
      static_cast<TProfile2D *>(results[0]->Clone()));
  for (size_t islot = 1; islot < results.size(); ++islot) {
    merged_result->Add(results[islot].get());
  }
  merged_result->SetDirectory(nullptr);
  return merged_result;
}