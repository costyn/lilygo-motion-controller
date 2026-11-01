Release ${nextRelease.version} for LilyGo Motion Controller

### Features

* choose the chopper mode per move and track the shaft while holding ([45329a5](https://github.com/costyn/lilygo-motion-controller/commit/45329a521d11f537304a2acde68e585b6aa8b183))


### Bug Fixes

* run SpreadCycle at all speeds ([b4c7f56](https://github.com/costyn/lilygo-motion-controller/commit/b4c7f567f86b0c257e6ea48075f6e379729671a4))
* stop the motor moving by itself after being turned by hand ([0ddab0d](https://github.com/costyn/lilygo-motion-controller/commit/0ddab0db7697628053fb3a3c19e0c072b828dfeb))

Release ${nextRelease.version} for LilyGo Motion Controller

### Features

* add Set Min and Set Max buttons and a stall warning ([1ce9d1d](https://github.com/costyn/lilygo-motion-controller/commit/1ce9d1df64e57e7c8842f1d1c6344bdf7d59f906))
* follow the shaft when the motor is turned by hand ([fbb79af](https://github.com/costyn/lilygo-motion-controller/commit/fbb79af6f663dfc58455b2a485cb1773283b3d43))
* lower default acceleration and widen the settings dialog ([82e627c](https://github.com/costyn/lilygo-motion-controller/commit/82e627c52e411a6e168e1ed22771a4166490f6ed))
* replace closed-loop control with measure-only encoder tracking ([9c74fff](https://github.com/costyn/lilygo-motion-controller/commit/9c74ffff0d3d12cdb6f81b0be486b99e7c93a1a5))
* require calibration after boot and zero position at the MIN switch ([322e9ce](https://github.com/costyn/lilygo-motion-controller/commit/322e9cedace87ff49b59f82e6c490bc4149431ab))
* stop on stalls and correct lost steps at the end of a move ([e3c4da5](https://github.com/costyn/lilygo-motion-controller/commit/e3c4da5e9198fa19f16a8d2adeb6b5098f3536d2))


### Bug Fixes

* correct swapped chopper modes and stop calibration if the motor jams ([c92f789](https://github.com/costyn/lilygo-motion-controller/commit/c92f789cc99b426c0769c1cae55560de992aa985))
* give the stepping loop Core 1 to itself ([1c23158](https://github.com/costyn/lilygo-motion-controller/commit/1c231585b5c59f0e2dc998fafb9acc6a858e8e04))
* limit acceleration settings to 200,000 steps/sec² ([bd3a289](https://github.com/costyn/lilygo-motion-controller/commit/bd3a2898f8ee49f625aed9622ebd6c1490d486be))
* stop alert icons overlapping the title ([1e7fda4](https://github.com/costyn/lilygo-motion-controller/commit/1e7fda49e1a83542d059ca2ac550b4b8423fc84c))
* stop driver mode switches pausing step pulses ([7aa7436](https://github.com/costyn/lilygo-motion-controller/commit/7aa7436c25d3d61b79e0a515413dcb52387e970a))


### Code Refactoring

* let the driver switch chopper modes at a fixed speed ([b9bacb0](https://github.com/costyn/lilygo-motion-controller/commit/b9bacb04f3a0ab39eb1cdb3eb3a25a3cf30e6c97))

Release ${nextRelease.version} for LilyGo Motion Controller

### Features

* add direct drive calibration mode with configurable slow speed ([88c542d](https://github.com/costyn/lilygo-motion-controller/commit/88c542d92d9a45a6053fc3177406adb95b110393))

Release ${nextRelease.version} for LilyGo Motion Controller

### Bug Fixes

* use pnpm also in Github Action ([84670bd](https://github.com/costyn/lilygo-motion-controller/commit/84670bd198378635dbd007355381d940de9b31b0))
