# ytdlp-interface
This is a Windows graphical interface for [yt-dlp](https://github.com/yt-dlp/yt-dlp), that is designed as a simple YouTube downloader. Since v1.2, the interface also accepts non-YouTube URLs, so theoretically it can be used to download from any site that `yt-dlp` supports (see [this list](https://docs.yt-dlp.org/en/latest/supportedsites.html)).

Some third-party binaries are distributed with the program binary:
- `yt-dlp.exe` : the command-line tool that downloads videos from YouTube; from v1.1 forward, this is included for user convenience
- `ffmpeg.exe`, `ffprobe.exe` : used by `yt-dlp.exe` for postprocessing (muxing formats, etc), downloaded from https://github.com/yt-dlp/FFmpeg-Builds

To use, unpack the archive in a new folder at a location of your choice, and run `ytdlp-interface.exe`. To completely remove from your system, also delete the settings file `%AppData%\ytdlp-interface.json`.

Download link: https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v1.2.0/ytdlp-interface.7z

---

## Building the source
The project depends on two static libraries: [Nana C++ GUI library](https://github.com/cnjinhao/nana) v1.8 or later (at the time I'm writing this v1.8 is in development, so you must build branch `develop-1.8`), and [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo). To build Nana with JPEG support see [this thread](http://nanapro.org/en-us/forum/index.php?u=/topic/1368/ggjpg).

The program also uses [JSON for modern C++](https://github.com/nlohmann/json) to get video info from `yt-dlp.exe` and to read/write the settings file, but that's just a header file that's included in the project (you can replace it with its latest version if you really want to).

The easiest way to build the project is to open the solution file with Visual Studio 2019 or later. If that's not an option, you're a resourceful individual, I'm sure you'll come up with something (that's as far as my support goes, sorry).

---

![ytdpl-interface page1](https://user-images.githubusercontent.com/20293505/146545946-82fc75fe-959e-43ef-a253-b669fd6afb9b.png)

---

![ytdpl-interface page2](https://user-images.githubusercontent.com/20293505/146546331-c32c9d20-7f0f-4002-9096-c5cc40032dab.png)

---

![ytdpl-interface page3](https://user-images.githubusercontent.com/20293505/146545999-566f8a9e-1cb4-47c7-87c0-9c0aec9817b7.png)
