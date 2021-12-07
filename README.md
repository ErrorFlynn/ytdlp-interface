# ytdlp-interface
This is a Windows graphical interface for [yt-dlp](https://github.com/yt-dlp/yt-dlp), that is designed as a simple YouTube downloader. It doesn't download playlists, and generally speaking it doesn't attempt to accomodate the crazy number of options offered by yt-dlp (it just downloads a video using the selected format, with a few options for the download).

Some third-party binaries are distributed with the program binary:
- `curl.exe` : used by the program to download a video thumbnail to display in the interface (downloaded from https://curl.se/windows)
- `ffmpeg.exe`, `ffprobe.exe` : used by `yt-dlp.exe` for postprocessing (muxing formats, etc), downloaded from https://github.com/yt-dlp/FFmpeg-Builds

To use, it's easiest to just dump the files in the folder where `yt-dlp.exe` is located, although you can put them anywhere. To completely remove from your system, also delete the settings file `%AppData%\ytdlp-interface.json`.

---

![ytdlp-interface 1](https://user-images.githubusercontent.com/20293505/144982043-7f2eff3f-856b-4426-b61d-df53decf070a.png)

---

![ytdlp-interface 2](https://user-images.githubusercontent.com/20293505/144982366-c942426e-95c4-4dfc-ac0e-d3edf64088fb.png)

---

![ytdlp-interface 3](https://user-images.githubusercontent.com/20293505/144983375-348c6569-7cbf-4516-b73f-f754cf954ea0.png)
