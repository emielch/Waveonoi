
class TimeScroller extends GUIElem {
    constructor(x, y, c, s) {
        super(x, y, s * 2.9, s * 3.8, c);

        this.size = s;
        this.hours = new NumberScroller(0, 0, c, 24, s);
        this.minutes = new NumberScroller(s * 1.6, 0, c, 60, s);


    }

    update() {
        if (this.hours.update()) this.needsRedraw = true;
        if (this.minutes.update()) this.needsRedraw = true;

        return this.needsRedraw;
    }

    setTime(h, m) {
        this.hours.val = h;
        this.minutes.val = m;
    }

    newTouch(touch) {
        touch.offsetX += this.x;
        touch.offsetY += this.y;
        if (this.hours.newTouch(touch)) return true;
        if (this.minutes.newTouch(touch)) return true;
        touch.offsetX -= this.x;
        touch.offsetY -= this.y;
        return false;
    }

    // rePos(x, y) {
    //     this.x = x;
    //     this.y = y;
    //     this.hours.rePos(x - this.size * 0.8, y);
    //     this.minutes.rePos(x + this.size * 0.8, y);
    // }

    redraw() {
        this.canvas.clear();
        this.canvas.fill(this.c);
        this.canvas.textSize(this.size);
        this.canvas.textAlign(CENTER, CENTER);
        this.canvas.noStroke();
        this.canvas.text(":", this.w / 2, this.h / 2 - this.size * 0.03);

        this.hours.draw(this.canvas);
        this.minutes.draw(this.canvas);

        this.needsRedraw = false;
    }
}



class NumberScroller extends GUIElem {
    constructor(x, y, c, _maxVal, s) {
        super(x, y, s * 1.3, s * 3.8, c);

        this.val = 0;
        this.maxVal = _maxVal;
        this.size = s;

        this.shift = 0;
        this.oldShift = 0;
        this.spd = 0;
    }

    update() {
        if (this.activeTouch) {
            if (!this.activeTouch.active) this.activeTouch = null;
            else this.spd = (this.activeTouch.y - this.activeTouch._y) / this.size;
        }

        this.shift += this.spd;

        this.spd *= 0.95;

        while (this.shift > 0.5) {
            this.shift -= 1;
            this.val = (this.val - 1 + this.maxVal) % this.maxVal;
            try { window.navigator.vibrate(5); }
            catch (err) { }
        } while (this.shift < -0.5) {
            this.shift += 1;
            this.val = (this.val + 1) % this.maxVal;
            try { window.navigator.vibrate(5); }
            catch (err) { }
        }
        if (abs(this.spd) < 0.007 && !this.activeTouch) {
            this.shift *= 0.9
        }

        if (abs(this.oldShift - this.shift) > 0.0005) {
            this.needsRedraw = true;
            this.oldShift = this.shift;
        }

        return this.needsRedraw;
    }

    redraw() {
        this.canvas.clear();

        this.canvas.textAlign(CENTER);



        this.canvas.fill(0, 20);
        this.canvas.noStroke();
        this.canvas.rectMode(CORNER);
        this.canvas.rect(0, 0, this.w, this.h);

        this.canvas.rectMode(CENTER);


        let startI = -2;
        let endI = 3;
        if (this.shift > 0.01) startI -= 1;
        else if (this.shift < -0.01) endI += 1;

        for (let i = startI; i < endI; i++) {
            if (i == 0) this.canvas.fill(this.c);
            else this.canvas.fill(70, 255 - abs(i) * 80 - ((i > 0) ? 1 : -1) * this.shift * 80 + ((i == 0) ? 1 : 0) * 80);
            let t = (this.val + this.maxVal + i) % this.maxVal;
            //print(t);

            let hString = String(t).padStart(2, '0');
            let currSize = this.size - abs(i + this.shift) * this.size * 0.15;
            this.canvas.textSize(currSize);
            let cWidth = textWidth(hString);

            this.canvas.translate(this.w / 2, this.h / 2 + sin((i + this.shift) * 0.524) * this.size * 1.8);
            this.canvas.scale(1, cos((i + this.shift) * 0.524));
            this.canvas.text(hString, 0, currSize * 0.35);
            //rect(0,0,this.size,cWidth);
            this.canvas.resetMatrix();
        }

        this.needsRedraw = false;
    }
}