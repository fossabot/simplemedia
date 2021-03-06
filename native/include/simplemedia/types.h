#ifndef GURUM_TYPES_H_
#define GURUM_TYPES_H_

#include <memory>
#include <functional>
#include <libavcodec/avcodec.h>

namespace gurum {

using CodecParameters=std::unique_ptr<AVCodecParameters, std::function<void(AVCodecParameters *)>>;

}


#endif // GURUM_TYPES_H_