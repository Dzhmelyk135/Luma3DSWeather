# 🌤️ Luma3DS Weather

> A working weather app for all consoles of the Nintendo 3DS family with custom firmware installed. Works even with smaller and less known cities, this is why I made it, other existing apps weren't able to display correctly my city.

![Platform](https://img.shields.io/badge/platform-Nintendo%203DS-red)
![Language](https://img.shields.io/badge/language-C-blue)
![CFW](https://img.shields.io/badge/CFW-Luma3DS-orange)
![API](https://img.shields.io/badge/data-Open--Meteo-green)
![License](https://img.shields.io/badge/license-MIT-yellow)

---

## 📸 Screenshots

> *Screenshots coming soon.*

---

## ✨ Features

- 🌍 **Works with any city in the world** — including small and less known ones (like Pegognaga, MN, Italy!)
- 📡 **Real-time weather data** via [Open-Meteo](https://open-meteo.com/) — free, no API key required
- 🕐 **Hourly forecast** — temperature, precipitation, humidity and weather condition for each hour of today
- 📅 **7-day forecast** — max/min temperature, precipitation, wind speed and weather condition
- 📊 **Additional data** — atmospheric pressure, wind speed & direction, UV index, feels-like temperature, sunrise & sunset times, with visual bar indicators
- 🏙️ **Multiple cities** — save up to 20 cities and switch between them instantly
- 🔀 **City reordering** — reorder your saved cities with an intuitive drag interface
- 🌐 **7 languages** supported:
  - 🇮🇹 Italiano
  - 🇬🇧 English
  - 🇫🇷 Français
  - 🇪🇸 Español
  - 🇩🇪 Deutsch
  - 🇺🇦 Ukrainska (romanized)
  - 🇯🇵 Nihongo (romanized)
- 💾 **Persistent settings** — language and cities are saved to the SD card and remembered on next launch
- 📖 **Symbol legend** — built-in legend screen explaining all weather icons
- 🔋 **Lightweight** — console-based UI, no heavy graphics, fast and responsive

---

## 🎮 Compatibility

| Console | Supported |
|---------|-----------|
| Nintendo 3DS | ✅ |
| Nintendo 3DS XL | ✅ |
| Nintendo 2DS | ✅ |
| New Nintendo 3DS | ✅ |
| New Nintendo 3DS XL | ✅ |
| New Nintendo 2DS XL | ✅ |

> **Requires:** Custom firmware (Luma3DS recommended) and an active WiFi connection.

---

## 🗺️ Controls

### 🏙️ City List

| Button | Action |
|--------|--------|
| **UP / DOWN** | Navigate cities |
| **A** | Download & show weather |
| **X** | Add a new city |
| **Y** | Delete selected city |
| **SELECT** | Reorder cities |
| **L** | Open language selection |
| **R** | Open symbol legend |
| **START** | Exit app |

### 🌡️ Current Weather

| Button | Action |
|--------|--------|
| **L** | Hourly forecast |
| **R** | 7-day forecast |
| **X** | Additional details |
| **B** | Back to city list |
| **START** | Exit app |

### 🕐 Hourly Forecast

| Button | Action |
|--------|--------|
| **UP / DOWN** | Scroll hours |
| **R** | 7-day forecast |
| **B** | Back |

### 📅 7-Day Forecast

| Button | Action |
|--------|--------|
| **L** | Hourly forecast |
| **X** | Additional details |
| **B** | Back |

### 🔀 City Reorder

| Button | Action |
|--------|--------|
| **A** | Pick up / put down city |
| **UP / DOWN** | Move selected city |
| **B** | Save and go back |

---

## 📖 Symbol Legend

| Symbol | Meaning |
|--------|---------|
| `(*)` | ☀️ Clear sky |
| `(^)` | 🌤️ Partly cloudy |
| `(n)` | ☁️ Cloudy / Overcast |
| `~~~` | 🌫️ Fog |
| `._.` | 🌦️ Drizzle |
| `.|.` | 🌧️ Rain / Showers |
| `***` | ❄️ Snow |
| `/!/` | ⛈️ Thunderstorm |
| `???` | ❓ Unknown |

---

## 🛠️ Building from source

### Requirements

- [devkitPro](https://devkitpro.org/) with **devkitARM** and **libctru**
- Linux, macOS or Windows (with WSL)

### Setup devkitPro (Linux)

```bash
wget https://apt.devkitpro.org/install-devkitpro-pacman
chmod +x install-devkitpro-pacman
sudo ./install-devkitpro-pacman
sudo dkp-pacman -S 3ds-dev
source /etc/profile.d/devkit-env.sh
```

### Clone & build

```bash
git clone https://github.com/YOUR_USERNAME/luma3ds-weather.git
cd luma3ds-weather
make
```

The compiled `3ds-weather.3dsx` file will appear in the project root.

### Clean build

```bash
make clean && make
```

---

## 📦 Installation

1. Copy `3ds-weather.3dsx` to your SD card:
   ```
   SD:/3ds/3ds-weather/3ds-weather.3dsx
   ```
2. Launch the **Homebrew Launcher** on your 3DS
3. Select **3DS Weather** and press **A**
4. Make sure your 3DS is connected to WiFi
5. Press **X** to add your first city and enjoy! 🌤️

> **Note:** The app will automatically create the folder `/3ds/3ds-weather/` on first launch and save your cities and language preference there.

---

## 📁 Project structure

```
luma3ds-weather/
├── Makefile
├── icon.png
├── README.md
└── source/
    ├── main.c        # Main loop, UI screens, input handling
    ├── weather.c     # HTTP requests, JSON parsing, Open-Meteo API
    ├── weather.h
    ├── cities.c      # City list management, save/load from SD
    ├── cities.h
    ├── lang.c        # Multilanguage string table (7 languages)
    ├── lang.h
    ├── jsmn.c        # Lightweight JSON parser (MIT)
    └── jsmn.h
```

---

## 🌐 API

This app uses **[Open-Meteo](https://open-meteo.com/)** — a free, open-source weather API:

- ✅ No API key required
- ✅ No account needed
- ✅ Works with any coordinates worldwide
- ✅ Supports HTTP (no HTTPS certificate issues on 3DS)
- ✅ Geocoding API for city name search

Weather data © [Open-Meteo.com](https://open-meteo.com/)

---

## 📜 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

---

## 🙏 Credits

- **[Open-Meteo](https://open-meteo.com/)** — free weather & geocoding API
- **[devkitPro](https://devkitpro.org/)** — 3DS homebrew toolchain
- **[libctru](https://github.com/devkitPro/libctru)** — Nintendo 3DS userland library
- **[jsmn](https://github.com/zserge/jsmn)** — lightweight JSON parser by Serge Zaitsev (MIT)
- **[Luma3DS](https://github.com/LumaTeam/Luma3DS)** — custom firmware

---

*Made with ❤️ because other weather apps couldn't find Pegognaga (MN, Italy).*
