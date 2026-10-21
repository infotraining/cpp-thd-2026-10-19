import matplotlib.pyplot as plt
import numpy as np

variants = [
    "Single Thread",
    "Aligned Counter",
    "Many Threads",
    "Mutex",
    "Atomic",
    "Local Counter"
]

times_ms = np.array([
    2118.99,   # Single Thread
    413.781,   # Aligned Counter
    574.476,   # Many Threads
    9409.47,   # Mutex
    2193.59,   # Atomic
    276.979    # Local Counter
])

baseline = times_ms[0]
speedups = baseline / times_ms

plt.style.use("seaborn-v0_8")
fig, ax = plt.subplots(figsize=(10, 6))
bars = ax.bar(variants, speedups, color=plt.cm.tab10.colors)

for bar, value in zip(bars, speedups):
    ax.text(
        bar.get_x() + bar.get_width()/2,
        bar.get_height(),
        f"{value:.2f}×",
        ha="center",
        va="bottom",
        fontsize=10
    )

ax.set_title("Monte Carlo Pi Speedup Relative to Single Thread", fontsize=14)
ax.set_ylabel("Speedup (×)")
plt.xticks(rotation=30, ha="right")
plt.tight_layout()
plt.show()
