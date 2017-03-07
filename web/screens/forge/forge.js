$(window).load(function ()
{
    var keyStates = {};
    var selectedObject = -1;

    $(document).keydown(function (e) { keyStates[e.keyCode] = true; });
    $(document).keyup(function (e) { delete keyStates[e.keyCode];  });

    $('#close').on('click', function () {
        dew.hide();
    });

    $(document.body).click(function (e) {
        if (($(e.target).is('body') || $(e.target).is('html'))) {
            // return input back to the game if we click outside the window
            dew.callMethod("captureInput", { capture: false });
            return false;
        }
    });

    $('.window .title-bar').on('mousedown', function (e) {
        var _this = $(this).parent();

        var pos = _this.position();
        var downOffset = { x: e.clientX - pos.left, y: e.clientY - pos.top };

        function moveHandler(e) {
            $(_this).css({
                left: e.clientX - downOffset.x,
                top: e.clientY - downOffset.y
            })
        }

        $(document).on('mousemove', moveHandler);
        $(document).on('mouseup', function upHandler() {
            $(document).off('mousemove', moveHandler);
            $(document).off('mosueup', upHandler);
        });
    });

    $('.transform .input-float').on('mousewheel', function (e) {
        var e = window.event || e;
        var delta = Math.max(-1, Math.min(1, (e.wheelDelta || -e.detail)));

        var step = 0.1 * delta;

        // TODO: remove hardcoding

        if (keyStates[16]) // shift
            step *= 4;

        if (keyStates[17]) // ctrl
            step *= 0.1;

        if($(this).hasClass('angle'))
            step *= 10;

        var val = parseFloat($(this).val());
        val += step;

        $(this).val(val.toFixed(4));

        updateTransform();
    });

    $('.transform .input-float').on('change', function (e) {
        updateTransform();
    });

    dew.on("WebForge::SelectObject", function (e) {
        selectedObject = e.data.ObjectIndex;

        $('#PositionX').val(e.data.PositionX.toFixed(4));
        $('#PositionY').val(e.data.PositionY.toFixed(4));
        $('#PositionZ').val(e.data.PositionZ.toFixed(4));
        $('#RotationX').val(e.data.RotationX.toFixed(4));
        $('#RotationY').val(e.data.RotationY.toFixed(4));
        $('#RotationZ').val(e.data.RotationZ.toFixed(4));
    });

    function updateTransform() {
        if (selectedObject == -1)
            return;

        var payload = {
            ObjectIndex: selectedObject,
            PositionX: parseFloat($('#PositionX').val()),
            PositionY: parseFloat($('#PositionY').val()),
            PositionZ: parseFloat($('#PositionZ').val()),
            RotationX: parseFloat($('#RotationX').val()),
            RotationY: parseFloat($('#RotationY').val()),
            RotationZ: parseFloat($('#RotationZ').val()),
        };

        dew.forgeCommand('updateTransform', payload);
    }
});