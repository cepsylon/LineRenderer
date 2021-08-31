Controls:
 - Space -> Toggle between SDF line rendering and single pixel width line rendering
 - Ctrl -> Toggle horizontal line rendering
 - Shift -> Toggle vertical line rendering
 - Left click -> One click to set start of line. Second click ends line
 - Mouse wheel -> Increase and decrease line width when rendering with SDF

Implementation details:
 Render flow: We keep an image of already rendered lines so we don't need to render
 them every loop. This way we only need to copy this image to the back buffer.
 When we start rendering a line, we first render the copy to the back buffer, and
 then, the new line is rendered on top. If we finish rendering the line, it will
 be rendered directly to the cached image with all the other lines. Allows for fast
 execution at the cost of some memory.

Improvements:
 At the moment, the SDF line renderer is thought to work with a set size (1024x1024).
 Adding flexibility should not be any issue, but we would have to keep in mind that at
 the moment SDF line rendering is somewhat dependant on having square resolution. Even
 if it works for non-squared resolutions, lines will not keep the right width.

 Performance optimisation: we can set scissor test to clip SDF rendering instead of
 rendering the whole texture.

 It could be good to allow for letting the user choose how sharp they want the lines
 to be. At the moment, there's just a set fade for the edges of the lines.