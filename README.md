# ChainPack - the best from both XML and JSON
## Motivation
### XML
PROS:
* ability to annotate element with attributes

CONS:
* boring verbose tag closing, not very usefull when doc is generated by machine.
* element anotations are not XML. CSS might be also XML if this would be.
* no standard binary representation
### JSON
PROS:
* not as verbose as XML

CONS:
* no attributes.
* no standard binary representation
* lack of BLOB type
* Strings are not encoded in UTF8

### ChainPack
PROS:
* binary `ChainPack` and text `Cpon` representation with easy and fast conversion (see `cp2cp`)
* any value can have metadata. Metadata itself are ChainPack/Cpon again. Even more, the metatada values can have own metadata etc.
* `Cpon` is `JSON` superset with only exception that string values are always encoded in UTF8.
* binary packed data are relatively small. Size is slightly bigger than `MessagePack` or `Cbor`
* BLOB support
* C, C++, D, Python, JavaScript bindings

CONS:
* user have to learn something very new with poor community support

More detailed information can be found on wiki.

## Build
```sh
qmake SHV_PROJECT_TOP_SRCDIR=`pwd` SHV_PROJECT_TOP_BUILDDIR=`pwd`
make
```
QtCreator qmake additional arguments
```
SHV_PROJECT_TOP_SRCDIR=%{sourceDir} SHV_PROJECT_TOP_BUILDDIR=%{buildDir}
```
