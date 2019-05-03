class GUIElem {
    constructor(x, y, w, h, c) {
        this.x = x;
        this.y = y;
        this.w = w;
        this.h = h;
        this.c = c;

        this.activeTouch = null;

        this.canvas = createGraphics(this.w, this.h);
        this.needsRedraw = true;
    }



    update() {
        // update values, like speed, shift 
        // also update GUIelem-children
        // and if changed set this.needsRedraw = true

        // for (let i = 0; i < GUIElems.length; i++) {
        //     if(GUIElems[i].update()) this.needsRedraw = true;
        // }

        if (this.activeTouch) {
            if (!this.activeTouch.active){
                if(this.activeTouch.clicked) this.click();
                this.activeTouch = null;
            }
        }

        return this.needsRedraw;
    }

    redraw() {
        this.canvas.clear();
        this.canvas.fill(this.c);
        this.canvas.rect(0, 0, this.w, this.h);

        // for (let i = 0; i < GUIElems.length; i++) {
        //     GUIElems[i].draw(this.canvas);
        // }

        this.needsRedraw = false;
    }

    draw(outCanvas) {
        if (this.needsRedraw) {
            this.redraw();
        }
        outCanvas.image(this.canvas, this.x, this.y);
    }


    inRect(x, y) {
        if (x > this.x && x < this.x + this.w && y > this.y && y < this.y + this.h) return true;
        return false;
    }

    newTouch(touch) {
        if (this.inRect(touch.x-touch.offsetX, touch.y-touch.offsetY)) {
            this.activeTouch = touch;
            return true;
        }
        return false;
    }

    click(){
        print("click");
    }

    rePos(x,y) {
        this.x = x;
        this.y = y;
        this.needsRedraw = true;
    }

    reSize(w,h){
        this.w = w;
        this.h = h;
        this.canvas = createGraphics(this.w, this.h);
        this.needsRedraw = true;
    }

}