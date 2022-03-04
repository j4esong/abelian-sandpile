# abelian-sandpile
A 3D simulation of the sandpile group made using OpenGL.
## controls
### movement
<kbd>W</kbd>: move forward  
<kbd>A</kbd>: move left  
<kbd>S</kbd>: move backward  
<kbd>D</kbd>: move right  
<kbd>Left Ctrl</kbd>: move down  
<kbd>Spacebar</kbd>: move up

### other
<kbd>Left Shift</kbd>: toggle cursor (for looking around/changing settings/maximizing)  
<kbd>P</kbd>: toggle pause  

## notes
- the simulation is capped at slightly above 60 FPS
- shadow quality gets very bad if the dimensions are high, so height and width are capped at 100
- resizing the plate will clear it and reset the drop count/other data
- \"frames\" refers to how many frames of animation are given to each update, where 1 means no animation
