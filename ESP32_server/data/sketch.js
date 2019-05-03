let timePicker;

let touchManager;

let mainGUI;

function setup() {
  createCanvas(windowWidth, windowHeight);
  pixelDensity(displayDensity());

  mainGUI = new MainGUI(0, 0, width, height, color(240, 240, 250));

  touchManager = new TouchManager();
}

function draw() {
  touchManager.update();

  if(mainGUI.update())  mainGUI.draw(this);

  rect(0,0,100,10);
  text(getFrameRate(),10,10);
}

function windowResized() {
  resizeCanvas(windowWidth, windowHeight);
  mainGUI.reSize(windowWidth,windowHeight)
}




// let im;

// function setup() {
//     createCanvas(windowWidth, windowHeight);
//     pixelDensity(displayDensity());
  
//     im = createGraphics(300, 200);
//     im.background(50);
//   }
  
//   function draw() {
   
//     background(200);
    
//     for(i = 0; i<1000; i++){
//     image(im,mouseX,mouseY);
//   }
// }