#ifndef PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_OPCUAVARIABLE_H
#define PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_OPCUAVARIABLE_H

#include <string>
#include <iostream>
#include <utility>
#include <QTableWidget>
#include <open62541/server.h>
#include <open62541/client_highlevel.h>
#include <nlohmann/json.hpp>
#include "PlotJuggler/plotdata.h"

class OPCUAVariableException : public std::exception
{
private:
  std::string message_;

public:
  explicit OPCUAVariableException(const std::string& message)
  {
    this->message_ = message;
  }
  const char* what() const noexcept override
  {
    return message_.c_str();
  }
};

class OPCUAVariable
{
private:
  std::string name_;
  std::string nodeIdStr_;

  UA_NodeId nodeID_{};

  UA_ReadValueId readValueID_{};

  UA_DataType variableType_;
  uint32_t arrayLength_;
  bool isArray_;
  bool isValid_;

public:
  /**
   * OPCUAVariable constructor
   *
   * @param name is the name of the opcua variable
   * @param nodeId is the ID of the node
   * @param ua_variable is the response from the server on the first validation request
   */
  OPCUAVariable(const std::string &name, const std::string &nodeId);
  
  ~OPCUAVariable();

  /**
   * @return the name of this OPCUAVariable
   */
  std::string getName() const;

  /**
   * TODO
   * @return
   */
  std::string getBrowseName(UA_Client* client) const;

  /**
   * TODO
   * @return
   */
  std::string getDescription(UA_Client* client) const;

  /**
   * @return the node ID of this OPCUAVariable
   */
   std::string getNodeID() const;

  /**
   * @return the type of this OPCUAVariable
   */
  uint32_t getType() const;

  /**
   * @return the type of this OPCUAVariable as string
   */
  std::string getTypeAsString() const;

  /**
   * @return the internal readValueID for the OPCUA request
   */
  UA_ReadValueId& getReadValueID();
  
  /**
   * @return whether this OPCUAVariable is an array
   */
  bool isArray() const;

  /**
   * JSON serializing method
   */
  nlohmann::json toJSON() const;

  /**
   * Validates this variable
   *
   * @param client is a pointer to the opcua client
   */
  void validate(UA_Client* client);

  /**
   * @return whether the variable is considered as valid
   */
  bool isValid() const;

  /**
   * Registers the variable to the dataMap
   * @param map is the reference to the PlotJuggler dataMap
   */
  void registerToDataMap(PJ::PlotDataMapRef& map) const;

  /**
   * Implementing streaming operator
   */
  friend std::ostream& operator<<(std::ostream& stream, const OPCUAVariable& var);

  /**
   * Equals operator
   */
  bool operator==(const OPCUAVariable& other) const;

  /**
   * Not-Equals operator
   */
  bool operator!=(const OPCUAVariable& other) const;

  /**
   * Converts UA_Strings to std::string
   */
  static std::string UAStringToString(UA_String uaString)
  {
    return std::string((char*)uaString.data, uaString.length);
  }
};

// Public streaming operator
std::ostream& operator<<(std::ostream& stream, const OPCUAVariable& var);

#endif  // PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_OPCUAVARIABLE_H
