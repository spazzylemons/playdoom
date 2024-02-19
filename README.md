# PlayDoom

A port of the Doom engine to Playdate based on Chocolate Doom.

## Building

PlayDoom can be built using the Playdate SDK.

## Usage

No WADs are provided. Place a WAD in the game's data folder (PLAYDATE/Data/me.spazzylemons.PlayDoom)
or the game will not run.

### Controls

| Button(s)     | Normal action   | Automap action |
| ------------- | --------------- | -------------- |
| Up            | Move forward    | Pan north      |
| Down          | Move backward   | Pan south      |
| Left          | Strafe left     | Pan west       |
| Right         | Strafe right    | Pan east       |
| A             | Fire            | Add mark       |
| B + Up        | Open automap    | Close automap  |
| B + Down      | Toggle autorun  | Toggle grid    |
| B + Left      | Previous weapon | Zoom out       |
| B + Right     | Next weapon     | Zoom in        |
| B + A         | Use             | Clear marks    |

Additionally, the crank is used to turn.

The game menu can be opened by selecting "open menu" in the pause menu. Cheats
can be entered by selecting "enter cheat" in the pause menu and using the
on-screen keyboard.

## Roadmap

The game is fully functional, but certain modules such as rendering may be
optimized further, and there are no configuration options available to select
an IWAD or PWADs, fast monsters, etc. In the future this may be added.

## Credits

- Music driver adapted from [TinySoundFont](https://github.com/schellingb/TinySoundFont)
- Soundfont taken from [DSDA-Doom](https://github.com/kraflab/dsda-doom)
- PlayDoom is built on [Chocolate Doom](https://www.chocolate-doom.org), v3.0.1.

## License

This port is licensed under GPL3 (See LICENSE).
