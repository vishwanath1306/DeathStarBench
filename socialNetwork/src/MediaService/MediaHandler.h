#ifndef SOCIAL_NETWORK_MICROSERVICES_SRC_MEDIASERVICE_MEDIAHANDLER_H_
#define SOCIAL_NETWORK_MICROSERVICES_SRC_MEDIASERVICE_MEDIAHANDLER_H_

#include <chrono>
#include <iostream>
#include <string>

#include "../../gen-cpp/MediaService.h"
#include "../logger.h"
#include "../tracing.h"

// 2018-01-01 00:00:00 UTC
#define CUSTOM_EPOCH 1514764800000

extern "C"{
    #include "tracer/hindsight.h"
    #include "tracer/agentapi.h"
}

namespace social_network {

class MediaHandler : public MediaServiceIf {
 public:
  MediaHandler() = default;
  ~MediaHandler() override = default;

  void ComposeMedia(std::vector<Media> &_return, int64_t,
                    const std::vector<std::string> &,
                    const std::vector<int64_t> &,
                    const std::map<std::string, std::string> &) override;

 private:
};

void MediaHandler::ComposeMedia(
    std::vector<Media> &_return, int64_t req_id,
    const std::vector<std::string> &media_types,
    const std::vector<int64_t> &media_ids,
    const std::map<std::string, std::string> &carrier) {
  // Initialize a span
  TextMapReader reader(carrier);
  std::map<std::string, std::string> writer_text_map;
  TextMapWriter writer(writer_text_map);

  // Hindsight Instrumentation Begin.
  hindsight_begin(req_id);

  auto baggage_it = carrier.find("baggage");
  if(baggage_it != carrier.end()){
    hindsight_deserialize(strdup((baggage_it->second).c_str()));
  }else{
    hindsight_breadcrumb(hindsight_serialize());
  }

  char trace_point_text[128];
  sprintf(trace_point_text, "compose_media_server");
  hindsight_tracepoint(trace_point_text, sizeof(trace_point_text));
  writer_text_map["baggage"] = hindsight_serialize();
  
  // Hindsight Instrumentation End. 
  auto parent_span = opentracing::Tracer::Global()->Extract(reader);
  auto span = opentracing::Tracer::Global()->StartSpan(
      "compose_media_server", {opentracing::ChildOf(parent_span->get())});
  opentracing::Tracer::Global()->Inject(span->context(), writer);

  if (media_types.size() != media_ids.size()) {
    ServiceException se;
    se.errorCode = ErrorCode::SE_THRIFT_HANDLER_ERROR;
    se.message =
        "The lengths of media_id list and media_type list are not equal";
    throw se;
  }

  for (int i = 0; i < media_ids.size(); ++i) {
    Media new_media;
    new_media.media_id = media_ids[i];
    new_media.media_type = media_types[i];
    _return.emplace_back(new_media);
  }
  hindsight_trigger(req_id);
  hindsight_end();
  span->Finish();
}

}  // namespace social_network

#endif  // SOCIAL_NETWORK_MICROSERVICES_SRC_MEDIASERVICE_MEDIAHANDLER_H_
