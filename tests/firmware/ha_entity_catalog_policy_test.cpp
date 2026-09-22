#include <cassert>
#include <string>

#include "ha_entity_catalog_policy.h"

using espcontrol::ha_entity_catalog_capabilities_valid;

int main() {
  assert(ha_entity_catalog_capabilities_valid(""));
  assert(ha_entity_catalog_capabilities_valid(" , \t,\n"));
  assert(ha_entity_catalog_capabilities_valid("brightness,color_temp"));
  assert(ha_entity_catalog_capabilities_valid(std::string(80, 'a')));
  assert(ha_entity_catalog_capabilities_valid(" \t" + std::string(80, 'a') + "\r\n"));
  assert(!ha_entity_catalog_capabilities_valid(std::string(81, 'a')));
  assert(!ha_entity_catalog_capabilities_valid(std::string(120, 'a')));
  assert(!ha_entity_catalog_capabilities_valid("brightness, " + std::string(81, 'a')));
  assert(!ha_entity_catalog_capabilities_valid(std::string(81, 'a') + ",brightness"));
  // A valid CSV can exceed 80 characters, but must fit the 120-byte bridge budget.
  assert(ha_entity_catalog_capabilities_valid(std::string(80, 'a') + "," + std::string(39, 'b')));
  assert(!ha_entity_catalog_capabilities_valid(std::string(80, 'a') + "," + std::string(40, 'b')));
  return 0;
}
