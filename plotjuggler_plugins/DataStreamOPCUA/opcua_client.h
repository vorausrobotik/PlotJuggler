#ifndef PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_OPCUA_CLIENT_H
#define PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_OPCUA_CLIENT_H

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <open62541/server.h>
#include <open62541/client.h>
#include <open62541/client_config_default.h>
#include <open62541/client_highlevel.h>
#include <open62541/statuscodes.h>
#include <nlohmann/json.hpp>
#include <json_validator.h>
#include "opcua_variable.h"

class OPCUAClientException : public std::exception
{
private:
  std::string message_;

public:
  explicit OPCUAClientException(const std::string& message)
  {
    this->message_ = message;
  }
  const char* what() const noexcept override
  {
    return message_.c_str();
  }
};

class OPCUAClient
{
private:
  std::string server_address_;
  std::map<int, std::shared_ptr<OPCUAVariable>> tableLUT_{};

  // Client
  UA_Client* client_ = nullptr;

  // List of assigned opcua variables
  std::vector<std::shared_ptr<OPCUAVariable>> variables_;

public:
  // Constructors
  explicit OPCUAClient(const std::string& address);
  OPCUAClient(const std::string& address, const nlohmann::json& clientData);

  // Destructor
  ~OPCUAClient();

  // Connect method
  bool connect();

  // Disconnect method
  void disconnect();

  /**
   * Adds an opcua variable to the internal list of tracked variables
   *
   * @param name is the name of the new variable
   * @param namespaceID is the ID of the OPCUA namespace
   * @param nodeID is the ID of the OPCUA node
   *
   * @throw OPCUAClientException if something went wrong
   */
  void addVariable(const std::string& name, uint32_t namespaceID, uint32_t nodeID);

  /**
   * Adds an opcua variable to the internal list of tracked variables
   *
   * @param variableData is a json object which contains all necessary data for the variable
   *
   * @throw OPCUAClientException if something went wrong
   */
  void addVariable(const nlohmann::json& variableData);

  /**
   * Removes an opcua variable from the internal list of tracked variables
   *
   * @param variable is the opcua variable which is to be removed
   */
  void removeVariable(const std::shared_ptr<OPCUAVariable>& variable);

  /**
   * Returns whether the variable has been registered to this client
   * @param variable is the variable which is to be checked
   * @return true if the variable is known to the client, false otherwise
   */
  bool hasVariable(const std::shared_ptr<OPCUAVariable>& variable);

  /**
   * Adds all variables to a table widget
   *
   * @param tableWidget is the table widget reference
   */
  void addVariablesToTable(QTableWidget& tableWidget);

  /**
   * Returns a variable reference by looking it up in the LUT
   *
   * @param tableRow is the row number
   *
   * @return the opcua variable
   */
  std::shared_ptr<OPCUAVariable> variableFromTableRow(int tableRow);

  /**
   * @return a vector of pointers to the variables
   */
  std::vector<std::shared_ptr<OPCUAVariable>>& getVariables();

  /**
   * @return the number of variables
   */
  uint32_t numVariables();

  /**
   * Fetches all variables from the OPCUAServer
   * @return UA_ReadResponse from the server
   */
  UA_ReadResponse fetchData();

  /**
   * @return the url of the OPCUA server
   */
  std::string getAddress() const;

  nlohmann::json toJSON();

  /**
   * Equals operator
   */
  bool operator==(const OPCUAClient& other) const;

  /**
   * Not-Equals operator
   */
  bool operator!=(const OPCUAClient& other) const;

};

// TYPEDEFS
typedef std::vector<std::unique_ptr<OPCUAClient>> OPCUAClientList;

#endif  // PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_OPCUA_CLIENT_H
