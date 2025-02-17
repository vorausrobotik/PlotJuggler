#include "opcua_variable.h"

OPCUAVariable::OPCUAVariable(const std::string &name, const std::string &nodeId) {
  this->name_ = name;
  this->nodeIdStr_ = nodeId;

  UA_NodeId_parse(&this->nodeID_, UA_STRING(nodeIdStr_.data()));
  this->isValid_ = false;
}

OPCUAVariable::~OPCUAVariable() {
  UA_NodeId_clear(&this->nodeID_);
}

std::string OPCUAVariable::getName() const
{
  return this->name_;
}

std::string OPCUAVariable::getNodeID() const
{
  return this->nodeIdStr_;
}

uint32_t OPCUAVariable::getType() const
{
  return this->variableType_.typeId.identifier.numeric;
}

std::string OPCUAVariable::getTypeAsString() const
{
  auto baseType = std::string(this->variableType_.typeName);
  return this->isArray() ? (baseType + "[" + std::to_string(this->arrayLength_) + "]") : baseType;
}

bool OPCUAVariable::isArray() const
{
  return this->isArray_;
}

void OPCUAVariable::registerToDataMap(PJ::PlotDataMapRef& map) const
{
  if (this->isArray())
  {
    for (size_t i = 0; i < this->arrayLength_; i++)
    {
      map.addNumeric(this->getName() + "/" + std::to_string(i));
    }
  }
  else
  {
    map.addNumeric(this->getName());
  }
}

bool OPCUAVariable::operator==(const OPCUAVariable& other) const
{
  return this->getName() == other.getName() && this->getNodeID() == other.getNodeID();
}

bool OPCUAVariable::operator!=(const OPCUAVariable& other) const
{
  return !(*this == other);
}

std::ostream& operator<<(std::ostream& stream, const OPCUAVariable& var)
{
  stream << "<OPCUAVariable | " << var.getName() << " | " << var.getNodeID();
  return stream;
}

void OPCUAVariable::validate(UA_Client* client)
{
  this->isValid_ = false;

  UA_Variant output;
  UA_StatusCode status_code = UA_Client_readValueAttribute(client, this->nodeID_, &output);

  // If the request is successful, the variable might be valid
  if (status_code != UA_STATUSCODE_GOOD) {
    UA_Variant_clear(&output);
    throw OPCUAVariableException("Variable doesn't exist");
  }

  // If the variable isn't numeric, it is invalid bc. other types (e.g. strings) aren't supported yet
  if (!UA_DataType_isNumeric(output.type)) {
    UA_Variant_clear(&output);
    throw OPCUAVariableException("Variable is not numeric");
  }
  
  this->variableType_ = *(output.type);
  this->arrayLength_ = output.arrayLength;
  this->isArray_ = !UA_Variant_isScalar(&output);
  this->readValueID_ = UA_ReadValueId{this->nodeID_, UA_ATTRIBUTEID_VALUE };
  this->isValid_ = true;

  // Clear the output
  UA_Variant_clear(&output);
}

bool OPCUAVariable::isValid() const
{
  return this->isValid_;
}

nlohmann::json OPCUAVariable::toJSON() const
{
  nlohmann::json repr;
  repr["name"] = this->getName();
  repr["nodeID"] = this->getNodeID();

  return repr;
}

UA_ReadValueId& OPCUAVariable::getReadValueID()
{
  return this->readValueID_;
}

std::string OPCUAVariable::getBrowseName(UA_Client* client) const
{
  UA_QualifiedName output;
  UA_StatusCode code = UA_Client_readBrowseNameAttribute(client, this->nodeID_, &output);
  if (code != UA_STATUSCODE_GOOD) {
    return "Unknown";
  }
  return OPCUAVariable::UAStringToString(output.name);

}
std::string OPCUAVariable::getDescription(UA_Client* client) const
{
  UA_LocalizedText output;
  UA_StatusCode code = UA_Client_readDisplayNameAttribute(client, this->nodeID_, &output);
  if (code != UA_STATUSCODE_GOOD) {
    return "Unknown";
  }
  return OPCUAVariable::UAStringToString(output.text);
}
