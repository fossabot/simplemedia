'use strict'

const {Source, AudioDecoder, AudioRenderer} = require('simplemedia');

const test_media_uri='https://file-examples.com/wp-content/uploads/2017/11/file_example_MP3_700KB.mp3';

var decoder = null;
var channels = 0;

function dump(frame) {
  console.log(frame.data);
}

function decode() {
  decoder.decode(frame => {
  var delay = 0;
  if(frame) {
    dump(frame);
    setTimeout(decode);
  }
  else {
    console.log('null packet');
    console.log('count: ' + this.count);
  }
  });	
}

var source = new Source();
source.datasource = test_media_uri;
let fmt = source.prepare();
if(!fmt) {
  console.log('failed to prepare the source');
  process.exit();	
}

console.log(fmt);

if(! source.hasAudio) {
  console.log('failed to prepare the source');
  process.exit();	
}

let pid = source.audioPid;
console.log('pid: ' + pid);
let pidchannel = source.requestPidChannel({
  pid : pid,
});

decoder = new AudioDecoder();
decoder.prepare(fmt.streams[pid]['native']);
decoder.pidchannel = pidchannel;
channels = fmt.streams[pid]['channels'];

source.start();
setTimeout(decode);

console.log('Press any key to exit');
process.stdin.setRawMode(true);
process.stdin.resume();
process.stdin.on('data', process.exit.bind(process, 0));

console.log('end!');