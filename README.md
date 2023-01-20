# pngcpp
Toy project to experiment with PNG file format

This is a C++ implementation of the Blog entry by [David Buchanan](https://www.da.vidbuchanan.co.uk/blog/hello-png.html).

His writeup was well done and I wanted to not only understand it, but test my ability in C++. It's not the cleanest code and there are no doubt many bugs and/or issues that should be done better. I'm still learning C++ so this is really just a proof of concept.

## Usage
Developed in Debian via WSL so building on different platforms could be an issue. Just close the repository and ensure you have zlib and run make.

Start with an image file in rgb format. You can use ImageMagick to convert and existing image:

```shell
$ convert ./samples/hello_png_original.png ./samples/hello_png.rgb
```

Then supply the rgb file to the app along with the output filename, width, and height. It will then convert it to a png file:

```shell
$ ./pngcpp hello_png.rgb new_file.png 320 180
```

## Issues
- Currently not checking platform for big or little endian, so it may not work on some systems.
- Segfaults when width and height are set to incorrect sizes sometimes. This is most likely due to how memory is allocated. Since it compresses the file, it allocates space equal to the original input file and if you specify an output size that exceeds that, it breaks. I may get around to fixing that eventually.