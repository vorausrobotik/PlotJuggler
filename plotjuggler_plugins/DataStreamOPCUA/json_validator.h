#ifndef PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_JSON_VALIDATOR_H
#define PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_JSON_VALIDATOR_H

#include <sstream>
#include <jsoncons/json.hpp>
#include <jsoncons/basic_json.hpp>

class JSONException : public std::exception
{
private:
  std::string message_;

public:
  explicit JSONException(const std::string& message)
  {
    this->message_ = message;
  }
  const char* what() const noexcept override
  {
    return message_.c_str();
  }
};

class JsonValidator
{
private:
  /**
   * Raises a JSONException
   * @tparam T is the expected type
   * @param jsonData is the json object
   * @param keyName is the key
   */
  template <typename T>
  static void raiseTypeException(const jsoncons::json& jsonData, const std::string& keyName);

public:
  /**
   * Verifies that all keys in keyNames are in the json object
   * @param jsonData is the json object
   * @param keyNames is the list of expected keys
   */
  static void expectKeys(const jsoncons::json& jsonData, const std::vector<std::string>& keyNames);

  /**
   * Verifies that a value in the json object is of type string and returns it if possible
   *
   * @param jsonData is the json object
   * @param keyName is the key name
   *
   * @return is the parsed string
   */
  static std::string parseString(const jsoncons::json& jsonData, const std::string& keyName);

  /**
   * Verifies that a value in the json object is of the type T and returns it if possible.
   * @tparam T is the expected type (restrigted to integer types, e.g. uint32_t, ...)
   * @param jsonData is the json object
   * @param keyName is the key name
   *
   * @return the parsed integer
   */
  template <typename T, class = typename std::enable_if<std::is_integral<T>::value>::type>
  static T parseInteger(const jsoncons::json& jsonData, const std::string& keyName);
};

// Since it is not possible to define template methods in the CPP file, we need to define them here

template <typename T, class>
T JsonValidator::parseInteger(const jsoncons::json& jsonData, const std::string& keyName)
{
  if (!jsonData.at(keyName).is_integer<T>())
  {
    JsonValidator::raiseTypeException<T>(jsonData, keyName);
  }
  return jsonData.at(keyName).as_integer<T>();
}

template <typename T>
void JsonValidator::raiseTypeException(const jsoncons::json& jsonData, const std::string& keyName)
{
  std::stringstream ss;
  ss << "Expected value of " << keyName << " to be of the type " << typeid(T).name() << " but got "
     << jsonData.at(keyName).type() << " instead.";
  throw JSONException(ss.str());
}

#endif  // PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_JSON_VALIDATOR_H
