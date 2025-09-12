import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
from scipy.optimize import minimize

#config
N_TERMS = 300  # change this as for your liking details
IMG_SIZE = 64

#load and prepare image
img = Image.open("me.jpeg").convert("L").resize((IMG_SIZE, IMG_SIZE))
data = np.array(img) / 255.0
h, w = data.shape
x = np.linspace(0, 1, w)
y = np.linspace(0, 1, h)
X, Y = np.meshgrid(x, y)

#smallparametriv or multi-term parametric function... multi- here
def model(params):
    f = np.zeros_like(X)
    for i in range(N_TERMS):
        A = params[i*4 + 0]
        kx = params[i*4 + 1]
        ky = params[i*4 + 2]
        phi = params[i*4 + 3]
        f += A * np.sin(2*np.pi*(kx*X + ky*Y) + phi)
    return f

#define loss function by mean squared error5
def loss(params):
    return np.mean((model(params) - data)**2)

#optimizing param
initial_guess = np.random.rand(N_TERMS * 4)  #random start
result = minimize(loss, initial_guess, method="Nelder-Mead", options={"maxiter": 5000})
best_params = result.x

#equation print
print("\nApproximation Equation:")
for i in range(N_TERMS):
    A, kx, ky, phi = best_params[i*4:(i+1)*4]
    print(f"Term {i+1}: {A:.3f} * sin(2π({kx:.3f}x + {ky:.3f}y) + {phi:.3f})")

#show reconstruct result
plt.subplot(1, 2, 1)
plt.title("Original")
plt.imshow(data, cmap="gray")
plt.axis("off")

plt.subplot(1, 2, 2)
plt.title(f"Approximation ({N_TERMS} terms)")
plt.imshow(model(best_params), cmap="gray")
plt.axis("off")
plt.show()
