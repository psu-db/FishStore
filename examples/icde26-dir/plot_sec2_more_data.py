import matplotlib.pyplot as plt
import numpy as np

# Data (raw is all 1's)
raw = np.ones(16)
traditional = np.array([
    0.043198256, 0.021802008, 0.014671977, 0.010730825, 0.008750141,
    0.007304578, 0.006329252, 0.005446992, 0.004872376, 0.00441588,
    0.996853612, 0.988708103, 1.004777007, 0.993973345, 1.003168212, 0.999697218
])
stitching = np.array([
    0.045161583, 0.024074764, 0.01689469, 0.013153249, 0.010744954,
    0.009204563, 0.008614231, 0.007398144, 0.006833174, 0.006358033,
    0.029827417, 0.116685909, 0.188777539, 0.245884727, 0.296070268, 0.338896119
])

# Compute speedup (1 / traditional and 1 / stitching)
speedup_traditional =  traditional
speedup_stitching =  stitching

# X-axis values (hours)
x = np.arange(1, len(raw) + 1)

# Plot speedup
plt.figure(figsize=(10, 6))
plt.plot(x, speedup_traditional, label='Traditional Speedup', marker='o', linewidth=2.5)
plt.plot(x, speedup_stitching, label='Stitching Speedup', marker='s', linewidth=2.5)
plt.plot(x, raw, label='Raw Baseline (Speedup=1)', linestyle='--', color='gray', linewidth=2)


plt.xlabel('Hour of Data Included')
plt.ylabel('Speedup (Raw Time / Method Time)')
plt.title('Speedup of Traditional and Stitching Methods vs. Raw Baseline')



plt.legend()


plt.show()