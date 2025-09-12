import numpy as np
import matplotlib.pyplot as plt
from PIL import Image

#prep and load image
img = Image.open("my_image.jpg").convert("L") # put your iamge int he same folder as this file and then paste yo image name over here. idk how to create a image input option 
img = img.resize((64, 64))
data = np.array(img) / 255.0
h, w = data.shape

#Fourier Trnsforamtion
fft_result = np.fft.fft2(data)
fft_shifted = np.fft.fftshift(fft_result)

# finding the strongest term
flat_indices = np.argsort(np.abs(fft_shifted.flatten()))[::-1] ## sort by magntinude
top_n = 20  #choose top N terms
indices = np.unravel_index(flat_indices[:top_n], fft_shifted.shape)

terms = []
for i in range(top_n):
    u, v = indices[0][i], indices[1][i]
    coeff = fft_shifted[u, v]
    amp = np.abs(coeff)
    phase = np.angle(coeff)
    freq_x = u - h//2
    freq_y = v - w//2
    terms.append((freq_x, freq_y, amp, phase))

#print equation ---
print("Approximation Equation:")
print("f(x, y) ≈ ", end="")
eq_parts = []
for fx, fy, amp, phase in terms:
    part = f"{amp:.3f} * cos(2π({fx}x + {fy}y) + {phase:.3f})"
    eq_parts.append(part)
print(" + ".join(eq_parts))

#reconstruct image using topterms
x = np.linspace(0, 1, w)
y = np.linspace(0, 1, h)
X, Y = np.meshgrid(x, y)
reconstructed = np.zeros_like(X)

for fx, fy, amp, phase in terms:
    reconstructed += amp * np.cos(2*np.pi*(fx*X + fy*Y) + phase)
#normalize result back to [0,1]
reconstructed = (reconstructed - reconstructed.min()) / (reconstructed.max() - reconstructed.min())

#show orignal and the reconstructed one 
plt.subplot(1, 2, 1)
plt.title("Original")
plt.imshow(data, cmap="gray")
plt.axis("off")

plt.subplot(1, 2, 2)
plt.title(f"Reconstructed (Top {top_n} Terms)")
plt.imshow(reconstructed, cmap="gray")
plt.axis("off")
plt.show()
