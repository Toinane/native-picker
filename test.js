const picker = require('./index');
const { EventEmitter } = require('events');

console.log(picker)

const emitter = new EventEmitter();

emitter.on('start', () => console.log('picker started'));

emitter.on('update', event => console.log(event));

emitter.on('end', () => console.log('Picker ended'));

picker.init(emitter.emit.bind(emitter), {
    previousColor: '#112233',
    gridSize: 3
});