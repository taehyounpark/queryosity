import numpy as np
from scipy.stats import cauchy, norm, poisson, randint
import matplotlib.pyplot as plt
import json

# Set seed for reproducibility
np.random.seed(42)

# Parameters
mass = 100.0
width = 0.5
offset = 1.1
resolution = 1.0

nentries = 10000

# Generate random numbers from the Cauchy distribution
cauchy_dist = cauchy(loc=mass, scale=width)
cauchy_samples = cauchy_dist.rvs(nentries)

# Gaussian random numbers for scaling
gaussian_dist = norm(loc=1.0, scale=1.0/125)
gaussian_samples = gaussian_dist.rvs(nentries)

# Perform the operations as described
x_true = cauchy_samples
x_nom = x_true * gaussian_samples
x_shift = x_nom + offset
x_smear = x_nom + norm(loc=0.0, scale=resolution).rvs(nentries)

# Poisson-distributed random numbers for weights
w_nom = np.ones(nentries)
w_toy = w_nom * poisson(1).rvs(nentries)

# category
category = randint.rvs(nentries)

# Prepare the data for JSON
data_to_save = [
    {
        "x_true": float(x_val), 
        "x_nom": float(x_nom_val), 
        "x_shift": float(x_shift_val), 
        "x_smear": float(x_smear_val),
        "w_nom": int(w_nom_val),
        "w_toy": int(w_toy_val),
        "category" : int(category)
    } 
    for x_val, x_nom_val, x_shift_val, x_smear_val, w_nom_val, w_toy_val in zip(x_true, x_nom, x_shift, x_smear, w_nom, w_toy, category)
]

# Save the data into a JSON file
with open('test_data.json', 'w') as f:
    json.dump(data_to_save, f, indent=4)
