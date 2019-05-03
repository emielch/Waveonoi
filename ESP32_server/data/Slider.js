
class Slider extends GUIElem {
    constructor(x, y, w, h, c) {
        super(x, y, w, h, c);
        this.val = 0;
        this.scrollX = 5;
        this.scrollW = w - this.scrollX * 2;
    }



    update() {
        if (this.activeTouch) {
            if (!this.activeTouch.active) this.activeTouch = null;
            else {
                this.val += (this.activeTouch.x - this.activeTouch._x) / this.w;
                this.val = constrain(this.val, 0, 1);

                this.needsRedraw = true;
            }
        }

        return this.needsRedraw;
    }


    redraw() {
        this.canvas.clear();

        this.canvas.stroke(1);

        this.canvas.fill(this.c);
        this.canvas.rectMode(CORNER);
        this.canvas.rect(this.scrollX, 5, this.scrollW, this.h - 10);
        this.canvas.rectMode(CENTER);
        this.canvas.rect(this.val * this.scrollW + this.scrollX, this.h / 2, 10, this.h-2);
        //this.canvas.rectMode(CORNER);

        this.needsRedraw = false;
    }

}

