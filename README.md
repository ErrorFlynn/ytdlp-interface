# ytdlp-interface
This is a Windows graphical interface for [yt-dlp](https://github.com/yt-dlp/yt-dlp), that is designed as a simple YouTube downloader. It doesn't download playlists, and generally speaking it doesn't attempt to accommodate the crazy number of features offered by yt-dlp (it just downloads a video using the selected format, with a few options for the download).

Some third-party binaries are distributed with the program binary:
- `curl.exe` : used by the program to download a video thumbnail to display in the interface (downloaded from https://curl.se/windows)
- `ffmpeg.exe`, `ffprobe.exe` : used by `yt-dlp.exe` for postprocessing (muxing formats, etc), downloaded from https://github.com/yt-dlp/FFmpeg-Builds

To use, it's easiest to just dump the files in the folder where `yt-dlp.exe` is located, although you can put them anywhere. To completely remove from your system, also delete the settings file `%AppData%\ytdlp-interface.json`.

---

## Building the source
The project depends on two static libraries: [Nana C++ GUI library](https://github.com/cnjinhao/nana) v1.8 or later (at the time I'm writing this v1.8 is in development, so you must build branch `develop-1.8`), and [libjpeg-turbo](https://github.com/libjpeg-turbo/libjpeg-turbo). To build Nana with JPEG support see [this thread](http://nanapro.org/en-us/forum/index.php?u=/topic/1368/ggjpg).

The program also uses [JSON for modern C++](https://github.com/nlohmann/json) to get video info from `yt-dlp.exe` and to read/write the settings file, but that's just a header file that's included in the project (you can replace it with its latest version if you really want to).

The easiest way to build the project is to open the solution file with Visual Studio 2019 or later. If that's not an option, you're a resourceful individual, I'm sure you'll come up with something (that's as far as my support goes, sorry).

---

![ytdlp-interface 1](https://user-images.githubusercontent.com/20293505/144982043-7f2eff3f-856b-4426-b61d-df53decf070a.png)

---

![ytdlp-interface 2](https://user-images.githubusercontent.com/20293505/144982366-c942426e-95c4-4dfc-ac0e-d3edf64088fb.png)

---

![ytdlp-interface 3](https://user-images.githubusercontent.com/20293505/144983375-348c6569-7cbf-4516-b73f-f754cf954ea0.png)
