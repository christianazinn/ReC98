# TH02 replay seek readiness

The public `T2RPY1` replay path starts from the recorded beginning only.
Stage-score rows and sequential stage-start control packets are metadata; they
do not carry a semantic restore state.

`T2CKP1` and `T2XCK1` are private checkpoint validation infrastructure. The
only apply transaction is enabled solely by the `T2REPLAY_EXACT_APPLY` private
build profile and is not a public replay contract. In particular, no OP menu
or release command may publish `T2XAP1.BIN` until fresh-process evidence
establishes the supported state class: first revealed page, both-page
convergence, callback/palette/HUD state, replay cursor, and forward gameplay
must match sequential playback.

Clean Practice constructors are not replay restore paths. They create legal
new states, whereas a replay seek must preserve the recorded live pools, RNG,
actor state, callbacks, page phase, and pending score state.

The full implementation plan and evidence inventory live in the harness
alongside the replay campaign. This source note exists to keep release-facing
code from treating diagnostic exact apply as a production feature.
