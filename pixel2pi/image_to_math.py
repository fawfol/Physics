import numpy as np
import matplotlib.pyplot as plt
from PIL import Image


# idk where the heck its used but i saw someone creating images with math equation and function so i had the ide to do the oppostie and searched about it a bit and got to know about fourier tranformation
# load and prepere prepare image ---
img = Image.open("me.jpeg").convert("L") # put your image in the same folder and put the name instead of me.jpeg 
img = img.resize((128, 128))  # keep small for speed
data = np.array(img) / 255.0  # normalize

#compute 2D fourier transform ---
fft_result = np.fft.fft2(data)
fft_shifted = np.fft.fftshift(fft_result)

# keep only he largest frequencies as apporximation
threshold = np.percentile(np.abs(fft_shifted), 99)  # keep top 1%
fft_filtered = np.where(np.abs(fft_shifted) > threshold, fft_shifted, 0)

# reconstruct image ---
reconstructed = np.abs(np.fft.ifft2(np.fft.ifftshift(fft_filtered)))

#show result
plt.subplot(1, 2, 1)
plt.title("Original")
plt.imshow(data, cmap="gray")
plt.axis("off")

plt.subplot(1, 2, 2)
plt.title("Math Approximation")
plt.imshow(reconstructed, cmap="gray")
plt.axis("off")
plt.show()
