# Third-party notices

The NABU Command Center project is licensed under the MIT License in [`LICENSE`](LICENSE). That license applies only to material for which Derek Leger holds the applicable rights. It does not relicense third-party software, reference snapshots, services, data, music, documentation, trademarks, or tools.

## NABU-LIB reference

NABU-LIB is credited to DJ Sures and the [DJSures/NABU-LIB](https://github.com/DJSures/NABU-LIB) project. The private engineering repository documented reference revision `c9cfc6b93290ca77cfa223367810729863cf3c9e`. Its upstream README states that it has no conventional license and asks users to provide credit.

NABU-LIB source is therefore **not redistributed** in this publication repository. It is not covered or relicensed by the NABU Command Center MIT License. See [`NABU_LIB_REFERENCE.md`](NABU_LIB_REFERENCE.md) for the preserved attribution and provenance record.

The production v1.0 client uses the separately installed z88dk `+nabu` runtime and headers.

## Mido 1.3.3

The production Gateway declares Mido 1.3.3 as a Python runtime dependency. Mido is distributed under the MIT License.

Copyright (c) Ole Martin Bjørndalen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Source: <https://github.com/mido/mido>

## Mutopia music source

The v1.0 Music Gateway identifies its selected work as:

- J. S. Bach, *Aria Bist Du Bei Mir*
- Mutopia identifier `Mutopia-2007/07/08-257`
- source metadata classification: Public Domain
- source page: <https://www.mutopiaproject.org/cgibin/piece-info.cgi?id=257>

The Gateway records source and converted-file provenance in its cache metadata. Users should preserve that metadata and recheck the source page if distributing the source MIDI independently.

## z88dk

z88dk is an external build toolchain and is not copied into this repository. The verified local installation contains its own `LICENSE`, which identifies the default project license as the Clarified Artistic License unless an individual file states otherwise. Any redistribution of z88dk or its components must follow those upstream terms.

Source: <https://github.com/z88dk/z88dk>

## External services and data

The Gateway accesses third-party services including NIST, the U.S. National Weather Service, U.S. Geological Survey, NOAA Space Weather Prediction Center, Where The ISS At, ADSB.lol, Census/ZCTA-derived geographic data, and Mutopia. These services and returned datasets are not licensed by the NABU Command Center MIT License. Their current terms, attribution requirements, availability, and data licenses remain controlling for service use and redistribution. In particular, ADSB.lol documents open aviation data under ODbL-related terms; preserve applicable source attribution when redistributing derived aviation data.

## External runtime applications

MAME/MESS and the NABU Internet Adapter are external applications and are not distributed as part of the project release directory. Their respective licenses and redistribution terms apply separately.
