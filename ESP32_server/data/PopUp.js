class PopUp extends GUIElem {

  constructor(parent, guiElem) {
    super(50, 50, width - 100, height - 100, color(150, 100));
    this.button = new Button(this.w / 2, this.h / 2, 200, 70, color(120), "OK", 60, this);
    this.guiElem = guiElem;

    this.parent = parent;
  }


  update() {
    if (this.button.update()) this.needsRedraw = true;
    if (this.guiElem.update()) this.needsRedraw = true;

    return super.update();
  }

  newTouch(touch) {
    touch.offsetX += this.x;
    touch.offsetY += this.y;
    if (this.guiElem.newTouch(touch)) return true;
    if (this.button.newTouch(touch)) return true;
    touch.offsetX -= this.x;
    touch.offsetY -= this.y;
    return false;
  }


  callback(){
    this.parent.popUpCallback(this);
  }

  redraw() {
    this.canvas.clear();
    this.canvas.fill(this.c);
    this.canvas.noStroke();
    this.canvas.rect(0, 0, this.w, this.h);

    this.guiElem.draw(this.canvas);
    this.button.draw(this.canvas);

    this.needsRedraw = false;
  }

}

