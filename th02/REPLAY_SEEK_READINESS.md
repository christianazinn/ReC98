# TH02 replay seek readiness

TH02 records into a `T2RPY3` temporary replay plus a `T2RSK2` temporary
sidecar. OP finalizes that pair as one self-contained `T2RPY4` file. The
136-byte replay header and packet stream are unchanged; a compact `T2ACC1`
tail stores selectable clean-stage starts after the input extent.

Each reached stage contributes a 24-byte directory entry and a 36-byte carried
start snapshot behind a 32-byte header. Entries bind the selected stage to the
replay's sample anchor, packet anchor, prefix checksum, finalized-header
checksum, and carried-start checksum. MAIN validates the entire embedded tail
before applying a selected start and entering the ordinary stage initializer.
OP independently validates it before exposing any later stage; every mismatch
must fail closed.

Pending replay and sidecar files form one save transaction. OP first builds a
fully checksummed `T2RPY4` candidate, then atomically promotes that candidate
and removes the temporary pair. Existing `T2RPY1` through `T2RPY3` files remain
sequentially playable; a historical numbered `T2RSK2` remains an optional
compatibility accelerator for `T2RPY3`.

The earlier `T2RSK1` / `T2RSQ1` Stage 5 Mima exact-envelope experiment remains
private validation infrastructure under `T2REPLAY_EXACT_APPLY`. It is not read
by the public menu and remains excluded from release media alongside `T2XAP1`,
`T2XOBQ`, and `T2XOBS`.
