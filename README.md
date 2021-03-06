# ytdlp-interface
This is a Windows graphical interface for [yt-dlp](https://github.com/yt-dlp/yt-dlp), that is designed as a simple YouTube downloader. Since v1.2, the interface also accepts non-YouTube URLs, so theoretically it can be used to download from any site that `yt-dlp` supports (see [the list](https://github.com/yt-dlp/yt-dlp/blob/master/supportedsites.md)).

Some third-party binaries are distributed with the program binary:
- `yt-dlp.exe` : the command-line tool that downloads media from YouTube and other sites; from v1.1 forward, this is included for user convenience
- `ffmpeg.exe`, `ffprobe.exe` : used by `yt-dlp.exe` for postprocessing (muxing formats, etc), downloaded from https://github.com/yt-dlp/FFmpeg-Builds
- `7z.dll` : 7-Zip plugin, used by the program to extract files from an archive when updating self or FFmpeg (since v1.5)

To use, unpack the archive in a new folder at a location of your choice, and run `ytdlp-interface.exe`. The program writes a settings file named `ytdlp-interface.json` to the folder it's in, or to `%AppData%` if the folder is not write accessible. For example, an application can't write in the `Program Files` folder (or any subfolder thereof) unless it has administrative permissions, so if `ytdlp-interface.exe` is located there, it won't be able to update `yt-dlp` and `FFmpeg`, or write the settings file there, unless it has administrator permissions (you would have to right-click on file and select `Run as administrator`, or in the properties of the shortcut, choose `Advanced...`->`Run as administrator`).

Download link: https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v1.5.1/ytdlp-interface.7z

---

## Building the source
The project depends on three static libraries: [Nana C++ GUI library](https://github.com/cnjinhao/nana) v1.8 or later (at the time I'm writing this v1.8 is in development, so you must build branch `develop-1.8`), [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo), and [bit7z](https://github.com/rikyoz/bit7z). To build Nana with JPEG support see [this thread](http://nanapro.org/en-us/forum/index.php?u=/topic/1368/ggjpg).

The program also uses [JSON for modern C++](https://github.com/nlohmann/json) to get video info from `yt-dlp.exe` and to read/write the settings file, but that's just a header file that's included in the project (you can replace it with its latest version if you really want to).

The easiest way to build the project is to open the solution file with Visual Studio 2019 or later. If that's not an option, you're a resourceful individual, I'm sure you'll come up with something (that's as far as my support goes, sorry).

---

![ytdlp-interface v1 5 - page 1](https://user-images.githubusercontent.com/20293505/171078136-79b1b793-b0d7-4dce-8a9f-8bf09772f545.png)

---

![ytdlp-interface v1 5 - settings](https://user-images.githubusercontent.com/20293505/171078171-ec4f35ed-60ee-4a5a-9bb6-dd4ebecdd0c9.png)

---

![ytdlp-interface v1 5 - updater](https://user-images.githubusercontent.com/20293505/171094668-3a8807a4-9389-44e3-a056-8bb00de536bd.png)

---

![ytdlp-interface v1 5 - download page](https://user-images.githubusercontent.com/20293505/171078195-fef579dc-852c-41bc-9e37-83619bbf2109.png)
