import numpy as np
import matplotlib.pyplot as plt
from PIL import Image

######################INSTRUCTION#################################
# PUT YOUR IMAGE IN SAME FOLDER AS THIS CODE FILE               ##
# AND CHANGE THE INMAGE NAME AT HERE : 				            ##
#								                                ##
img_original = Image.open("me.jpeg")	#prep and load img	    ##
#								                                ##
#								                                ##
#SET THE IMAGE RESOLUTION FOR ANALYSIS                      	##
# Smaller values are obviously faster...                	    ##
# Larger values(128, 256) have more detail but slower no shit	##
image_resolution = 128                                        	##
##################################################################



#---------------------------------------------------------------##

##################################################################
#SWITCH FOR WITH OR WITHOUT COLOUR OPTION         		        ##
# Set to False to run the grayscale version (lightweight)   	##
# Set to True to run the new color version                  	##
process_in_color = True                                     	##
##################################################################


##################################################################
#SET VALUE OF TOP N                           			        ##
#500 seems to be the threshold for somewhat good detail     	##
top_n = 3000 # you can set how many frequency terms to use   	##
##################################################################


##################################################################
#SWITCH TO PRINT THE MATH EQUATION (can be very long!)     	##
# Set to True to print the equation to the terminal.         	##
# Best used with a very small top_n (e.g., 5).               	##
print_equation = False                                       	##
##################################################################


##################################################################
#SET THE IMAGE RESOLUTION FOR ANALYSIS                      	##
# Smaller values are obviously faster...                	    ##
# Larger values(128, 256) have more detail but slower no shit	##
image_resolution = 128                                        	##
##################################################################

###############  BY THE WAY  #####################################
#Avg phones have modest of :					                ##
#				(128 as image_resolution)	                    ##
#				(4000 for top N value)		                    ##
##################################################################


#---------------------------------------------------------------##


# greyscale path
if not process_in_color:
    img = img_original.convert("L")
    img = img.resize((image_resolution, image_resolution))
    data = np.array(img) / 255.0
    h, w = data.shape

    #fourier transformation
    fft_result = np.fft.fft2(data)
    fft_shifted = np.fft.fftshift(fft_result)

    #finding strongest terms
    flat_indices = np.argsort(np.abs(fft_shifted.flatten()))[::-1]
    indices = np.unravel_index(flat_indices[:top_n], fft_shifted.shape)
    
    terms = []
    for i in range(top_n):
        u, v = indices[0][i], indices[1][i]
        coeff = fft_shifted[u, v]
        amp = np.abs(coeff) / (h * w)
        phase = np.angle(coeff)
        freq_x = v - w//2
        freq_y = u - h//2
        terms.append((freq_x, freq_y, amp, phase))

    #print equation for greyscale if true
    if print_equation:
        print("Approximation Equation for Greyscale Image:")
        print("f(x, y) ≈ ", end="")
        eq_parts = []
        for fx, fy, amp, phase in terms:
            part = f"{amp:.3f} * cos(2π({fx}x + {fy}y) + {phase:.3f})"
            eq_parts.append(part)
        print(" + ".join(eq_parts))
        print("\n" + "="*50 + "\n")
    #

    #reconstruction of image for idk
    x = np.linspace(0, 1, w, endpoint=False)
    y = np.linspace(0, 1, h, endpoint=False)
    X, Y = np.meshgrid(x, y)
    reconstructed = np.zeros_like(X)
    reconstructed += np.abs(fft_shifted[h//2, w//2]) / (h*w) #DC component

    for fx, fy, amp, phase in terms:
        if fx == 0 and fy == 0: continue
        reconstructed += amp * np.cos(2*np.pi*(fx*X + fy*Y) + phase)
    
    #for dispaly, normalization
    reconstructed = (reconstructed - reconstructed.min()) / (reconstructed.max() - reconstructed.min())
    display_image = reconstructed
    cmap_val = "gray"
    original_display = data

# coloured path
else:
    img = img_original.resize((image_resolution, image_resolution))
    data = np.array(img) / 255.0
    h, w, _ = data.shape
    reconstructed_color = np.zeros_like(data)
    
    #process each channel(R, G, B) separately
    for i in range(3):
        channel_data = data[:, :, i]
        
        # fourier transformation for coloured path
        fft_result = np.fft.fft2(channel_data)
        fft_shifted = np.fft.fftshift(fft_result)

        #find strongest terms for the channel
        flat_indices = np.argsort(np.abs(fft_shifted.flatten()))[::-1]
        indices = np.unravel_index(flat_indices[:top_n], fft_shifted.shape)

        terms = []
        for j in range(top_n):
            u, v = indices[0][j], indices[1][j]
            coeff = fft_shifted[u, v]
            amp = np.abs(coeff) / (h * w)
            phase = np.angle(coeff)
            freq_x = v - w//2
            freq_y = u - h//2
            terms.append((freq_x, freq_y, amp, phase))

        #print the equation for the current channel if true
        if print_equation:
            channel_name = ["Red", "Green", "Blue"][i]
            print(f"\nApproximation Equation for {channel_name} Channel:")
            print("f(x, y) ≈ ", end="")
            eq_parts = []
            for fx, fy, amp, phase in terms:
                part = f"{amp:.3f} * cos(2π({fx}x + {fy}y) + {phase:.3f})"
                eq_parts.append(part)
            print(" + ".join(eq_parts))
            print("\n" + "="*50 + "\n")
        #

        #reconstruct channel
        x = np.linspace(0, 1, w, endpoint=False)
        y = np.linspace(0, 1, h, endpoint=False)
        X, Y = np.meshgrid(x, y)
        reconstructed_channel = np.zeros_like(X)
        reconstructed_channel += np.abs(fft_shifted[h//2, w//2]) / (h*w) # DCcomp

        for fx, fy, amp, phase in terms:
            if fx == 0 and fy == 0: continue
            reconstructed_channel += amp * np.cos(2*np.pi*(fx*X + fy*Y) + phase)
        
        #ad the reconstructed channel to our final color image
        reconstructed_color[:, :, i] = reconstructed_channel

    #normalize the final color image for display
    reconstructed_color = (reconstructed_color - reconstructed_color.min()) / (reconstructed_color.max() - reconstructed_color.min())
    display_image = reconstructed_color
    cmap_val = None #colour aint needed 
    original_display = data



# RESULT DISPLAY
plt.figure(figsize=(10, 5))
plt.subplot(1, 2, 1)
plt.title("Original")
plt.imshow(original_display)
plt.axis("off")

plt.subplot(1, 2, 2)
plt.title(f"Reconstructed (Top {top_n} Terms)")
plt.imshow(display_image, cmap=cmap_val)
plt.axis("off")
plt.tight_layout()
plt.show()
