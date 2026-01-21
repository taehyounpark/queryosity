#pragma once

#include "TH1.h"
#include "TH1C.h"
#include "TH1D.h"
#include "TH1F.h"
#include "TH1I.h"

#include "hist.hpp"

#include <ROOT/RVec.hxx>

#include <queryosity.hpp>

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace queryosity {

namespace ROOT {

template <typename Cat, typename Val> class hist_categorized;

template <typename Cat, typename Val>
class hist_categorized : public query::definition<std::map<Cat,std::shared_ptr<TH1>>(Cat, Val)> {

public:
  hist_categorized(const std::string &hname, std::vector<Cat> const &categories,
         std::vector<double> const &xbins);
  virtual ~hist_categorized() = default;

  virtual void fill(column::observable<Cat> cat, column::observable<Val>,
                    double) final override;
  virtual std::map<Cat,std::shared_ptr<TH1>> result() const final override;
  virtual std::map<Cat,std::shared_ptr<TH1>>
  merge(std::vector<std::map<Cat,std::shared_ptr<TH1>>> const &results) const final override;

protected:
  std::map<Cat, std::shared_ptr<TH1>> m_hists;
};

} // namespace ROOT

} // namespace queryosity

template <typename Cat, typename Val>
queryosity::ROOT::hist_categorized<Cat, Val>::hist_categorized(const std::string &hname,
                                           std::vector<Cat> const &categories,
                                           const std::vector<double> &xbins) {
  for (auto const &cat : categories) {
    m_hists[cat] = make_hist<1, Val>(xbins);
    m_hists[cat]->SetNameTitle((hname + "_" + std::to_string(cat)).c_str(),
                               (hname + "_" + std::to_string(cat)).c_str());
  }
}

template <typename Cat, typename Val>
void queryosity::ROOT::hist_categorized<Cat, Val>::fill(column::observable<Cat> cat,
                                              column::observable<Val> x,
                                              double w) {
  m_hists[cat.value()]->Fill(x.value(), w);
}

template <typename Cat, typename Val>
std::map<Cat,std::shared_ptr<TH1>> queryosity::ROOT::hist_categorized<Cat, Val>::result() const {
  return m_hists;
}

template <typename Cat, typename Val>
std::map<Cat,std::shared_ptr<TH1>> queryosity::ROOT::hist_categorized<Cat, Val>::merge(
    std::vector<std::map<Cat,std::shared_ptr<TH1>>> const &results) const {
  std::map<Cat, std::shared_ptr<TH1>> merged_result;
  for (auto const& [cat, hist] : results[0]) {
    merged_result[cat] = std::shared_ptr<TH1>(static_cast<TH1*>(hist->Clone()));
    merged_result[cat]->SetDirectory(nullptr);
  }
  for (size_t islot = 1; islot < results.size(); ++islot) {
    for (auto const& [cat, hist] : results[islot]) {
      merged_result[cat]->Add(hist.get());
    }
  }
  return merged_result;
}
