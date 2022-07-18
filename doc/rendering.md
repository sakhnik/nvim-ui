# Rendering pipeline

Gtk suggests to use its stock widgets to display the information.
So the grid cells are processed, grouped to turn into Gtk labels in the screen.
CSS style and Pango layout are used to handle text highlighting.

* Neovim maintains and communicates the state of each grid cell to the UI.
  * `[["text": string, hl_id: int]]`
* When Flush is executed in the rendering thread:
  * The adjacent cells with the same hl_id are combined into chunks of homogenous highlighting.
    * `[[index: int]]`, see `_SplitChunks()`
  * The chunks are combined into a vector of "words":
    * `[[text: string, width: int, hl_id: int]]`
  * The execution is passed to the Gtk thread, see `Present()`
    * Pango markup is created from the "words"
    * Gtk label is create for every changed line and placed in the proper screen line
