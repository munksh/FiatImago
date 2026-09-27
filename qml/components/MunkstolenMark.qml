import QtQuick 2.0
import Sailfish.Silica 1.0
import ".."

// The Munkstolen mark: a choir stall, seen from the front.
//
// A munkstol is a misericord -- the small ledge under a hinged choir seat that
// lets a monk rest while appearing to stand through a long office. A good name
// for a maker's mark, and a good thing to have drawn rather than shipped.
//
// DRAWN, NOT SHIPPED. The mark is one colour, so it is a path and not an
// image. That means it takes the theme colour and is therefore correct in Fiat
// colours and in every ambience, light or dark, without a second asset and
// without anyone deciding what a dark mark does on dark paper. It is also
// sharp at any size, which a PNG is not.
//
// Canvas rather than QtQuick.Shapes: Shapes needs Qt 5.10 and Sailfish cannot
// be relied on to have it. Canvas is in every Qt 5.
//
// The path below was converted from the original SVG mechanically and checked
// by rebuilding an SVG from these very calls and comparing the two renderings
// pixel for pixel -- ten pixels differed, all of them antialiasing. Do not
// hand-edit it. Re-run tools/convert_mark.py if the logo ever changes.
//
// Coordinates are in the original 51.02 viewBox and scaled by k, so the whole
// thing follows `width`.

Canvas {
    id: root

    // The mark itself. Defaults to the app's primary text colour, which is what
    // makes it right in every mode without being told.
    property color color: FiatImagoTheme.primaryText

    // The roundel around it:
    //
    //   "none"  the mark alone, filling the item
    //   "ring"  the roundel drawn as an outline in the mark's own colour, so
    //           it belongs to the page rather than sitting on top of it
    //   "disc"  the roundel filled, for an unknown background
    //
    // "ring" is what the About page uses. The filled disc is a white plate,
    // and a white plate on cream paper reads as a shape rather than a mark.
    property string frame: "none"

    property color discColor: FiatImagoTheme.card

    // How much of the ring's diameter the mark itself takes. The original
    // artwork leaves a lot of air; at 80 px that air is what makes the chair
    // small, so the mark is scaled up inside the ring rather than kept at its
    // drawn proportion.
    property real markScale: 0.62

    // Fill the item with the MARK's own bounding box rather than the original
    // square, which is mostly empty around it. Only when there is no roundel.
    property bool trim: frame === "none"

    width: Theme.itemSizeSmall
    height: width
    renderStrategy: Canvas.Immediate

    // See ProgressRing: a Canvas loses its texture while the app is in the
    // background and nothing repaints it on the way back.
    Connections {
        target: Qt.application
        onStateChanged: {
            if (Qt.application.state === Qt.ApplicationActive) root.requestPaint()
        }
    }

    onVisibleChanged: {
        if (visible) requestPaint()
    }

    onColorChanged: requestPaint()
    onDiscColorChanged: requestPaint()
    onFrameChanged: requestPaint()
    onMarkScaleChanged: requestPaint()
    onTrimChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.reset()
        ctx.clearRect(0, 0, width, height)

        var side = Math.min(width, height)

        // The roundel first, in item coordinates, so it is exactly the size of
        // the item however the mark inside is scaled.
        if (root.frame === "disc") {
            ctx.fillStyle = root.discColor
            ctx.beginPath()
            ctx.arc(width / 2, height / 2, side / 2, 0, Math.PI * 2)
            ctx.fill()
        } else if (root.frame === "ring") {
            var lw = Math.max(1, side * 0.024)
            ctx.strokeStyle = root.color
            ctx.lineWidth = lw
            ctx.globalAlpha = 0.45
            ctx.beginPath()
            ctx.arc(width / 2, height / 2, side / 2 - lw / 2, 0, Math.PI * 2)
            ctx.stroke()
            ctx.globalAlpha = 1.0
        }

        // The mark occupies x 16.68..34.36, y 12.39..38.51 of the 51.02 box.
        var k, dx, dy
        if (root.trim) {
            k = Math.min(width / 17.68, height / 26.12)
        } else {
            // Sized against the ring rather than against the original square.
            k = (side * root.markScale) / 26.12
        }
        dx = (width - 17.68 * k) / 2 - 16.68 * k
        dy = (height - 26.12 * k) / 2 - 12.39 * k

        ctx.save()
        ctx.translate(dx, dy)

        ctx.fillStyle = root.color
        ctx.beginPath()
            ctx.moveTo(33.970 * k, 18.040 * k)
            ctx.bezierCurveTo(34.200 * k, 17.910 * k, 34.340 * k, 17.710 * k, 34.350 * k, 17.490 * k)
            ctx.bezierCurveTo(34.350 * k, 17.300 * k, 34.250 * k, 17.120 * k, 34.080 * k, 16.990 * k)
            ctx.bezierCurveTo(33.910 * k, 16.870 * k, 33.690 * k, 16.800 * k, 33.460 * k, 16.800 * k)
            ctx.bezierCurveTo(33.230 * k, 16.800 * k, 33.010 * k, 16.870 * k, 32.840 * k, 16.990 * k)
            ctx.bezierCurveTo(32.720 * k, 17.080 * k, 32.650 * k, 17.200 * k, 32.610 * k, 17.320 * k)
            ctx.lineTo(31.610 * k, 15.880 * k)
            ctx.lineTo(31.630 * k, 12.500 * k)
            ctx.bezierCurveTo(31.630 * k, 12.500 * k, 31.620 * k, 12.440 * k, 31.600 * k, 12.420 * k)
            ctx.bezierCurveTo(31.580 * k, 12.400 * k, 31.550 * k, 12.390 * k, 31.520 * k, 12.390 * k)
            ctx.lineTo(31.520 * k, 12.390 * k)
            ctx.lineTo(19.480 * k, 12.410 * k)
            ctx.bezierCurveTo(19.480 * k, 12.410 * k, 19.420 * k, 12.420 * k, 19.400 * k, 12.440 * k)
            ctx.bezierCurveTo(19.380 * k, 12.460 * k, 19.370 * k, 12.490 * k, 19.370 * k, 12.520 * k)
            ctx.lineTo(19.400 * k, 15.710 * k)
            ctx.lineTo(18.380 * k, 17.090 * k)
            ctx.bezierCurveTo(18.220 * k, 16.900 * k, 17.960 * k, 16.770 * k, 17.650 * k, 16.760 * k)
            ctx.bezierCurveTo(17.420 * k, 16.760 * k, 17.200 * k, 16.810 * k, 17.030 * k, 16.930 * k)
            ctx.bezierCurveTo(16.850 * k, 17.060 * k, 16.750 * k, 17.230 * k, 16.740 * k, 17.420 * k)
            ctx.bezierCurveTo(16.740 * k, 17.690 * k, 16.930 * k, 17.930 * k, 17.230 * k, 18.050 * k)
            ctx.lineTo(17.150 * k, 18.150 * k)
            ctx.lineTo(16.740 * k, 18.590 * k)
            ctx.bezierCurveTo(16.740 * k, 18.590 * k, 16.710 * k, 18.640 * k, 16.710 * k, 18.670 * k)
            ctx.lineTo(16.680 * k, 38.090 * k)
            ctx.bezierCurveTo(16.680 * k, 38.090 * k, 16.690 * k, 38.150 * k, 16.710 * k, 38.170 * k)
            ctx.bezierCurveTo(16.720 * k, 38.180 * k, 17.020 * k, 38.510 * k, 17.530 * k, 38.510 * k)
            ctx.bezierCurveTo(17.530 * k, 38.510 * k, 17.540 * k, 38.510 * k, 17.550 * k, 38.510 * k)
            ctx.bezierCurveTo(18.070 * k, 38.510 * k, 18.340 * k, 38.180 * k, 18.360 * k, 38.170 * k)
            ctx.bezierCurveTo(18.380 * k, 38.150 * k, 18.390 * k, 38.120 * k, 18.390 * k, 38.100 * k)
            ctx.lineTo(18.420 * k, 35.510 * k)
            ctx.bezierCurveTo(18.420 * k, 35.510 * k, 18.470 * k, 35.530 * k, 18.470 * k, 35.530 * k)
            ctx.bezierCurveTo(18.470 * k, 35.530 * k, 18.490 * k, 35.530 * k, 18.500 * k, 35.530 * k)
            ctx.lineTo(18.500 * k, 35.530 * k)
            ctx.lineTo(24.780 * k, 35.500 * k)
            ctx.lineTo(24.780 * k, 36.520 * k)
            ctx.bezierCurveTo(24.780 * k, 36.520 * k, 24.780 * k, 36.570 * k, 24.800 * k, 36.580 * k)
            ctx.bezierCurveTo(24.800 * k, 36.580 * k, 24.860 * k, 36.660 * k, 24.980 * k, 36.730 * k)
            ctx.bezierCurveTo(25.120 * k, 36.810 * k, 25.280 * k, 36.850 * k, 25.470 * k, 36.850 * k)
            ctx.bezierCurveTo(25.500 * k, 36.850 * k, 25.520 * k, 36.850 * k, 25.550 * k, 36.850 * k)
            ctx.bezierCurveTo(26.070 * k, 36.820 * k, 26.260 * k, 36.590 * k, 26.280 * k, 36.570 * k)
            ctx.bezierCurveTo(26.290 * k, 36.550 * k, 26.300 * k, 36.530 * k, 26.300 * k, 36.500 * k)
            ctx.lineTo(26.300 * k, 35.500 * k)
            ctx.bezierCurveTo(26.300 * k, 35.500 * k, 32.360 * k, 35.470 * k, 32.360 * k, 35.470 * k)
            ctx.lineTo(32.330 * k, 38.100 * k)
            ctx.bezierCurveTo(32.330 * k, 38.100 * k, 32.330 * k, 38.150 * k, 32.350 * k, 38.170 * k)
            ctx.bezierCurveTo(32.360 * k, 38.180 * k, 32.600 * k, 38.500 * k, 33.130 * k, 38.500 * k)
            ctx.bezierCurveTo(33.150 * k, 38.500 * k, 33.160 * k, 38.500 * k, 33.180 * k, 38.500 * k)
            ctx.bezierCurveTo(33.740 * k, 38.480 * k, 33.940 * k, 38.190 * k, 33.950 * k, 38.180 * k)
            ctx.bezierCurveTo(33.960 * k, 38.160 * k, 33.970 * k, 38.140 * k, 33.970 * k, 38.120 * k)
            ctx.lineTo(34.360 * k, 18.620 * k)
            ctx.bezierCurveTo(34.360 * k, 18.620 * k, 34.360 * k, 18.570 * k, 34.340 * k, 18.550 * k)
            ctx.lineTo(33.990 * k, 18.070 * k)
            ctx.closePath()
            ctx.moveTo(24.730 * k, 16.100 * k)
            ctx.lineTo(24.730 * k, 20.110 * k)
            ctx.bezierCurveTo(24.580 * k, 20.080 * k, 24.430 * k, 20.020 * k, 24.290 * k, 19.920 * k)
            ctx.bezierCurveTo(23.620 * k, 19.430 * k, 23.310 * k, 18.980 * k, 23.330 * k, 18.550 * k)
            ctx.bezierCurveTo(23.340 * k, 18.300 * k, 23.410 * k, 18.160 * k, 23.460 * k, 18.060 * k)
            ctx.bezierCurveTo(23.510 * k, 17.960 * k, 23.570 * k, 17.830 * k, 23.420 * k, 17.720 * k)
            ctx.bezierCurveTo(23.390 * k, 17.700 * k, 23.340 * k, 17.670 * k, 23.280 * k, 17.640 * k)
            ctx.bezierCurveTo(23.060 * k, 17.520 * k, 22.700 * k, 17.330 * k, 22.550 * k, 16.990 * k)
            ctx.bezierCurveTo(22.400 * k, 16.670 * k, 22.390 * k, 16.280 * k, 22.390 * k, 16.100 * k)
            ctx.lineTo(24.730 * k, 16.100 * k)
            ctx.closePath()
            ctx.moveTo(26.910 * k, 16.100 * k)
            ctx.lineTo(28.680 * k, 16.100 * k)
            ctx.bezierCurveTo(28.680 * k, 16.100 * k, 28.680 * k, 16.200 * k, 28.660 * k, 16.310 * k)
            ctx.bezierCurveTo(28.610 * k, 16.690 * k, 28.530 * k, 16.880 * k, 28.130 * k, 17.340 * k)
            ctx.bezierCurveTo(27.880 * k, 17.620 * k, 27.770 * k, 17.650 * k, 27.750 * k, 17.660 * k)
            ctx.bezierCurveTo(27.720 * k, 17.660 * k, 27.680 * k, 17.660 * k, 27.650 * k, 17.680 * k)
            ctx.bezierCurveTo(27.610 * k, 17.710 * k, 27.600 * k, 17.760 * k, 27.610 * k, 17.800 * k)
            ctx.bezierCurveTo(27.810 * k, 18.510 * k, 27.720 * k, 18.930 * k, 27.240 * k, 19.530 * k)
            ctx.bezierCurveTo(26.930 * k, 19.920 * k, 26.590 * k, 20.000 * k, 26.390 * k, 20.020 * k)
            ctx.lineTo(26.410 * k, 16.100 * k)
            ctx.lineTo(26.920 * k, 16.100 * k)
            ctx.closePath()
            ctx.moveTo(18.610 * k, 19.190 * k)
            ctx.lineTo(20.710 * k, 16.100 * k)
            ctx.lineTo(21.000 * k, 16.100 * k)
            ctx.bezierCurveTo(21.030 * k, 16.310 * k, 21.120 * k, 16.800 * k, 21.260 * k, 17.380 * k)
            ctx.bezierCurveTo(21.510 * k, 18.400 * k, 21.820 * k, 19.170 * k, 22.180 * k, 19.680 * k)
            ctx.bezierCurveTo(22.590 * k, 20.270 * k, 23.030 * k, 20.730 * k, 23.470 * k, 21.030 * k)
            ctx.bezierCurveTo(23.880 * k, 21.320 * k, 24.300 * k, 21.470 * k, 24.740 * k, 21.520 * k)
            ctx.lineTo(24.740 * k, 23.470 * k)
            ctx.bezierCurveTo(24.740 * k, 23.470 * k, 18.530 * k, 25.220 * k, 18.530 * k, 25.220 * k)
            ctx.lineTo(18.600 * k, 19.190 * k)
            ctx.closePath()
            ctx.moveTo(26.320 * k, 31.460 * k)
            ctx.lineTo(26.350 * k, 26.780 * k)
            ctx.bezierCurveTo(27.360 * k, 26.800 * k, 28.770 * k, 26.820 * k, 30.000 * k, 26.820 * k)
            ctx.bezierCurveTo(30.020 * k, 26.820 * k, 30.040 * k, 26.820 * k, 30.060 * k, 26.820 * k)
            ctx.bezierCurveTo(31.610 * k, 26.820 * k, 32.210 * k, 26.780 * k, 32.470 * k, 26.740 * k)
            ctx.lineTo(32.380 * k, 33.930 * k)
            ctx.lineTo(26.330 * k, 31.450 * k)
            ctx.closePath()
            ctx.moveTo(30.170 * k, 16.100 * k)
            ctx.lineTo(32.520 * k, 19.580 * k)
            ctx.bezierCurveTo(32.520 * k, 19.580 * k, 32.530 * k, 19.600 * k, 32.540 * k, 19.600 * k)
            ctx.lineTo(32.470 * k, 25.170 * k)
            ctx.lineTo(26.360 * k, 23.440 * k)
            ctx.lineTo(26.360 * k, 21.480 * k)
            ctx.bezierCurveTo(27.510 * k, 21.370 * k, 28.060 * k, 20.700 * k, 28.880 * k, 19.680 * k)
            ctx.lineTo(28.880 * k, 19.680 * k)
            ctx.bezierCurveTo(29.250 * k, 19.220 * k, 29.570 * k, 18.460 * k, 29.830 * k, 17.420 * k)
            ctx.bezierCurveTo(29.980 * k, 16.820 * k, 30.070 * k, 16.290 * k, 30.100 * k, 16.090 * k)
            ctx.lineTo(30.160 * k, 16.090 * k)
            ctx.closePath()
            ctx.moveTo(18.510 * k, 26.710 * k)
            ctx.lineTo(24.750 * k, 26.750 * k)
            ctx.lineTo(24.750 * k, 31.490 * k)
            ctx.bezierCurveTo(24.750 * k, 31.490 * k, 18.430 * k, 33.960 * k, 18.430 * k, 33.960 * k)
            ctx.lineTo(18.520 * k, 26.710 * k)
            ctx.closePath()
            ctx.moveTo(24.760 * k, 32.560 * k)
            ctx.lineTo(24.760 * k, 34.430 * k)
            ctx.bezierCurveTo(24.760 * k, 34.430 * k, 20.060 * k, 34.430 * k, 20.060 * k, 34.430 * k)
            ctx.lineTo(24.760 * k, 32.560 * k)
            ctx.closePath()
            ctx.moveTo(26.310 * k, 32.600 * k)
            ctx.bezierCurveTo(27.140 * k, 32.940 * k, 29.240 * k, 33.800 * k, 30.740 * k, 34.420 * k)
            ctx.lineTo(26.300 * k, 34.420 * k)
            ctx.bezierCurveTo(26.300 * k, 34.420 * k, 26.310 * k, 32.600 * k, 26.310 * k, 32.600 * k)
            ctx.closePath()
        ctx.fill()
        ctx.restore()
    }
}
