# Module Activation Contract

Frozen architecture:

`ONE COMMAND CENTER GATEWAY -> ONE GOLDEN TRANSPORT OWNER -> ONE SERIALIZED RESOURCE SCHEDULER -> MODULE DESCRIPTORS -> CACHED MODULE STATE -> ONE REUSABLE DASHBOARD RENDERER`

No module may create another HCCA/IM2 owner, transport path, gateway, or
unbounded buffer. Activation requires a fixed descriptor with: module ID,
dashboard page, tile position, display label, bounded resource name, refresh
policy, cached state, parser, summary renderer, optional detail handler, and
stale/error state.

The gateway provider entry must define: provider ID, source registry entry,
refresh interval, timeout/failure behavior, cache behavior, source timestamp,
bounded record format, atomic publication, and health state.

Activation sequence: define record and evidence source; add provider tests;
add bounded parser and cache without changing transport ownership; bind summary
and optional detail rendering; run isolated malformed/stale tests; run mixed
TIME/WEATHER regression; obtain Owner MAME acceptance; freeze separately.

TIME and WEATHER are the verified reference implementations. Future modules
remain inactive until explicitly selected by Derek Leger.

