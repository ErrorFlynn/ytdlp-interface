# ytdlp-interface
This is a Windows graphical interface for [yt-dlp](https://github.com/yt-dlp/yt-dlp), that is designed as a simple YouTube downloader. Since v1.2, the interface also accepts non-YouTube URLs, so theoretically it can be used to download from any site that `yt-dlp` supports (see [the list](https://docs.yt-dlp.org/en/latest/supportedsites.html)).

Some third-party binaries are distributed with the program binary:
- `yt-dlp.exe` : the command-line tool that downloads media from YouTube and other sites; from v1.1 forward, this is included for user convenience
- `ffmpeg.exe`, `ffprobe.exe` : used by `yt-dlp.exe` for postprocessing (muxing formats, etc), downloaded from https://github.com/yt-dlp/FFmpeg-Builds
- `7zxa.dll` : 7-Zip extraction plugin, used by the program to unpack the archive of the new version when self-updating (since v1.3)

To use, unpack the archive in a new folder at a location of your choice, and run `ytdlp-interface.exe`. To completely remove from your system, also delete the settings file `%AppData%\ytdlp-interface.json`.

Download link: https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v1.3.0/ytdlp-interface.7z

---

## Building the source
The project depends on two static libraries: [Nana C++ GUI library](https://github.com/cnjinhao/nana) v1.8 or later (at the time I'm writing this v1.8 is in development, so you must build branch `develop-1.8`), and [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo). To build Nana with JPEG support see [this thread](http://nanapro.org/en-us/forum/index.php?u=/topic/1368/ggjpg).

The program also uses [JSON for modern C++](https://github.com/nlohmann/json) to get video info from `yt-dlp.exe` and to read/write the settings file, but that's just a header file that's included in the project (you can replace it with its latest version if you really want to).

The easiest way to build the project is to open the solution file with Visual Studio 2019 or later. If that's not an option, you're a resourceful individual, I'm sure you'll come up with something (that's as far as my support goes, sorry).

---

![ytdlp-interface v1 3 - page 1](https://user-images.githubusercontent.com/20293505/154186548-e1b36f48-dfaf-4673-87f9-bc0678f5a88c.png)

---

![ytdlp-interface v1 3 - settings](https://user-images.githubusercontent.com/20293505/154186098-f0e83df1-a026-4869-8e80-ce6595d56aef.png)

---

![ytdlp-interface v1 3 - download page](https://user-images.githubusercontent.com/20293505/154185788-cadd2d0b-7915-45f2-9772-c5bf7f363afc.png)
