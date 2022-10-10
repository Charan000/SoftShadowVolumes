# Soft Shadow Mapping Graphics Model

- Developed a graphics Model in C++ by that produces real time soft shadows by doing appropriate lighting calculations, even with multiple light sources, and the technique is based on the following research : [Forest, Vincent & Barthe, Loïc & Paulin, Mathias. (2006). Realistic soft shadows by penumbra-wedges blending. In Graph. Hardw. 39-46. 10.1145/1283900.1283907.](https://www.researchgate.net/publication/234802739_Realistic_soft_shadows_by_penumbra-wedges_blending) 
- Soft shadows are the ones we see in real life where they have both umbra (sharp, homogeneous and dark) and penumbra (blurred parts around the edges where some amount of light enters) regions because of area and volume light sources, whereas normal shadows in the context of computer graphics only have the umbra regions assuming only point light sources exist. 
- The lighting calculations involved getting depth data and storing it in a ZTexture from the light source point of view, so that the umbra and penumbra regions can be differentiated. The coloring of penumbra region was done by non-linear alpha blending. 
- Used the Direct3D, techniques of Shader programming for rendering the output.

Refer the [Report](https://github.com/Charan000/SoftShadowVolumes/blob/main/Soft%20Shadow%20Volumes.pdf) for additional details.

<img src="https://github.com/Charan000/SoftShadowVolumes/blob/main/Shadows/images/test1.JPG" align="left" alt="Your image title" width="300"/>
<img src="https://github.com/Charan000/SoftShadowVolumes/blob/main/Shadows/images/test2.JPG" align="left" alt="Your image title" width="300"/>
