#include "opcua_client.h"

OPCUAClient::OPCUAClient(const std::string& address)
{
  // Check address and store it
  if (address.empty())
  {
    throw OPCUAClientException("The client address cannot be empty");
  }
  this->server_address_ = address;

  // Create client and check whether it is accessible
  this->client_ = UA_Client_new();
  UA_ClientConfig_setDefault(UA_Client_getConfig(this->client_));
  if (!this->connect())
  {
    throw OPCUAClientException("OPCUA Server " + this->server_address_ + " is not reachable");
  }
}

OPCUAClient::OPCUAClient(const std::string& address, const nlohmann::json& clientData) : OPCUAClient(address)
{
  // Validate json body keys
  JsonValidator::expectKeys(clientData, { "variables" });

  if (!clientData["variables"].is_array())
  {
    throw JSONException("Expected 'variables' to be an array.");
  }

  for (const auto& variableData : clientData["variables"])
  {
    this->addVariable(variableData);
  }
}

OPCUAClient::~OPCUAClient()
{
  this->disconnect();
}

bool OPCUAClient::connect()
{
  UA_StatusCode status_code = UA_Client_connect(this->client_, this->getAddress().c_str());

  // If the connection could not be established, call the disconnect method and return false
  if (status_code != UA_STATUSCODE_GOOD)
  {
    this->disconnect();
    return false;
  }

  // Connection successful
  return true;
}

void OPCUAClient::disconnect()
{
  if (this->client_ == nullptr)
    return;

  // Delete client (implies a disconnect)
  UA_Client_delete(this->client_);

  // Delete client
  this->client_ = nullptr;
}

void OPCUAClient::addVariable(const nlohmann::json& variableData)
{
  // Validate json body keys
  JsonValidator::expectKeys(variableData, { "name", "namespaceID", "nodeID" });

  // Parse variables
  std::string variableName = JsonValidator::parseString(variableData, "name");
  auto namespaceID = JsonValidator::parseInteger<uint32_t>(variableData, "namespaceID");
  auto nodeID = JsonValidator::parseInteger<uint32_t>(variableData, "nodeID");

  // Create new variable
  return this->addVariable(variableName, namespaceID, nodeID);
}

void OPCUAClient::addVariable(const std::string& name, uint32_t namespaceID, uint32_t nodeID)
{
  // Check inputs
  if (name.empty())
  {
    throw OPCUAClientException("The variable name cannot be empty");
  }
  UA_Variant output;
  auto retRead = UA_Client_readValueAttribute(client_, UA_NODEID_NUMERIC(namespaceID, nodeID), &output);

  // Check return status
  if (retRead != UA_STATUSCODE_GOOD)
  {
    throw OPCUAClientException("Variable doesn't exist");
  }

  auto newVar = std::make_shared<OPCUAVariable>(name, namespaceID, nodeID, output);

  // Clear output
  UA_Variant_clear(&output);

  // Check if variable already exists
  if (this->hasVariable(newVar))
  {
    throw OPCUAClientException("Variable already exists");
  }

  // Check whether the variable is valid
  newVar->validate(this->client_);

  // Add variable to the vector of variables
  this->variables_.push_back(newVar);
}

void OPCUAClient::removeVariable(const std::shared_ptr<OPCUAVariable>& variable)
{
  for (auto it = this->variables_.begin(); it < this->variables_.end(); ++it)
  {
    if (*it == variable)
    {
      this->variables_.erase(it);
    }
  }
}

bool OPCUAClient::hasVariable(const std::shared_ptr<OPCUAVariable>& variable)
{
  return std::count(this->variables_.begin(), this->variables_.end(), variable);
}

UA_ReadResponse OPCUAClient::fetchData()
{
  UA_ReadRequest request;
  UA_ReadResponse response;

  UA_ReadRequest_init(&request);
  UA_ReadResponse_init(&response);

  std::vector<UA_ReadValueId> out;
  for (const auto& var : this->variables_)
  {
    out.push_back(var->getReadValueID());
  }

  request.nodesToRead = out.data();
  request.nodesToReadSize = out.size();

  return UA_Client_Service_read(client_, request);
}

std::vector<std::shared_ptr<OPCUAVariable>>& OPCUAClient::getVariables()
{
  return this->variables_;
}

std::string OPCUAClient::getAddress() const
{
  return this->server_address_;
}

void OPCUAClient::addVariablesToTable(QTableWidget& tableWidget)
{
  this->tableLUT_.clear();
  for (const auto& variable : this->variables_)
  {
    int rowIndex = tableWidget.rowCount();

    // Create new row
    tableWidget.insertRow(rowIndex);

    // Create link in tableLUT_
    this->tableLUT_[rowIndex] = variable;

    // Create cell item for each proper
    std::vector<QTableWidgetItem*> items = {
      new QTableWidgetItem(QString::fromStdString(this->getAddress())),
      new QTableWidgetItem(QString::fromStdString(variable->getName())),
      new QTableWidgetItem(QString::number(variable->getNamespaceID())),
      new QTableWidgetItem(QString::number(variable->getNodeID())),
      new QTableWidgetItem(QString::fromStdString(variable->getDescription(this->client_))),
      new QTableWidgetItem(QString::fromStdString(variable->getTypeAsString())),
      new QTableWidgetItem(variable->isValid() ? "VALID" : "INVALID"),
    };

    // Add all cell items to the table
    for (uint index = 0; index < items.size(); ++index)
    {
      // Get index of item
      auto item = items[index];

      // Make the background red if the variable isn't valid
      if (!variable->isValid())
        item->setBackground(Qt::red);

      // Disable editing of item
      item->setFlags(item->flags() ^ Qt::ItemIsEditable);
      // Add item to table cell
      tableWidget.setItem(rowIndex, index, item);
    }
  }
}

nlohmann::json OPCUAClient::toJSON()
{
  nlohmann::json variables = nlohmann::json::array();

  for (const auto& variable : this->variables_)
  {
    variables.push_back(variable->toJSON());
  }

  nlohmann::json rep;
  rep[this->getAddress()] = {{"variables", variables}};
  return rep;
}

uint32_t OPCUAClient::numVariables()
{
  return this->variables_.size();
}

std::shared_ptr<OPCUAVariable> OPCUAClient::variableFromTableRow(int tableRow)
{
  if (this->tableLUT_.find(tableRow) != this->tableLUT_.end())
  {
    return this->tableLUT_.at(tableRow);
  }
  else
  {
    return nullptr;
  }
}
bool OPCUAClient::operator==(const OPCUAClient& other) const
{
  return this->getAddress() == other.getAddress();
}

bool OPCUAClient::operator!=(const OPCUAClient& other) const
{
  return !(*this == other);
}
