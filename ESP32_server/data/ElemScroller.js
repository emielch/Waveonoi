class ElemScroller extends GUIElem {
    constructor(x, y, w, h, c) {
        super(x, y, w, h, c);

        this.shift = 0;
        this.oldShift = 0;
        this.spd = 0;

        this.GUIElems = [];
        for (let i = 0; i < 20; i++) {
            let objH = 80;
            this.GUIElems.push(new GUIElem(10, 10 + (objH + 10) * i, this.w - 20, objH, color(130)));
        }
        let lastobj = this.GUIElems[this.GUIElems.length - 1];
        this.shiftCanvas = createGraphics(this.w, lastobj.y+lastobj.h+10);
    }


    update() {
        if (this.activeTouch) {
            if (this.activeTouch.active)
                this.spd = this.activeTouch.y - this.activeTouch._y;
        }

        let lastobj = this.GUIElems[this.GUIElems.length - 1];
        let endBounce = -max(lastobj.y + lastobj.h - this.h, 0);

        if (this.shift > 0) {
            this.spd *= max(0.7 - this.shift * 0.003, 0);
            if (!this.activeTouch && this.spd < 0.01) {
                this.spd = 0;
                this.shift *= 0.9;
            }
        }

        if (this.shift < endBounce) {
            this.spd *= max(0.7 - (endBounce - this.shift) * 0.003, 0);
            if (!this.activeTouch && this.spd > -0.01) {
                this.spd = 0;
                this.shift -= (this.shift - endBounce) * 0.1;
            }
        }

        this.shift += this.spd;
        this.spd *= 0.9;


        if (abs(this.oldShift - this.shift) > 0.1) {
            this.needsRedraw = true;
            this.oldShift = this.shift;
        }

        return super.update();
    }

    redraw() {
        this.shiftCanvas.clear();

        for (let i = 0; i < this.GUIElems.length; i++) {
            //if (this.GUIElems[i].y + this.shift < -this.GUIElems[i].h || this.GUIElems[i].y + this.shift > this.h) continue;
            this.GUIElems[i].draw(this.shiftCanvas);
        }

        this.canvas.clear();
        this.canvas.noStroke();
        this.canvas.background(this.c);
        this.canvas.image(this.shiftCanvas,0,this.shift);


        this.needsRedraw = false;
    }

}
