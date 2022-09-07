# Soft Shadow Mapping Graphics Model

- Developed a graphics Model in C++ by that produces soft shadows by doing appropriate lighting calculations. 
- Soft shadows are the ones we see in real life where they have both umbra and penumbra regions because of area and volume light sources, whereas normal shadows only have the umbra regions assuming only point light sources exist. 
- The lighting calculations involved getting depth data and storing it in a ZTexture from the light source point of view, so that the umbra and penumbra regions can be differentiated. The coloring of penumbra region was done by non-linear alpha blending. 
- Used the Direct3D library for rendering the output.

Refer the [Report](https://github.com/Charan000/SoftShadowVolumes/blob/main/Soft%20Shadow%20Volumes.pdf) for additional details.

![alt text](https://github.com/Charan000/SoftShadowVolumes/blob/main/Shadows/images/test1.JPG?raw=true)
![alt text](https://github.com/Charan000/SoftShadowVolumes/blob/main/Shadows/images/test2.JPG?raw=true)
