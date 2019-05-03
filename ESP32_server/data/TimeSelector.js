class TimeSelector extends GUIElem {
    constructor(x, y, w, h, c, size) {

        super(x, y, w, h, c);

        this.hour = 0;
        this.min = 0;

        this.size = size;
    }

    click(){
        let ts = new TimeScroller(200, 200, color(50, 100, 220), 80);
        ts.setTime(this.hour,this.min);
        mainGUI.setPopUp(new PopUp(this, ts));
    }

    popUpCallback(popUp) {
        this.hour = popUp.guiElem.hours.val;
        this.min = popUp.guiElem.minutes.val;
        mainGUI.clearPopUp();
        this.needsRedraw = true;
    }

    redraw() {
        this.canvas.clear();

        let timeTxt = String(this.hour).padStart(2, '0') + ":" + String(this.min).padStart(2, '0');

        this.canvas.noStroke();
        this.canvas.rectMode(CORNER);
        this.canvas.fill(255);
        this.canvas.rect(0, 0, this.w, this.h);
        this.canvas.textSize(this.size);
        this.canvas.textAlign(LEFT, TOP);

        this.canvas.fill(this.c);
        this.canvas.text(timeTxt, 0, 0);

        this.needsRedraw = false;
    }

}