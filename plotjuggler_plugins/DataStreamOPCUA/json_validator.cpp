#include "json_validator.h"

void JsonValidator::expectKeys(const jsoncons::json& jsonData, const std::vector<std::string>& keyNames)
{
  // Number of items in the root of the json object
  auto numberOfChildren = std::distance(jsonData.object_range().begin(), jsonData.object_range().end());
  // Number of expected child items
  auto expectedNumberOfChildren = keyNames.size();

  // If the number of children does not match, throw an exception
  if (numberOfChildren != keyNames.size())
  {
    std::stringstream ss;
    ss << "Expected " << expectedNumberOfChildren << " keys in json but got " << numberOfChildren;
    throw JSONException(ss.str());
  }

  // Iterate over all expected keys and check if they exist in the json object
  for (const auto& key : keyNames)
  {
    if (!jsonData.contains(key))
    {
      std::stringstream ss;
      ss << "Key '" << key << "' is missing in json";
      throw JSONException(ss.str());
    }
  }
}

std::string JsonValidator::parseString(const jsoncons::json& jsonData, const std::string& keyName)
{
  if (!jsonData.at(keyName).is_string())
  {
    JsonValidator::raiseTypeException<std::string>(jsonData, keyName);
  }
  return jsonData.at(keyName).as_string();
}
