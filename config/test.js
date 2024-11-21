const chalk = require('chalk');

const picker = require('../build/Release/picker.node');

console.log(picker);

console.log(picker.getMousePos())

while (true) {
  const color = '#' + picker.getPixelColor();
  console.log(chalk.hex(color)(color));
}