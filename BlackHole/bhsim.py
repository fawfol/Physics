import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

#Setup the 3D Plot
fig = plt.figure(figsize=(12, 12))
ax = fig.add_subplot(111, projection='3d')
plt.style.use('dark_background')

#Create the Curved Spacetime tapestry

#define the mass of the black hole
M = 1.5

#create a grid of X and Y points. Low resolution (50x50) for performance
grid_points = 50
x = np.linspace(-10, 10, grid_points)
y = np.linspace(-10, 10, grid_points)
X, Y = np.meshgrid(x, y)

# calculate the distance from the center (location of the black hole)
R = np.sqrt(X**2 + Y**2)

#Pevent division by zero at the very center
R[R == 0] = 1e-6

#Calculate the Z coordinate to create the "gravity well" shape
#This is a simplified gravitational potential formula
Z = -M / R

#Plot the surface of spacetime
#"rstride" and 'cstride" control how many lines are in the wirefram
# A colormap like 'viridis' or 'plasma' looks nice
ax.plot_surface(X, Y, Z, cmap='plasma', rstride=1, cstride=1, alpha=0.6, linewidth=0.1)

#Create the Black Hole (Event Horizon)

#Define the size of the event horizon
schwarzschild_radius = 1.0

#create points for a sphere
phi = np.linspace(0, 2*np.pi, 10)
theta = np.linspace(0, np.pi, 10)
x_bh = schwarzschild_radius * np.outer(np.cos(phi), np.sin(theta))
y_bh = schwarzschild_radius * np.outer(np.sin(phi), np.sin(theta))
z_bh = schwarzschild_radius * np.outer(np.ones(np.size(phi)), np.cos(theta))

#the black hole sits at the bottom of the well. We need to find the Z value there.
center_z = -M / schwarzschild_radius
ax.plot_surface(x_bh, y_bh, z_bh + center_z, color='black')

#simulate and plot Light Rays

def plot_light_ray(start_y, num_steps=200):
    """Calculates and plots a single light ray's path."""
    #start the ray far away on the left
    path = np.zeros((num_steps, 3))
    path[0] = [-10, start_y, 0] #start at x=-10, y=start_y, z=0 (on the plane)
    
    #initial velocity is straight to the right
    velocity = np.array([0.1, 0, 0])
    
    for i in range(num_steps - 1):
        #Current position and distance to center
        pos = path[i, 0:2]
        dist_to_center = np.linalg.norm(pos)
        
        #simple gravity approximation: pull is stronger when closer
        # We add a small number to avoid extreme forces near the center
        force_magnitude = M / (dist_to_center**2 + 0.5)
        
        #direction of pull is towards the origin (0,0)
        accel = -pos / dist_to_center * force_magnitude
        
        #update velocity and position
        velocity[0:2] += accel
        #slice the velocity array
        path[i+1, 0:2] = path[i, 0:2] + velocity[0:2]
        
        #get Z value from our spacetime grid so the ray sits on it
        r_path = np.linalg.norm(path[i+1, 0:2])
        path[i+1, 2] = -M / r_path if r_path > 0 else 0
        
    #the final path
    ax.plot(path[:,0], path[:,1], path[:,2], color='cyan', lw=1.5)

#plot several light rays at different starting heights
plot_light_ray(start_y=0.8)   #strongly bent
plot_light_ray(start_y=1.5)   #moderately bent
plot_light_ray(start_y=5)     #barely affected
plot_light_ray(start_y=-2.5) 

# Set a nice viewing angle
ax.view_init(elev=60, azim=45)

#to make it cleaner we can hide the axis panes and labels
ax.set_axis_off()

plt.title("Simplified 3D Visualization of Spacetime Curvature")
plt.show()
