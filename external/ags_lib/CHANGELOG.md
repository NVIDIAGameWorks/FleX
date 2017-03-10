# AMD AGS Library Changelog

### v4.0.3 - 2016-08-18
* Improve support for DirectX 11 and DirectX 12 GCN shader extensions
* Add support for Multidraw Indirect Count Indirect for DirectX 11
* Fix clock speed information for Polaris GPUs
* Requires Radeon Software Crimson Edition 16.9.1 (driver version 16.40) or later

### v4.0.0 - 2016-05-24
* Add support for GCN shader extensions
  * Shader extensions are exposed for both DirectX 11 and DirectX 12
  * Requires Radeon Software Crimson Edition 16.5.2 or later
* Remove `RegisterApp` from the extension API
  * This extension is not currently supported in the driver

### v3.2.2 - 2016-05-23
* Add back `radeonSoftwareVersion` now that updated driver is public
  * Radeon Software Crimson Edition 16.5.2 or later
* Fix GPU info when primary adapter is > 0
* Update the implementation of agsDriverExtensions_NotifyResourceEndWrites

### v3.2.0 - 2016-02-12
* Add ability to disable Crossfire
  * This is in addition to the existing ability to enable the explicit Crossfire API
  * Desired Crossfire mode is now passed in to `agsInit`
  * Separate `SetCrossfireMode` function has been removed from the AGS API
  * The `agsInit` function should now be called **prior to device creation**
* Return library version number in the optional info parameter of `agsInit`
* Build amd_ags DLLs such that they do not depend on any Microsoft Visual C++ redistributable packages

### v3.1.1 - 2016-01-28
* Return null for the context when initialization fails
* Add version number defines to `amd_ags.h`
* Remove `radeonSoftwareVersion` until needed driver update is public

### v3.1.0 - 2016-01-26
* Initial release on GitHub
