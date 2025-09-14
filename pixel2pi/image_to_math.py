import numpy as np
import matplotlib.pyplot as plt
from PIL import Image

#prep and load image
#image in smae folder as this code
img = Image.open("me.jpeg").convert("L")  #and image name as per
img = img.resize((64, 64))
data = np.array(img) / 255.0
h, w = data.shape

#fourier Transformation
fft_result = np.fft.fft2(data)
fft_shifted = np.fft.fftshift(fft_result)

#finding the strongest terms
flat_indices = np.argsort(np.abs(fft_shifted.flatten()))[::-1] # Sort by magnitude
top_n = 599  #choose top N terms incerease or dec it and for more or less detail
indices = np.unravel_index(flat_indices[:top_n], fft_shifted.shape)

terms = []
for i in range(top_n):
    u, v = indices[0][i], indices[1][i]
    coeff = fft_shifted[u, v]
    amp = np.abs(coeff) / (h * w) #normalize amplitude
    phase = np.angle(coeff)
    
    #horizontal frequency (fx) corresponding to the column index (v)
    #vertical frequency (fy) corresponding to the row index (u)
    freq_x = v - w//2
    freq_y = u - h//2
    terms.append((freq_x, freq_y, amp, phase))

#reconstruct image using top terms
x = np.linspace(0, 1, w, endpoint=False)
y = np.linspace(0, 1, h, endpoint=False)
X, Y = np.meshgrid(x, y)
reconstructed = np.zeros_like(X)

#the DC component (average brightness) should be treated separately
dc_amp = np.abs(fft_shifted[h//2, w//2]) / (h * w)
reconstructed += dc_amp

for fx, fy, amp, phase in terms:
    if fx == 0 and fy == 0:
        continue
    reconstructed += amp * np.cos(2*np.pi*(fx*X + fy*Y) + phase)

#normalize result back to [0,1] for display
reconstructed = (reconstructed - reconstructed.min()) / (reconstructed.max() - reconstructed.min())


#show original and the reconstructed one 
plt.figure(figsize=(10, 5))
plt.subplot(1, 2, 1)
plt.title("Original")
plt.imshow(data, cmap="gray")
plt.axis("off")

plt.subplot(1, 2, 2)
plt.title(f"Reconstructed (Top {top_n} Terms)")
plt.imshow(reconstructed, cmap="gray")
plt.axis("off")
plt.tight_layout()
plt.show()
