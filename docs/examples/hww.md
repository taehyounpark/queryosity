# Higgs boson analysis

## Overview

This demo analyzes physics collision in simulated \(H\rightarrow WW^{\ast}\rightarrow \ell\nu\ell\nu\) events.

- The public dataset is available at [CERN Open Data Portal](https://opendata.cern.ch/record/700).
- The implementation of [CERN ROOT Framework](https://root.cern) for queryosity is provided by [AnalysisPlugins](https://github.com/taehyounpark/AnalysisPlugins).

The following tasks will be performed:

## Nominal

```cpp
dataflow df;

auto tree_files = std::vector<std::string>{"hww.root"};
auto tree_name = "mini";
auto ds = df.load(dataset::input(tree_files, tree_name));

auto [mc_weight, el_sf, mu_sf] = ds.read(
  dataset::column<float>("mcWeight"),
  dataset::column<float>("scaleFactor_ELE"),
  dataset::column<float>("scaleFactor_MUON")
  );

auto [
  lep_pt_MeV,
  lep_eta,
  lep_phi,
  lep_E_MeV,
  lep_Q,
  lep_type
  ] = ds.read(
    dataset::column<VecF>("lep_pt"),
    dataset::column<VecF>("lep_eta"),
    dataset::column<VecF>("lep_phi"),
    dataset::column<VecF>("lep_E"),
    dataset::column<VecF>("lep_E"),
    dataset::column<VecF>("lep_charge"),
    dataset::column<VecF>("lep_type")
    );

auto [met_MeV, met_phi] = ds.read(
  dataset::column<float>("met_et"),
  dataset::column<float>("met_phi")
  );

auto MeV = ana.constant(1000.0);
auto lep_pt = lep_pt_MeV / MeV;
auto lep_E = lep_pt_MeV / MeV;
auto met = met_MeV / MeV;

auto lep_eta_max = df.constant(2.4);
auto lep_pt_sel = lep_pt[ lep_eta < lep_eta_max && lep_eta > (-lep_eta_max) ];

auto p4l1 = df.define<NthP4>(0)(lep_pt_sel, lep_eta_sel, lep_phi_sel, lep_E_sel);
auto p4l2 = df.define<NthP4>(1)(lep_pt_sel, lep_eta_sel, lep_phi_sel, lep_E_sel);

auto p4ll = p4l1+p4l2;

using P4 = TLorentzVector;
auto mll = df.define([](const P4& p4){return p4.M();})(p4ll);

auto pth = df.define(
  [](const P4& p4, float q, float q_phi) {
    TVector2 p2; p2.SetMagPhi(p4.Pt(), p4.Phi());
    TVector2 q2; q2.SetMagPhi(q, q_phi);
    return (p2+q2).Mod();
  })(p4ll, met, met_phi);

auto n_lep_sel = df.define([](VecF const& lep){return lep.size();},lep_pt_sel);
auto n_lep_req = df.define(column::constant<unsigned int>(2));

auto cut_2l = df.weight(mc_weight * el_sf * mu_sf)\
                .filter(n_lep_sel == n_lep_req);

auto cut_2los = cut_2l.filter([](const VecI& lep_charge){return lep_charge.book(0)+lep_charge.book(1)==0;},lep_Q);
auto cut_2ldf = cut_2los.filter([](const VecUI& lep_type){return lep_type.book(0)+lep_type.book(1)==24;},lep_type);
auto cut_2lsf = cut_2los.filter([](const VecUI& lep_type){return (lep_type.book(0)+lep_type.book(1)==22)||(lep_type.book(0)+lep_type.book(1)==26);},lep_type);

auto mll_cut = df.constant(60.0);
auto cut_2ldf_sr = cut_2ldf.filter(mll < mll_cut);
auto cut_2lsf_sr = cut_2lsf.filter(mll < mll_cut);
auto cut_2ldf_wwcr = cut_2ldf.filter(mll > mll_cut);
auto cut_2lsf_wwcr = cut_2lsf.filter(mll > mll_cut);

auto pth_hist = df.get<Hist<1,float>>("pth",100,0,400).fill(pth).book(cut_2los);

auto l1n2_pt_hists_wwcrs = l1n2_pt_hist.book(cut_2lsf_sr, cut_2lsf_wwcr);

auto Escale = df.define([](VecD E){return E;}).vary("lp4_up",[](VecD E){return E*1.02;}).vary("lp4_dn",[](VecD E){return E*0.98;});
auto lep_pt_sel = Escale(lep_pt)[ lep_eta < lep_eta_max && lep_eta > (-lep_eta_max) ];
auto lep_E_sel = Escale(lep_E)[ lep_eta < lep_eta_max && lep_eta > (-lep_eta_max) ];

auto p4l1 = df.define<NthP4>(0)(lep_pt, lep_eta, lep_phi, lep_E);
auto p4l2 = df.define<NthP4>(1)(lep_pt, lep_eta, lep_phi, lep_E);

auto cut_2l = df.weight("weight")(mc_weight * el_sf * mu_sf)\
                .filter("2l")(n_lep_sel == n_lep_req);

auto mll_vars = df.get<Hist<1,float>>("mll",50,0,100).fill(mll).book(cut_2los);

mll_vars.nominal()->Draw();
mll_vars["lp4_up"]->Draw("same");
```
![mll_varied](../../images/mll_varied.png)

