# ytdlp-interface
This is a Windows graphical interface for [yt-dlp](https://github.com/yt-dlp/yt-dlp), that is designed as a simple YouTube downloader. Since v1.2, the interface also accepts non-YouTube URLs, so theoretically it can be used to download from any site that `yt-dlp` supports (see [the list](https://github.com/yt-dlp/yt-dlp/blob/master/supportedsites.md)).

Some third-party binaries are distributed with the program binary:
- `yt-dlp.exe` : the command-line tool that downloads media from YouTube and other sites; from v1.1 forward, this is included for user convenience
- `ffmpeg.exe`, `ffprobe.exe` : used by `yt-dlp.exe` for postprocessing (muxing formats, etc), downloaded from https://github.com/yt-dlp/FFmpeg-Builds
- `7z.dll` : 7-Zip plugin, used by the program to extract files from an archive when updating self or FFmpeg (since v1.5)

To use, unpack the archive in a new folder at a location of your choice, and run `ytdlp-interface.exe`. The program writes a settings file named `ytdlp-interface.json` to the folder it's in, or to `%AppData%` if the folder is not write accessible. For example, an application can't write in the `Program Files` folder (or any subfolder thereof) unless it has administrative permissions, so if `ytdlp-interface.exe` is located there, it won't be able to update `yt-dlp` and `FFmpeg`, or write the settings file there, unless it has administrator permissions (you would have to right-click on file and select `Run as administrator`, or in the properties of the shortcut, choose `Advanced...`->`Run as administrator`).

Download link: https://github.com/ErrorFlynn/ytdlp-interface/releases/download/v1.8.2/ytdlp-interface.7z

---

## Building the source
The project depends on three static libraries: [Nana C++ GUI library](https://github.com/cnjinhao/nana) v1.8 or later (at the time I'm writing this v1.8 is in development, so you must build branch `develop-1.8`), [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo), and [bit7z](https://github.com/rikyoz/bit7z). To build Nana with JPEG support see [this thread](http://nanapro.org/en-us/forum/index.php?u=/topic/1368/ggjpg).

The program also uses [JSON for modern C++](https://github.com/nlohmann/json) to get video info from `yt-dlp.exe` and to read/write the settings file, but that's just a header file that's included in the project (you can replace it with its latest version if you really want to).

The easiest way to build the project is to open the solution file with Visual Studio 2019 or later. If that's not an option, you're a resourceful individual, I'm sure you'll come up with something (that's as far as my support goes, sorry).

---

![ytdlp-interface_settings](https://user-images.githubusercontent.com/20293505/199632897-31020961-63b8-4a8f-98e5-9cda8adc4340.png)

---

![ytdlp-interface_queue](https://user-images.githubusercontent.com/20293505/199632781-db7aa408-15cb-44ea-b5a5-e67e595e2372.png)

---

![ytdlp-interface_output](https://user-images.githubusercontent.com/20293505/199632812-21d21500-3acf-453c-bd1b-a445eec03f7a.png)
