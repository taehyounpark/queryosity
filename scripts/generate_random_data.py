import numpy as np
from scipy.stats import cauchy, norm, poisson
import matplotlib.pyplot as plt
import json

# Set seed for reproducibility
np.random.seed(42)

# Parameters
higgs_mass = 125.0
higgs_width = 0.004
shift_scale = 1.1
smear_reso = 1.0

nentries = 10000

# Generate random numbers from the Cauchy distribution
cauchy_dist = cauchy(loc=higgs_mass, scale=higgs_width)
cauchy_samples = cauchy_dist.rvs(nentries)

# Gaussian random numbers for scaling
gaussian_dist = norm(loc=1.0, scale=1.0/125)
gaussian_samples = gaussian_dist.rvs(nentries)

# Perform the operations as described
x_true = cauchy_samples
x_nom = x_true * gaussian_samples
x_scale = x_nom + 1.0
x_smear = x_nom + norm(loc=0.0, scale=smear_reso).rvs(nentries)

# Poisson-distributed random numbers for weights
w_nom = np.ones(nentries)
w_toy = w_nom * poisson(1).rvs(nentries)

# Prepare the data for JSON
data_to_save = [
    {
        "x_true": float(x_val), 
        "x_nom": float(x_nom_val), 
        "x_scale": float(x_scale_val), 
        "x_smear": float(x_smear_val),
        "w_nom": int(w_nom_val),
        "w_toy": int(w_toy_val)
    } 
    for x_val, x_nom_val, x_scale_val, x_smear_val, w_nom_val, w_toy_val in zip(x_true, x_nom, x_scale, x_smear, w_nom, w_toy)
]

# Save the data into a JSON file
with open('data.json', 'w') as f:
    json.dump(data_to_save, f, indent=4)
