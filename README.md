
NOTICE: This engine was a youthful experiment and is marked with a degree of impulsivity and, as a result, technical debt that makes it... not much fun. It's also probably super-broken.

I recommend https://github.com/indigoparadox/dsekai instead.

=== Mode Plugins ===

=== Ideas ===

  * Implement "MUD/MUSE" mode for normal IRC clients:
    * Our client has to send special command to get into normal mode.
    * Normal clients get "/em approaches you" message when other mob gets close.
    * Maps have /look definition.
    * Curses compatibility.
  * Write tests for heatshrink.
    * Separate heatshrink out.
    * Convert to C89.
  * Cache counts for hashmap, ensure it's fully encapsulated first!
  * Weapons:
    * Item, on use, creates animation.
    * Item properties determine animation start, sprite, travel range.
    * Modes can have transformation function to modify animation.

