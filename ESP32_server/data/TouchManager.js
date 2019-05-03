// ------   TOUCH / MOUSE INPUT ----------

function touchStarted(event) {
    if (event.type != "touchstart") return false;
    for (var i = 0; i < event.changedTouches.length; i++) {
        let touch = event.changedTouches[i];
        touchManager.start(touch.identifier, touch.pageX, touch.pageY);
    }
}


function touchMoved(event) {
    // prevent default
    return false;
}

function touchEnded(event) {
    if (event.type != "touchend") return;
    for (var i = 0; i < event.changedTouches.length; i++) {
        let touch = event.changedTouches[i];
        touchManager.end(touch.identifier, touch.pageX, touch.pageY);
    }
}


function mousePressed(event) {
    if (event.type != "mousedown") return;
    touchManager.start("mouse", event.pageX, event.pageY);


    if (mouseButton === CENTER) {
        let fs = fullscreen();
        fullscreen(!fs);
    }
}

function mouseReleased(event) {
    if (event.type != "mouseup") return;
    touchManager.end("mouse", event.pageX, event.pageY);
}


class TouchManager {
    constructor() {
        this.touches = {};
    }

    update() {
        for (let i = 0; i < touches.length; i++) {
            let touch = touches[i];
            this.move(touch.id, touch.x, touch.y);
        }
        if (this.touches["mouse"] && this.touches["mouse"].active) this.move("mouse", mouseX, mouseY);
    }

    start(id, x, y) {
        this.touches[id] = { active: true, x: x, y: y, startX: x, startY: y, startTime: millis(), _x: x, _y: y, offsetX: 0, offsetY: 0, clicked: false };
        mainGUI.newTouch(this.touches[id]);
    }

    move(id, x, y) {
        let touch = this.touches[id];
        touch._x = touch.x;
        touch._y = touch.y;
        touch.x = x;
        touch.y = y;
    }

    end(id, x, y) {
        let touch = this.touches[id];
        touch.active = false;
        let tDiff = millis() - touch.startTime;
        let distance = dist(touch.x, touch.y, touch.startX, touch.startY);
        if (tDiff < 1000 && distance < 5) touch.clicked = true;
        delete this.touches[id];
    }

}


