# TH02 replay seek readiness

TH02's public replay remains `T2RPY3`. Selectable clean-stage starts are stored
in an additive `T2RSK2` sidecar, not in the replay header or packet stream.
The original starting stage is always playable without a sidecar.

Each reached stage contributes a 48-byte directory entry and a 36-byte carried
start snapshot behind a 64-byte header. Entries bind the selected stage to the
replay's sample anchor, packet anchor, prefix checksum, finalized header and
payload checksums, stream totals, and versioned format fingerprint. MAIN
validates these fields before applying the carried state and entering the
ordinary stage initializer. OP validates the complete sidecar before exposing
any later stage; malformed, stale, missing, or swapped sidecars fail closed.

Pending replay and sidecar files form one save transaction. Slot overwrite
backs up, rebinds, renames, and rolls back both files together. Legacy `T2RPY3`
files therefore keep original-start playback but do not advertise unavailable
later starts.

The earlier `T2RSK1` / `T2RSQ1` Stage 5 Mima exact-envelope experiment remains
private validation infrastructure under `T2REPLAY_EXACT_APPLY`. It is not read
by the public menu and remains excluded from release media alongside `T2XAP1`,
`T2XOBQ`, and `T2XOBS`.
