'use strict'

const {Source, AudioDecoder, AudioRenderer, VideoRenderer, VideoDecoder} = require('bindings')('simplemedia');
const adjust = 3;
module.exports = class MediaPlayer {
  constructor() {
    this.source = new Source();
    this.audio = {};
    this.video = {};
    
    this.audio.renderer = new AudioRenderer();
    this.video.renderer = new VideoRenderer();
    
    this.count=0;

    this.source.trace = true;
    this.audio.renderer.trace = true;

    this.isFirstFrame = true;
    this.pts = 0;
  }

  async prepare() {
    let source = this.source;
    return new Promise((resolve, reject) => {
      let fmt = source.prepare();
      if(fmt) {
        console.log(fmt);
        if(source.hasAudio) {
          let pid = source.audioPid;
          console.log('pid: ' + pid);
          let pidchannel = source.requestPidChannel({
            pid : pid,
          });
      
          this.audio.decoder = new AudioDecoder();
          this.audio.decoder.prepare(fmt.streams[pid]['native']);
          this.audio.decoder.pidchannel = pidchannel;
          this.audio.decoder.trace = true;
      
          this.audio.renderer.prepare({
            samplerate: this.audio.decoder.samplerate,
            channels: this.audio.decoder.channels,
            channellayout: this.audio.decoder.channellayout,
            sampleformat: this.audio.decoder.sampleformat,
          });
        }

        if(source.hasVideo) {
          let pid = source.videoPid;
          console.log('pid: ' + pid);
          let pidchannel = source.requestPidChannel({
            pid : pid,
          });
      
          this.video.decoder = new VideoDecoder();
          this.video.decoder.prepare(fmt.streams[pid]['native']);
          this.video.decoder.pidchannel = pidchannel;
          this.video.decoder.trace = true;
      
          this.video.renderer.prepare({
            width: this.video.decoder.width,
            height: this.video.decoder.height,
            pixelformat: this.video.decoder.pixelformat,
          });
        }
        resolve(fmt);
      }
      else {
        reject();
      }
    });  
  }

  _decodeAudio() {
    this.audio.decoder.decode(frame => {
      var delay = 0;
      if(frame) {
        if(this.isFirstFrame) {
          this.pts = frame.pts;
          this.isFirstFrame = false;
        }
        else {
          delay = frame.pts - this.pts;
          this.pts = frame.pts;
        }

        this.audio.renderer.render(frame.data);
        this.count++;
       
        setTimeout(()=>{
          this._decodeAudio();
        }, (delay / 1000) - adjust);
      }
      else {
        console.log('null packet');
        console.log('count: ' + this.count);
        setTimeout(()=>{
          if(this.onend)
            this.onend();
        });
      }
    });
  }

  _decodeVideo() {
    this.video.decoder.decode(frame => {
      var delay = 0;
      if(frame) {
        if(this.isFirstFrame) {
          this.pts = frame.pts;
          this.isFirstFrame = false;
        }
        else {
          delay = frame.pts - this.pts;
          this.pts = frame.pts;
        }

        this.video.renderer.render(frame.data);
        this.count++;
       
        setTimeout(()=>{
          this._decodeVideo();
        }, (delay / 1000) - adjust);
      }
      else {
        console.log('null packet');
        console.log('count: ' + this.count);
        // setTimeout(()=>{
        //   if(this.onend)
        //     this.onend();
        // });
      }
    });
  }
  /**
   * @param {() => void} onend
   */
  set onended(onend) {
    this.onend = onend;
  }

  start() {
    this.source.start();
    setTimeout(()=>{
      this._decodeAudio();
    });

    setTimeout(()=>{
      this._decodeVideo();
    });    
  }

  stop() {
    this.audio.decoder.stop();
    this.source.stop();
  }

  pause() {
    this.source.pause();
    this.audio.decoder.pause();
  }

  resume() {
    this.start();  
  }


  /**
   * @param {string} datasource
   */
  set datasource(datasource) {
    this.source.datasource = datasource;
  }
}