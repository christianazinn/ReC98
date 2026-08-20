; A random shared point variable used for drawing various things.
; 
; TH04's player_invalidate() proves that ZUN's source had the two halves as
; SEPARATE 16-bit variables at least at one call site, not as members of one
; struct: it passes them to tiles_invalidate_around() as TWO word pushes,
; and `-3` folds two adjacent `pascal` arguments into one 32-bit push
; whenever it can prove they are contiguous -- which it can for two members
; of one object and cannot for two distinct globals. Measured with `tcc -S`
; (kb/codegen/0152) over five source spellings: only the two-globals one
; emits the pair. So the two halves also get zero-byte names of their own,
; for the callers that need them (kb/codegen/0123).
public _drawpoint
public _drawpoint_x, _drawpoint_y
_drawpoint	label Point
_drawpoint_x	dw ?
_drawpoint_y	dw ?
