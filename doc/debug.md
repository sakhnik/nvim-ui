# Debugging

## Text layout on the grid

* Obtain strings using the inspector: Help -> Inspector, pick the necessary label on the grid
* Collect the strings into a file /tmp/pango.txt
* Remove the background attributes: s/background="#......"//g
* Use pango-view to see the rendered glyphs:
  * `pango-view --markup --font="Fira Code" --annotate=glyph --dpi=300 /tmp/pango.txt`
