#pragma once

#ifdef USE_WEBSERVER

#include <array>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>

#include <esp_http_server.h>

#include "button_grid_ha.h"
#include "esphome/components/json/json_util.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "panel_identity.h"

namespace espcontrol {

// The browser uses a short-polling endpoint because ESP-IDF's request object
// cannot safely be retained while an ESPHome action is travelling to HA.
constexpr size_t HA_ENTITY_CATALOG_MAX_PENDING = 2;
constexpr uint32_t HA_ENTITY_CATALOG_TIMEOUT_MS = 15000;
constexpr size_t HA_ENTITY_CATALOG_MAX_QUERY = 120;

struct HaEntityCatalogPending {
  enum class State : uint8_t { FREE, PENDING, COMPLETE, ERROR };
  State state{State::FREE};
  uint32_t request_id{0};
  uint32_t call_id{0};
  uint32_t created_ms{0};
  std::string body;
  std::string error;
};

inline std::array<HaEntityCatalogPending, HA_ENTITY_CATALOG_MAX_PENDING> &
ha_entity_catalog_pending() {
  static std::array<HaEntityCatalogPending, HA_ENTITY_CATALOG_MAX_PENDING> slots;
  return slots;
}

inline std::mutex &ha_entity_catalog_mutex() {
  static std::mutex mutex;
  return mutex;
}

inline uint32_t ha_entity_catalog_next_request_id() {
  static uint32_t value = 1;
  const uint32_t result = value++;
  return result == 0 ? value++ : result;
}

inline uint32_t ha_entity_catalog_next_call_id() {
  static uint32_t value = 310000;
  const uint32_t result = value++;
  return result == 0 ? value++ : result;
}

inline HaEntityCatalogPending *ha_entity_catalog_find(uint32_t request_id) {
  for (auto &slot : ha_entity_catalog_pending()) {
    if (slot.state != HaEntityCatalogPending::State::FREE &&
        slot.request_id == request_id) {
      return &slot;
    }
  }
  return nullptr;
}

inline std::string ha_entity_catalog_json_status(const char *status,
                                                 uint32_t request_id,
                                                 const char *error = nullptr) {
  return esphome::json::build_json([&](JsonObject root) {
    root["status"] = status;
    root["request_id"] = request_id;
    if (error != nullptr) root["error"] = error;
  });
}

inline void ha_entity_catalog_complete(uint32_t request_id,
                                       const esphome::api::ActionResponse &response) {
  std::lock_guard<std::mutex> lock(ha_entity_catalog_mutex());
  HaEntityCatalogPending *slot = ha_entity_catalog_find(request_id);
  if (slot == nullptr || slot->state != HaEntityCatalogPending::State::PENDING) return;
  if (!response.is_success()) {
    slot->state = HaEntityCatalogPending::State::ERROR;
    slot->error = response.get_error_message().c_str();
    return;
  }
  auto root = response.get_json();
  auto payload = root["response"];
  if (payload.isNull()) {
    slot->state = HaEntityCatalogPending::State::ERROR;
    slot->error = "Home Assistant returned no catalog response";
    return;
  }
  slot->body.clear();
  serializeJson(payload, slot->body);
  if (slot->body.empty()) {
    slot->state = HaEntityCatalogPending::State::ERROR;
    slot->error = "Home Assistant returned an empty catalog response";
    return;
  }
  slot->state = HaEntityCatalogPending::State::COMPLETE;
}

inline bool ha_entity_catalog_send(HaEntityCatalogPending &slot,
                                   const std::string &query,
                                   const std::string &field,
                                   const std::string &area,
                                   const std::string &device_id,
                                   const std::string &limit,
                                   const std::string &cursor) {
  if (!ha_api_state_connected() || !ha_internal_heap_available("entity catalog")) {
    return false;
  }
  esphome::api::HomeassistantActionRequest request;
  const uint32_t call_id = ha_entity_catalog_next_call_id();
  if (!ha_action_begin(request, "espcontrol.search_entities", false, 8, call_id)) {
    return false;
  }
  request.wants_response = true;
  ha_action_add_data(request, "query", query.c_str());
  ha_action_add_data(request, "field", field.c_str());
  ha_action_add_data(request, "area", area.c_str());
  ha_action_add_data(request, "device_id", device_id.c_str());
  ha_action_add_data(request, "include_hidden", "false");
  ha_action_add_data(request, "include_disabled", "false");
  ha_action_add_data(request, "limit", limit.c_str());
  ha_action_add_data(request, "cursor", cursor.c_str());
  slot.call_id = call_id;
  if (!ha_register_action_response_callback(
          call_id, [request_id = slot.request_id](
                        const esphome::api::ActionResponse &response) {
            ha_entity_catalog_complete(request_id, response);
          })) {
    return false;
  }
  if (!ha_action_send(request)) {
    ha_cancel_action_response_callback(call_id, "Home Assistant action could not be sent");
    return false;
  }
  return true;
}

class HaEntityCatalogHandler final
    : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    if (request->method() != HTTP_GET) return false;
    char url_buffer[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    return request->url_to(url_buffer) == "/api/v1/ha/entities/search";
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    if (panel_identity == nullptr || !panel_identity->ready()) {
      request->send(503, "application/json", "{\"error\":\"panel identity unavailable\"}");
      return;
    }
#ifdef USE_WEBSERVER_AUTH
    if (!request->authenticate(panel_identity->username(), panel_identity->password())) {
      request->requestAuthentication();
      return;
    }
#endif
    const std::string request_id_text = request->arg("request_id");
    if (!request_id_text.empty()) {
      send_existing(request, static_cast<uint32_t>(std::strtoul(request_id_text.c_str(), nullptr, 10)));
      return;
    }
    start_request(request);
  }

 private:
  static void send_existing(
      esphome::web_server_idf::AsyncWebServerRequest *request,
      uint32_t request_id) {
    std::lock_guard<std::mutex> lock(ha_entity_catalog_mutex());
    HaEntityCatalogPending *slot = ha_entity_catalog_find(request_id);
    if (slot == nullptr) {
      request->send(404, "application/json", "{\"error\":\"unknown catalog request\"}");
      return;
    }
    if (slot->state == HaEntityCatalogPending::State::PENDING) {
      // The ESP-IDF web-server adapter reports non-200 async responses as
      // errors to browser fetch clients. Keep the body stateful and use 200
      // so the client can reliably continue polling.
      request->send(200, "application/json",
                    ha_entity_catalog_json_status("pending", request_id).c_str());
      return;
    }
    if (slot->state == HaEntityCatalogPending::State::ERROR) {
      const std::string body = ha_entity_catalog_json_status(
          "error", request_id, slot->error.c_str());
      request->send(502, "application/json", body.c_str());
      slot->state = HaEntityCatalogPending::State::FREE;
      return;
    }
    request->send(200, "application/json", slot->body.c_str());
    slot->state = HaEntityCatalogPending::State::FREE;
  }

  static void start_request(esphome::web_server_idf::AsyncWebServerRequest *request) {
    const std::string query = request->arg("query");
    if (query.size() > HA_ENTITY_CATALOG_MAX_QUERY) {
      request->send(400, "application/json", "{\"error\":\"query too long\"}");
      return;
    }
    HaEntityCatalogPending *slot = nullptr;
    uint32_t stale_call_id = 0;
    {
      std::lock_guard<std::mutex> lock(ha_entity_catalog_mutex());
      for (auto &candidate : ha_entity_catalog_pending()) {
        if (candidate.state == HaEntityCatalogPending::State::PENDING &&
            esphome::millis() - candidate.created_ms > HA_ENTITY_CATALOG_TIMEOUT_MS) {
          stale_call_id = candidate.call_id;
          candidate.state = HaEntityCatalogPending::State::ERROR;
          candidate.error = "Home Assistant entity catalog request timed out";
        }
        if (candidate.state == HaEntityCatalogPending::State::FREE ||
            candidate.state == HaEntityCatalogPending::State::COMPLETE ||
            candidate.state == HaEntityCatalogPending::State::ERROR) {
          slot = &candidate;
          break;
        }
      }
      if (slot == nullptr) {
        request->send(429, "application/json", "{\"error\":\"catalog busy\"}");
        return;
      }
      slot->state = HaEntityCatalogPending::State::PENDING;
      slot->request_id = ha_entity_catalog_next_request_id();
      slot->created_ms = esphome::millis();
      slot->body.clear();
      slot->error.clear();
    }
    if (stale_call_id != 0) {
      ha_cancel_action_response_callback(stale_call_id, "entity catalog request timed out");
    }
    const std::string field = request->arg("field").empty() ? "entity" : request->arg("field");
    const std::string area = request->arg("area");
    const std::string device_id = request->arg("device_id");
    const std::string limit = request->arg("limit").empty() ? "25" : request->arg("limit");
    const std::string cursor = request->arg("cursor").empty() ? "0" : request->arg("cursor");
    if (!ha_entity_catalog_send(*slot, query, field, area, device_id, limit, cursor)) {
      std::lock_guard<std::mutex> lock(ha_entity_catalog_mutex());
      slot->state = HaEntityCatalogPending::State::ERROR;
      slot->error = "Home Assistant is not ready for entity catalog requests";
    }
    const std::string body = ha_entity_catalog_json_status("pending", slot->request_id);
    // See the polling response above: pending is represented in the JSON
    // payload because the web-server adapter does not preserve 202 here.
    request->send(200, "application/json", body.c_str());
  }
};

inline bool register_ha_entity_catalog_endpoint(
    esphome::web_server_idf::AsyncWebServer &server) {
  static bool registered = false;
  if (!registered) {
    server.addHandler(new HaEntityCatalogHandler());
    registered = true;
  }
  return true;
}

}  // namespace espcontrol

#endif  // USE_WEBSERVER
