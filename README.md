# fiat imago

*Let there be image.* A photo developer for Sailfish OS: it reads the RAW files
[RAWfish](https://github.com/Logic-gate/RAWfish) saves and ordinary JPEG photos,
and adjusts light, colour, crop, sharpness and vignette with a live preview.
The original is never changed; every export is a new JPEG in
`Pictures/fiat imago`.

Part of the fiat family by [Munkstolen](https://munkstolen.se).

## Building

The shared family components, the family icons and the LICENSE come from
Fiat Lux. Copy them in once before the first build:

```bash
tools/copy-family-parts.sh          # or LUX=/path/to/FiatLux tools/copy-family-parts.sh
```

The script swaps `FiatLuxTheme` for `FiatImagoTheme` and prints four checks;
the first two must be empty. The `.pro` refuses to build without the copied
files.

```bash
sfdk build
```

## How it works

Everything is developed on the CPU in C++ with 32-bit floats, one code path
for the live preview (1200 px), the 100 % view and the full-size export, so
what you see is what you export. Radii are fractions of the frame width, not
pixels, for the same reason.

- **RAW:** RAW16, RAW12, RAW10 and RAW8, the packing taken from a `format`
  key in the sidecar, else the file extension, else the row layout. Rows
  read with `row_stride`, black level subtracted per CFA
  position, scaled to the white level, white balance from
  `neutral_color_point`, colour through the camera's own
  `capture_color_transform` for the shot (or, without it, `forward_matrix2`
  or 1) into linear sRGB. Preview bins each 2×2 cell; export and 100 % demosaic with
  Malvar–He–Cutler. The RAW is turned to match its sibling JPEG and brought
  to the JPEG's brightness.
- **JPEG:** read upright (EXIF orientation), linearised.
- **Develop:** crop / straighten / perspective / rotate / flip as one
  sampling transform, zoomed just enough that no empty corners show,
  then temperature and tint, exposure, a tone curve on luminance (RAW gets a
  gentle base contrast and a highlight shoulder), a colour mixer (hue,
  saturation and lightness for eight hue bands, lightness kept when colour is
  taken away), saturation and vibrance,
  vignette, blur, sRGB, sharpening.
- **Export:** JPEG with the camera's EXIF block copied from the sibling JPEG,
  its Orientation set to 1.

Looks are starting points: a set of light, colour, detail and effects values
without the crop, applied with a strength from 0 to 150 %. Ten are built in;
your own are saved in `presets.json` in the app's data folder.

Effects include split toning (a hue and strength for shadows and highlights,
and a balance) and the triangle pattern Sailfish draws on its own
backgrounds, measured from a screenshot and repeated at a size relative to
the frame.

Photos are found in Pictures, Pictures/Camera and Pictures/RAWfish, and on a
memory card in the same folders and in DCIM.

Edits are stored in the app's own data folder (`recipes.json`), keyed by the
photo's path.

## Licence

MIT, Copyright (c) 2026 Caesar Prometheus Ivarsson.
