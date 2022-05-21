#include "ana/vecutils.h"

#include <cmath>

std::vector<double> vec::make_lin(size_t binCount, double a, double b)
{
	double binWidth = (b - a) / binCount;
	std::vector<double> bins;
	for (int i=0 ; i<=binCount ; ++i) {
		bins.push_back(a+i*binWidth);
	}
	return bins;
}

std::vector<double> vec::make_log(size_t binCount, double a, double b)
{
	std::vector<double> bins; 
	double binWidth = (log(b)-log(a)) / binCount;
	for (int i=0 ; i<=binCount ; ++i) {
		bins.push_back(exp(log(a)+i*binWidth));
	}
	return bins;
}
