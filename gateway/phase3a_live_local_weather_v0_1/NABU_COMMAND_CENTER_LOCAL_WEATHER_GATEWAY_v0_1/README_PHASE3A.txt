NABU COMMAND CENTER PHASE 3A-01A NIST TIME GATEWAY

Creator: Derek Leger
Copyright (c) 2026 Derek Leger. All rights reserved.
Product Version: UNASSIGNED
Build ID: NCC-GW-260810-P3A-01A

One controlled publication:
python nist_time_gateway.py --store-path "D:\NABU Internet Adapter\Store" --hostname time.nist.gov --server-ip 132.163.96.2 --timeout 5

The server IP shown here is the address returned for the generic NIST hostname
and proven responding during this transaction. Re-resolve time.nist.gov for a
future transaction rather than treating this address as permanent.

Tests:
python -m unittest discover -s tests -v

The publisher imports and reuses canonical_record() and atomic_write() from the
preserved gateway.py. It does not change the host clock or open a listener.
