# ytdlp-interface
This is a Windows graphical interface for [yt-dlp](https://github.com/yt-dlp/yt-dlp), that is designed as a simple YouTube downloader. Since v1.2, the interface also accepts non-YouTube URLs, so theoretically it can be used to download from any site that `yt-dlp` supports (see [the list](https://docs.yt-dlp.org/en/latest/supportedsites.html)).

Some third-party binaries are distributed with the program binary:
- `yt-dlp.exe` : the command-line tool that downloads media from YouTube and other sites; from v1.1 forward, this is included for user convenience
- `ffmpeg.exe`, `ffprobe.exe` : used by `yt-dlp.exe` for postprocessing (muxing formats, etc), downloaded from https://github.com/yt-dlp/FFmpeg-Builds

To use, unpack the archive in a new folder at a location of your choice, and run `ytdlp-interface.exe`. To completely remove from your system, also delete the settings file `%AppData%\ytdlp-interface.json`.

Download link: https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v1.2.0/ytdlp-interface.7z

---

## Building the source
The project depends on two static libraries: [Nana C++ GUI library](https://github.com/cnjinhao/nana) v1.8 or later (at the time I'm writing this v1.8 is in development, so you must build branch `develop-1.8`), and [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo). To build Nana with JPEG support see [this thread](http://nanapro.org/en-us/forum/index.php?u=/topic/1368/ggjpg).

The program also uses [JSON for modern C++](https://github.com/nlohmann/json) to get video info from `yt-dlp.exe` and to read/write the settings file, but that's just a header file that's included in the project (you can replace it with its latest version if you really want to).

The easiest way to build the project is to open the solution file with Visual Studio 2019 or later. If that's not an option, you're a resourceful individual, I'm sure you'll come up with something (that's as far as my support goes, sorry).

---

![ytdpl-interface 1](https://user-images.githubusercontent.com/20293505/148423527-1c6d828f-2006-46d4-ae61-e94e75864f71.png)

---

![ytdpl-interface 2](https://user-images.githubusercontent.com/20293505/148423559-6887482d-f684-4a7d-a73c-808b624d8a02.png)

---

![ytdpl-interface 3](https://user-images.githubusercontent.com/20293505/148423581-b9a131cc-1d1b-46ea-a909-c6335d5bea76.png)
