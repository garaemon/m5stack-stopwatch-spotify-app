# M5Stack StopWatch Spotify app

A standalone app for the M5Stack StopWatch (ESP32-S3, 466x466 round AMOLED).
It shows the album art of the track that is playing on Spotify and lets you
like, pause, and skip tracks from the watch. The device calls the Spotify Web
API directly over Wi-Fi. A PC is needed only for the one-time OAuth login.

## Features

- Album art as the full-screen background, with the title, the artists, and a progress ring.
- Like and unlike the current track.
- Restart, previous, play/pause, and next.
- Prefetches the artwork of the next track.
- Dims the display after 30 seconds and turns it off after 2 minutes, and slows polling to save power.
- Shows Japanese titles and artists.
- Shows connection problems (Wi-Fi, rate limit, expired login) on the display.

## Controls

| Input | Action |
| --- | --- |
| Left button (BtnA), single click | Restart the current track |
| Left button, double click | Previous track |
| Right button (BtnB), single click | Pause or resume |
| Right button, double click | Next track |
| Screen, double tap | Like or unlike the current track |
| Screen, single tap | Nothing |

Any input on a dark display only wakes the display.
A single click takes about 0.35 seconds to register, because the device waits
to see whether a second click follows.

## Requirements

- An M5Stack StopWatch connected over USB.
- A Spotify Premium account. The Web API rejects playback commands for free accounts.
- [PlatformIO](https://platformio.org/) (`pip install platformio`).
- Python 3 on your PC, for the login script.

## Setup

1. Create an app in the [Spotify developer dashboard](https://developer.spotify.com/dashboard).
   Add `http://127.0.0.1:8888/callback` as a redirect URI, and note the client ID.
2. Create `include/secrets.h` from the template, and fill in `WIFI_SSID`,
   `WIFI_PASSWORD`, and `SPOTIFY_CLIENT_ID`. The file is git-ignored.

   ```sh
   cp include/secrets.h.example include/secrets.h
   ```

3. Log in to Spotify. The script opens a browser, receives the callback on
   `127.0.0.1:8888`, and writes `SPOTIFY_REFRESH_TOKEN` into `include/secrets.h`.

   ```sh
   python3 tools/spotify_auth.py
   ```

4. Build and flash the firmware.

   ```sh
   pio run -t upload
   ```

The device renews the access token itself and stores the rotated refresh token
in flash. If the login expires, the display shows "Spotify login expired".
Run `tools/spotify_auth.py` again and flash the device.

## Development

Run the host-side unit tests, which cover the logic in `lib/core`:

```sh
pio test -e native
```

Check the formatting and lint the code, as CI does:

```sh
git ls-files -- '*.h' '*.cpp' | xargs clang-format --dry-run --Werror
clang-tidy lib/core/src/*.cpp -- -std=c++17 -I lib/core/src -I .pio/libdeps/native/ArduinoJson/src
ruff check tools && ruff format --check tools
```

### Layout

- `lib/core/src`: platform-independent logic, such as the JSON parsers, the app state machine, and the power policy. The native tests cover it.
- `src/`: device code for Wi-Fi, HTTPS, the display, and input.
- `src/network_worker.cpp`: runs every Spotify request on a FreeRTOS task, so that network latency never stalls input or rendering.
- `tools/spotify_auth.py`: the OAuth login helper.

## Limitations

- Hangul and other scripts missing from the efontJA font do not render.
- Spotify's development mode limits an app to a small number of users.

## License

[MIT](LICENSE)
