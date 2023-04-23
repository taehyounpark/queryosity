# Features
A coherent columnar data analysis interface in C++.
- Implicit parallel processing of datasets.
- Fast and intuitive computation of column values.
- Systematic application of selections and/or statistical weights to entries.
- Customizable, lazily-executed output of analysis results.

## Prerequisites
- C++17 standard compiler
- CMake 3.12 or newer

The following example uses the [RAnalysis](https://github.com/taehyounpark/RAnalysis), an implementation for the [CERN ROOT library](https://root.cern/)

## 0. Open a dataset

Support for any dataset format, as long as the information can be per-row, columnar-valued representation.
```
  ana::multithread::enable();
  auto data = ana::analysis<CustomDataset>();
  data.open(...);
```

## 1. Compute quantities of interest 
Existing columns can be accessed by:
```
  // supply the data type and column name in order to read their  values
  auto x = data.read<int>("x");

  // columns can be of arbitrarily complex and/or nested types
  auto vs = data.read<std::vector<std::string>>("vs");
```
Computing new columns can be as straightforward as the expressions themselves:
```
  auto x = data.define<double>();
```
Or, as elaborate and extensible as it needs to be:
```
class CustomVariable : public ana::column::definition<>
{

};

// ...

  auto 
```

## 2. Filter entries
Selection of entries based on the values of columns can be made by:
```
  // trivial cut that passes all entries
  auto allCombinationsCut = data.cut("allCombinations", [](){return true;});

  // subsequent cuts automatically append to latest
  auto sameFlavourCut = data.cut("sameFlavour", [](){} );

  // cuts can be "re-based" on any prior-defind ones
  auto oppositeFlavoursCut = allCombinationsCut.cut("oppositeFlavours", [](){}, );
```
(The same logic can be used to apply statistical weights, see `ana::weight`).

## 3. Count results
Count the selected entries and output the results in any desired format:
```
  // create a custom counter
  auto dileptonMassSpectrum = data.count<Histogram<1,float>>("dileptonMassSpectrum", 100, 0.0, 200.0);

  // counter can be "filled" with column data
  dileptonMassSpectrum.fill(dileptonMass);

  // its action must be booked at one or more filters
  sameFlavourCut.book(dileptonMassSpectrum);

  // accessing the result triggers execution of all steps defined in 1, 2, and 3.
  auto dileptonMassHistogram = dileptonMassSpectrum["sameFlavour"].result();
  dileptonMassHistogram.Draw();
```

# Benchmarks

| Command | uproot+pandas | ana+RAnalysis
| --- | --- |
| filter + count | List all new or modified files |
| sum | Show file differences that haven't been staged |
