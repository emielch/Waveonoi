
class Button extends GUIElem {
    constructor(x, y, w, h, c, txt, s, parent) {
        super(x, y, w, h, c);
        this.txt = txt;
        this.s = s;
        this.parent = parent;
    }

    click(){
        this.parent.callback();
    }

    redraw() {
        this.canvas.clear();

        this.canvas.fill(this.c);
        this.canvas.rectMode(CORNER);
        this.canvas.rect(0, 0, this.w, this.h);
        this.canvas.textAlign(CENTER, CENTER);
        this.canvas.fill(0);
        this.canvas.textSize(this.s);
        this.canvas.text(this.txt, this.w / 2, this.h / 2);

        this.needsRedraw = false;
    }

}

