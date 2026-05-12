# Arcardputer

A multi-game arcade casino station for the **M5Stack Cardputer** (ESP32-S3, 240×135 TFT, QWERTY keyboard).

Six games in one firmware — navigate via the grid menu and press Space/Enter to play.

---

## Games

| # | Game | Description | Controls |
|---|------|-------------|----------|
| 1 | **Slot Machine** | 3 reels, 7 symbols, sprite graphics | SPC=spin, +/-=bet, Q=menu |
| 2 | **Blackjack** | Beat the dealer to 21 | H=hit, S=stand, D=double, SPC=deal, Q=menu |
| 3 | **Dice Roll** | 5 dice, 3 rolls, Yahtzee-style scoring | 1-5=hold, SPC=roll, Q=menu |
| 4 | **Video Poker** | 5-card draw, Jacks or Better pay table | 1-5=hold, SPC=deal/draw, Q=menu |
| 5 | **Hi / Lo** | Guess higher or lower, chain multiplier | H=higher, L=lower, C=cashout, SPC=deal, Q=menu |
| 6 | **Roulette** | European wheel, 37 pockets, 9 bet types | A/D=bet type, +/-=bet, SPC=spin, Q=menu |

All games start with **100 credits** and reset if you go broke.

---

## Menu Navigation

```
+----------+----------+
| SLOT     | BLACKJACK|
+----------+----------+
| DICE     | VIDEO    |
|          | POKER    |
+----------+----------+
| HI / LO  | ROULETTE |
+----------+----------+
```

- **A / –** — previous game
- **D / +** — next game
- **W** — up one row (–2)
- **S** — down one row (+2)
- **Space / Enter** — launch selected game
- **Q** (in any game) — return to menu

---

## Sound

All games share a unified sound toggle:

- **M** — toggle sound on/off (default: off)
- **[** / **]** — decrease / increase volume (5 levels)

---

## Roulette Bet Types

| Key (A/D) | Bet | Payout |
|-----------|-----|--------|
| RED | Red numbers | 2× |
| BLACK | Black numbers | 2× |
| ODD | Odd numbers | 2× |
| EVEN | Even numbers | 2× |
| 1-18 | Low half | 2× |
| 19-36 | High half | 2× |
| 1-12 | First dozen | 3× |
| 13-24 | Second dozen | 3× |
| 25-36 | Third dozen | 3× |

---

## Video Poker Pay Table

| Hand | Pay (×bet) |
|------|-----------|
| Jacks or Better | 1× |
| Two Pair | 2× |
| Three of a Kind | 3× |
| Straight | 4× |
| Flush | 6× |
| Full House | 9× |
| Four of a Kind | 25× |
| Straight Flush | 50× |
| Royal Flush | 250× |

---

## Build & Flash

Requires [PlatformIO](https://platformio.org/) and the M5Stack board package.

```bash
# Build
pio run

# Build and upload
pio run --target upload

# Monitor serial
pio device monitor
```

`platformio.ini` targets `m5stack-cardputer` with M5Unified and M5GFX libraries.

---

## Hardware

- **Device**: M5Stack Cardputer (M5CardputerADV)
- **MCU**: ESP32-S3
- **Display**: 240×135 ST7789 TFT (landscape)
- **Input**: QWERTY physical keyboard (I2C)
- **Speaker**: internal buzzer via M5Unified

---

## Project Structure

```
src/
├── main.cpp          # Menu, game dispatch loop
├── slots.h           # Slot Machine
├── blackjack.h       # Blackjack
├── dice.h            # Dice / Yahtzee
├── videopoker.h      # Video Poker
├── hilo.h            # Hi / Lo
├── roulette.h        # Roulette (elliptical wheel)
├── img_cherry.h      # Sprite data (48×48 RGB565)
├── img_lemon.h
├── img_orange.h
├── img_grape.h
├── img_watermelon.h
├── img_seven.h
└── img_bell.h
```

---

## License

MIT — personal / educational use.
