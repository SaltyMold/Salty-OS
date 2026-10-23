<p align="center"><img src="github_assets/SaltyOS.png" /></p>

Salty-OS is a custom userland for the numworks calculator based on the latest published version of [epsilon](https://github.com/Numworks/epsilon) (26.1.0, branch `version-26`).

<img src="github_assets/numworks.png" width=320 alt="Salty-OS">

## Table of Contents
- [Explanations](#explanations)
- [Features](#functionalities)
- [How to install](#how-to-install)
- [How to use](#how-to-use)
- [How to build](#how-to-build)

## Explanations

A userland is not an OS, it has more restrictions.

NumWorks calculators boot slot A by default, then you can access Slot B with a launcher (ex: [ABLaucher](https://github.com/SaltyMold/ABLauncher-Numworks)).

If the userland crashes or if you reset it, the calculator falls back to booting slot A, but slot B isn't wiped in the process, so the custom userland is still there and can be re-launched afterward (at least on latest versions of epsilon).

Here are some example of the layout of the calculator flash:
- On classic epsilon
<img src="github_assets/slots.png" width=400 alt="slots-default">

- With SaltyOS on slot B for locked calculator
<img src="github_assets/slots_salty_os.png" width=400 alt="slots-salty-os">

- With SaltyOS on slot A for unlocked calculator
<img src="github_assets/slots_salty_os_unlocked.png" width=400 alt="slots-salty-os">

The pythons scripts are in ram, so the layout of the external flash can't affect them.
The bootloader is in another flash, the internal flash.

See more at https://nwagyu.org/reference/firmware/

## Functionalities

The userland is build with a custom onboarding and external apps allowed.
It supports locked n0110, n0115, n0120 and also unlocked n0110 (It has not been tested on unlocked n0115 or n0120, but it could work.) with Epsilon kernel 26.4.0.

### Current

- Multiple themes simultaneously.
- A custom image wallpaper per theme.
- Custom icons per theme.
- Customizable icon form per theme.
- Custom color palette per theme.
- Toggle display of external apps.
- Salty-OS settings app (About and Themes), its information comes from the repository (Makefile, git, `themes/` folder) and from the Theme Area.
- Symbolic calculation
- French by default, and no Python script preinstalled.

Everything customizable via a website.

### Upcoming

Open a pull request if you want to add a feature.

## How to install

If you're unsure what to choose, choose locked calculator.

<details><summary><b>Locked calculator</b></summary>

### Update the calculator twice

- Go to the [Numworks updater](https://my.numworks.com/devices/upgrade).
- Connect your calculator.
- Click update.
- Do it a second time to make sure slot A is used.

### Userland

- Go to https://saltymold.github.io/Salty-OS-Installer/
- Go to the "SaltyOS" section.
- Connect your calculator.
- Click install.

### Launcher

- Go to https://saltymold.github.io/Salty-OS-Installer/
- Go to the "Launcher" section.
- Connect your calculator.
- Click install.

### Themes

- Go to https://saltymold.github.io/Salty-OS-Installer/
- Go to the "Themes" section.
- Connect your calculator.
- Create your themes.
- Click install.
</details>

<details><summary><b>Unlocked calculator</b></summary>

### Flash Epsilon kernel

- Go to the [Upsilon installer](https://getupsilon.web.app/install).
- Connect your calculator.
- Choose "Custom binpack".
- Download epsilon kernel 26.4.0 from the official website with [this guide](https://nwagyu.org/reference/others/downloading-epsilon.html) or from the [archive](https://git.gabizar.top/GabiZar/Epsilon-Archive/releases/tag/26.4.0)
- Flash it at address `0x90000000` for slot A and `0x90400000` for slot B

### Userland

- Go to the [Upsilon installer](https://getupsilon.web.app/install).
- Connect your calculator.
- Choose "Custom binpack".
- Download SaltyOS from the [latest release](https://github.com/SaltyMold/Salty-OS/releases/latest)
- Flash it at address `0x90020000` for slot A and `0x90420000` for slot B

### Themes

- Go to the [Upsilon installer](https://getupsilon.web.app/install).
- Connect your calculator.
- Choose "Custom binpack".
- Create your themes with the python script `themes/generate_theme.py` of this repo.
- Flash `theme_area.bin` at `0x90670000` for slot A and at `0x90270000` for slot B.
</details>

## How to use

Every custom feature of the os has its own keybind.
It is always both parantheses and the keybind.

| Key   | Action        |
|-------|---------------|
| Both parantheses + shift | Previous theme    |
| Both parantheses + alpha | Next theme    |
| Both parantheses + xnt | Toggle external apps    |
| Both parantheses + backspace | Salty-OS settings (About: versions and build, Themes: one folder per theme with all its details) |

## How to build

- Clone the repo.
```sh
git clone https://github.com/SaltyMold/Salty-OS
cd Salty-OS
```

- Activate the virtual environment.
```sh
python3 -m venv .venv
source .venv/bin/activate
```

- Install the dependencies.
```sh
# Use tools/setup.sh or adapt it to your system.
```

- Build the userland for n0120 for exemple.
```sh
make -j$(nproc) PLATFORM=n0120 EMBED_EXTRA_DATA=1 userland.onboarding.allow3rdparty.B.dfu
# With onboarding
# With 3rd party apps allowed
# For slot B
# With embedded extra data
```

- Flash the userland to the calculator.
```sh
python tools/device/dfu.py -D output/release/n0120/userland.onboarding.allow3rdparty.B.dfu -s 0x90420000
```