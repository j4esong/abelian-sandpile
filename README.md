# abelian-sandpile
A 3D simulation of the sandpile group made with OpenGL.
## keyboard controls
### movement
<kbd>W</kbd>: move forward  
<kbd>A</kbd>: move left  
<kbd>S</kbd>: move backward  
<kbd>D</kbd>: move right  
<kbd>Left Ctrl</kbd>: move down  
<kbd>Spacebar</kbd>: move up

### other
<kbd>Q</kbd>: toggle cursor (for looking around/changing settings/maximizing)  
<kbd>P</kbd>: toggle pause  

## interface
The interface is built with ImGui, and is much less intrusive if the application is fullscreened.

#### `play/pause`
Plays/pauses the simulation. The simulation will pause itself if has reached the maximum number of drops. To continue, increase the number of drops or reset the sandpile in some way (`clear`, `randomize`).
#### `export data`
Exports the currently recorded data as two files, `asp_freqDist.txt` and `asp_simInfo.txt`. It can be plotted and exported to PDF using the included `plot.R`, assuming that R is already installed.
#### `display`
Can be disabled to allow for fast data collection. It can only be disabled if both the `infinite` checkbox is unchecked and if the simulation is paused. Once the simulation is unpaused, it will freeze the screen until the sandpile has finished processing the requested amount of drops.
#### `center`
Toggles whether the sand is dropped in the center or in a random cell.
#### `highlight`
Toggles whether or not to highlight the collapsing (*n* >= 4) cells.
#### `infinite`
Toggles whether or not the simulation should continue infinitely. If disabled, an additional field `drops` for the number of maximum drops appears.
#### `clear`, `randomize`
Clears or randomizes the sandpile, resetting the number of drops and recorded size data.
#### `frames`
Controls how many frames of animation are given to each update, where 1 means no animation.
#### `width`, `height`
Can be used to resize the sandpile plate. Note that resizing the plate will clear it first and also reset the drop count.

## notes
- the simulation is capped at slightly above 60 FPS to reduce CPU usage. However, if `display` is turned off, CPU usage will spike.
- shadow quality gets very bad if the dimensions are high, so height and width are capped at 100.
