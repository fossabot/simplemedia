'use strict'

const {Window} = require('bindings')('simplemedia');

// const test_media_uri='https://file-examples.com/wp-content/uploads/2017/11/file_example_MP3_700KB.mp3';
// const test_media_uri='/Users/buttonfly/Music/Aru_Hareta_Hi_Ni.mp3';
const test_media_uri='/Users/buttonfly/Movies/ace.mp4';

function waitEvent() {
  window.waitEvent();
  setTimeout(()=>{
    waitEvent();
  });
}

let window = new Window();
let renderer = window.createRenderer();

const MediaPlayer = require('../lib/media_player.js');
let player = new MediaPlayer(renderer);
player.datasource = test_media_uri;
player.prepare().then(resolve => {
  console.log('prepared');
  player.start();
}).catch(err => {
  console.log(err);
});

player.onend = (() => {
  console.log('end-of-stream!');
});

console.log('Press any key to exit');
process.stdin.setRawMode(true);
process.stdin.resume();
process.stdin.on('data', process.exit.bind(process, 0));

waitEvent();
console.log('end!');
