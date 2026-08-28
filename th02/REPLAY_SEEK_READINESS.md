# TH02 replay seek readiness

The public `T2RPY1` and `T2RPY2` replay paths start from the recorded
beginning only. `T2RPY2` adds run-wide slowdown telemetry; neither public
carrier adds a checkpoint directory, authenticated restore payload, or a
later-start command. Stage-score rows and sequential stage-start control
packets are metadata; they do not carry a semantic restore state.

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

The only current exact candidate is the private schema-6 Stage 5 Mima
envelope. It stays private and release-disabled until a separately held,
finalized replay and matching capture pass the fresh-process matrix: the
anchor and next-sample cursors, first revealed page, both-page convergence,
HUD/TRAM, palette, callback registry, logical page/scroll state, semantic
digest, and natural terminal must agree with uninterrupted playback. The
harness gate is `tools/replay/verify_th02_selectable_later_stage_start.ps1`;
its default static result is deliberately not promotion evidence.
