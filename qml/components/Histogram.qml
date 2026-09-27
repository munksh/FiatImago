import QtQuick 2.0

// The one place imago shows colour of its own. The channels add like light,
// as in the launcher icon: white where all three overlap, colour at the edges.
Canvas {
    id: canvas

    property var bins: []

    onBinsChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    onPaint: {
        var ctx = getContext("2d")
        ctx.globalCompositeOperation = "source-over"
        ctx.clearRect(0, 0, width, height)
        if (!bins || bins.length !== 3) return
        var colours = ["rgb(215, 60, 55)", "rgb(60, 185, 80)", "rgb(60, 105, 230)"]
        ctx.globalCompositeOperation = "lighter"
        for (var c = 0; c < 3; ++c) {
            var values = bins[c]
            if (!values || values.length < 2) continue
            var n = values.length
            ctx.beginPath()
            ctx.moveTo(0, height)
            for (var i = 0; i < n; ++i)
                ctx.lineTo(i * width / (n - 1), height - values[i] * height)
            ctx.lineTo(width, height)
            ctx.closePath()
            ctx.fillStyle = colours[c]
            ctx.fill()
        }
        ctx.globalCompositeOperation = "source-over"
    }
}
