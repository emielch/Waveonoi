class MainGUI extends GUIElem {
    constructor(x, y, w, h, c) {
        super(x, y, w, h, c);

        this.GUIElems = [];
        let popUp = null;

        this.GUIElems.push(new TimeSelector(10, 600, 210, 80, color(72, 151, 214), 80));
        this.GUIElems.push(new Slider(50, 100, 400, 30, color(150, 150, 150)));
        this.GUIElems.push(new ElemScroller(500, 10, 300, 900, color(180)));
    }


    update() {
        for (let i = 0; i < this.GUIElems.length; i++) {
            if (this.GUIElems[i].update()) this.needsRedraw = true;
        }

        if (this.popUp)
            if (this.popUp.update()) this.needsRedraw = true;

        if (this.activeTouch) {
            if (!this.activeTouch.active) this.activeTouch = null;
        }

        return this.needsRedraw;
    }


    redraw() {
        print("draw Main");
        this.canvas.clear();
        this.canvas.background(this.c);

        for (let i = 0; i < this.GUIElems.length; i++) {
            this.GUIElems[i].draw(this.canvas);
        }

        if (this.popUp) this.popUp.draw(this.canvas);

        this.needsRedraw = false;
    }


    newTouch(touch) {
        if (this.popUp) {
            if (this.popUp.newTouch(touch)) return true;
        }
        else for (let i = 0; i < this.GUIElems.length; i++) {
            if (this.GUIElems[i].newTouch(touch)) return true;
        }
        return false;
    }

    setPopUp(popUp) {
        this.popUp = popUp;
        this.needsRedraw = true;
    }

    clearPopUp() {
        this.popUp = null;
        this.needsRedraw = true;
    }

}