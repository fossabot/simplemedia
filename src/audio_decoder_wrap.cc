#include "audio_decoder_wrap.h"
#include <napi.h>
#include <uv.h>
#include "log_message.h"
#include <future>
#include "frame_wrap.h"

Napi::FunctionReference AudioDecoder::constructor;

Napi::Object AudioDecoder::Init(Napi::Env env, Napi::Object exports) {
  Napi::HandleScope scope(env);

  Napi::Function func =
      DefineClass(env,
                  "AudioDecoder",
                  {
                    InstanceMethod("prepare", &AudioDecoder::Prepare),
                    InstanceMethod("start", &AudioDecoder::Start),
                    InstanceMethod("stop", &AudioDecoder::Stop),
                    InstanceMethod("pause", &AudioDecoder::Pause),
                    InstanceMethod("decode", &AudioDecoder::Decode),
                    InstanceMethod("flush", &AudioDecoder::Flush),
                    InstanceAccessor("pidchannel", nullptr, &AudioDecoder::SetPidChannel),
                    InstanceAccessor("sampleformat", &AudioDecoder::sampleformat, nullptr),
                    InstanceAccessor("samplerate", &AudioDecoder::samplerate, nullptr),
                    InstanceAccessor("channels", &AudioDecoder::channels, nullptr),
                    InstanceAccessor("channellayout", &AudioDecoder::channellayout, nullptr),
                    InstanceAccessor("trace", &AudioDecoder::log_enabled, &AudioDecoder::EnableLog),
                  });

  constructor = Napi::Persistent(func);
  constructor.SuppressDestruct();

  exports.Set("AudioDecoder", func);

  return exports;
}

AudioDecoder::AudioDecoder(const Napi::CallbackInfo& info) : Napi::ObjectWrap<AudioDecoder>(info) {
  Napi::Env env = info.Env();
  Napi::HandleScope scope(env);

  decoder_.reset(new gurum::AudioDecoder);
}


void AudioDecoder::SetPidChannel(const Napi::CallbackInfo& info, const Napi::Value &value) {
  Napi::Env env = info.Env();
  if (info.Length() <= 0 || !info[0].IsExternal()) {
    Napi::TypeError::New(env, "External expected").ThrowAsJavaScriptException();
    return;
  }

  auto external = value.As<Napi::External<gurum::PidChannel>>();
  auto pidchannel = external.Data();
  if(log_enabled_) LOG(INFO) << __func__ << " pidchannel: " << pidchannel;
  assert(pidchannel);
  assert(decoder_);

  decoder_->SetPidChannel(pidchannel);
}

void AudioDecoder::Prepare(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (info.Length() <= 0 || !info[0].IsExternal()) {
    Napi::TypeError::New(env, "External expected").ThrowAsJavaScriptException();
    return;
  }

  auto external = info[0].As<Napi::External<AVStream>>();
  auto strm = external.Data();
  assert(strm);
  
  if(log_enabled_) LOG(INFO) << __func__ << " strm: "<< strm;

  assert(decoder_);
  if(decoder_) {
    int err; 
    err = decoder_->Prepare(gurum::CodecParam(strm));
    if(err) {
      LOG(ERROR) << " failed to prepare the audio decoder";
      Napi::TypeError::New(env, "prepare exception").ThrowAsJavaScriptException();
      return;
    }
  }
}

void AudioDecoder::Start(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  assert(decoder_);
  if(decoder_) {
    int err; 
    err = decoder_->Start();
    if(err) {
      LOG(ERROR) << " failed to start the audio decoder";
      Napi::TypeError::New(env, "exception while starting a decoder").ThrowAsJavaScriptException();
      return;
    }
  }
}

void AudioDecoder::Stop(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  assert(decoder_);
  if(decoder_) {
    int err;
    err = decoder_->Stop();
    if(err) {
      LOG(ERROR) << " failed to stop the audio decoder";
      Napi::TypeError::New(env, "exception while stopping a decoder").ThrowAsJavaScriptException();
      return;
    }
  }
}

void AudioDecoder::Pause(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  assert(decoder_);
  if(decoder_) {
    int err;
    err = decoder_->Pause();
    if(err) {
      LOG(ERROR) << " failed to pause the audio decoder";
      Napi::TypeError::New(env, "pause exception").ThrowAsJavaScriptException();
      return;
    }
  }
}


Napi::Value AudioDecoder::Decode(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (info.Length() <= 0 || !info[0].IsFunction()) {
    Napi::TypeError::New(env, "Function expected").ThrowAsJavaScriptException();
    return env.Undefined();
  }


  auto callback = info[0].As<Napi::Function>();
  bool sent_frame{false};
  decoder_->SetOnNullPacketSent([&](const gurum::Decoder &decoder){
    if(log_enabled_) LOG(INFO) << __func__ << " null packet!";
    callback.Call(env.Global(), {env.Null()});
    sent_frame = true;
  });

  if(sent_frame) {
    return Napi::Boolean::New(env, sent_frame);
  }

  decoder_->Decode([&](const AVFrame *arg){
    auto frame = Frame::NewInstance(env, Napi::External<AVFormatContext>::New(env, (AVFormatContext *)arg));
    callback.Call(env.Global(), {frame});
    sent_frame = true;
  });

  return Napi::Boolean::New(env, sent_frame);
}

void AudioDecoder::Flush(const Napi::CallbackInfo& info) {
  if(decoder_) decoder_->Flush();
}

Napi::Value AudioDecoder::samplerate(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), decoder_->samplerate());
}

Napi::Value AudioDecoder::sampleformat(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), decoder_->sampleFormat());
}

Napi::Value AudioDecoder::channels(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), decoder_->channels());
}

Napi::Value AudioDecoder::channellayout(const Napi::CallbackInfo& info) {
  return Napi::Number::New(info.Env(), decoder_->channellayout());
}

void AudioDecoder::EnableLog(const Napi::CallbackInfo& info, const Napi::Value &value) {
  Napi::Env env = info.Env();
  if (info.Length() <= 0 || !value.IsBoolean()) {
    Napi::TypeError::New(env, "Boolean expected").ThrowAsJavaScriptException();
    return;
  }
  log_enabled_ = value.ToBoolean();
  if(decoder_) decoder_->EnableLog(log_enabled_);
}

Napi::Value AudioDecoder::log_enabled(const Napi::CallbackInfo& info) {
  return Napi::Boolean::New(info.Env(), log_enabled_);
}