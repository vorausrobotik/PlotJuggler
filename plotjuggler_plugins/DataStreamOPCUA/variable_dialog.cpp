#include "variable_dialog.h"

inline bool file_exists(const std::string& name)
{
  std::ifstream f(name.c_str());
  return f.good();
}

VariableLoader::VariableLoader(QWidget* parent) : QDialog(parent)
{
  // Create configuration directory if it doesn't exist yet
  this->createConfigDirectory();

  // Set the update interval and clients pointer
  this->update_interval_ = this->DEFAULT_INTERVAL_;

  // Initialize the layout
  this->initLayout_();

  // Update the table
  this->updateTable();

  // We haven't any unsaved changes yet
  this->unsavedChanges_ = false;
}

void VariableLoader::initLayout_()
{
  // INPUT FORM ////////////////////////////////////////////////////////////////////////////

  // Update interval. OnChange: Update the updateInterval variable
  this->intervalFormElement = new FormNumberElement("Update interval [ms]", this->update_interval_);
  this->intervalFormElement->onChange(this, SLOT(updateInterval()));

  // Input fields
  this->serverFormElement = new FormStringElement("Server address", "opc.tcp://IP_ADDRESS:PORT");
  this->variableNameFormElement = new FormStringElement("Variable Name", "Name of the variable");
  this->namespaceIDFormElement = new FormNumberElement("Namespace ID [numeric]", 1);
  this->nodeIDFormElement = new FormNumberElement("Node ID [numeric]", 1);

  // Add variable button
  this->addButtonFormElement = new FormButtonElement("Add");
  this->addButtonFormElement->onChange(this, SLOT(addVariable()));

  // Clear input button
  this->clearButtonFormElement = new FormButtonElement("Clear");
  this->clearButtonFormElement->onChange(this, SLOT(clearInput()));

  // TABLE /////////////////////////////////////////////////////////////////////////////////

  // Table which displays all variables
  tableWidget_ = new QTableWidget();
  tableWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  tableWidget_->setSelectionBehavior(QAbstractItemView::SelectRows);

  // Subscribe the onTableSelectionChanged method to all table selection change events
  connect(tableWidget_->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
          SLOT(onTableSelectionChanged(const QItemSelection&, const QItemSelection&)));

  // DIALOG BUTTONS ////////////////////////////////////////////////////////////////////////
  this->dialogButtons = new QDialogButtonBox();

  // Define buttons
  this->okButton_ = new FormButtonElement("OK", QDialogButtonBox::ButtonRole::AcceptRole);
  this->cancelButton_ = new FormButtonElement("Cancel", QDialogButtonBox::ButtonRole::RejectRole);
  this->loadJSONButton_ = new FormButtonElement("Load from JSON", QDialogButtonBox::ButtonRole::ActionRole);
  this->saveJSONButton_ = new FormButtonElement("Save to JSON", QDialogButtonBox::ButtonRole::ActionRole);
  this->deleteVariableButton_ = new FormButtonElement("Delete variable", QDialogButtonBox::ButtonRole::ActionRole);

  // Add buttons to the dialog box
  for (const auto& btn : { okButton_, cancelButton_, loadJSONButton_, saveJSONButton_, deleteVariableButton_ })
  {
    btn->addToButtonBox(dialogButtons);
  }

  this->deleteVariableButton_->onChange(this, SLOT(deleteVariable()));
  this->saveJSONButton_->onChange(this, SLOT(saveJSON()));
  this->loadJSONButton_->onChange(this, SLOT(loadJSON()));

  // Disable delete variable button
  this->deleteVariableButton_->setEnabled(false);

  // Connect the accept and reject methods to the OK and Cancel buttons
  connect(dialogButtons, SIGNAL(accepted()), this, SLOT(accept()));
  connect(dialogButtons, SIGNAL(rejected()), this, SLOT(reject()));

  // LAYOUT ///////////////////////////////////////////////////////////////////////////////

  auto* intervalLayout = new QHBoxLayout;
  this->intervalFormElement->addToLayout(intervalLayout);

  auto* inputLayout = new QHBoxLayout;
  this->serverFormElement->addToLayout(inputLayout);
  this->variableNameFormElement->addToLayout(inputLayout);
  this->namespaceIDFormElement->addToLayout(inputLayout);
  this->nodeIDFormElement->addToLayout(inputLayout);
  this->addButtonFormElement->addToLayout(inputLayout);
  this->clearButtonFormElement->addToLayout(inputLayout);

  auto* mainLayout = new QGridLayout;
  mainLayout->addLayout(intervalLayout, 0, 0);
  mainLayout->addLayout(inputLayout, 1, 0);
  mainLayout->addWidget(tableWidget_, 2, 0);
  mainLayout->addWidget(dialogButtons, 3, 0);

  setLayout(mainLayout);
  setMinimumWidth(this->windowMinWidth_);
  setMaximumWidth(this->windowMaxWidth_);
  setMinimumHeight(this->windowMinHeight_);
  setMaximumHeight(this->windowMaxHeight_);
  setWindowTitle(this->windowTitle_);

  // Update the table
  this->updateTable();
}

void VariableLoader::showMessagePopup(const std::string& type, const std::string& message)
{
  QMessageBox msgBox;
  msgBox.setWindowTitle(QString::fromStdString(type));
  msgBox.setText(QString::fromStdString(message));
  msgBox.setMinimumWidth(400);
  msgBox.exec();
}

void VariableLoader::saveJSON()
{
  if (this->getNumberOfVariables() == 0)
  {
    VariableLoader::showMessagePopup("error", "There are no variables to save");
    return;
  }

  auto filter = "OPCUA PlotJuggler configuration files (*" + this->CONFIG_FILE_ENDING + ")";
  auto fileName = QFileDialog::getSaveFileName(this, "Save OPCUA configuration", this->DEFAULT_PATH, filter);

  // Make sure that the file ending is correct
  if (!fileName.endsWith(this->CONFIG_FILE_ENDING))
  {
    fileName += this->CONFIG_FILE_ENDING;
  }

  // Create new json object
  jsoncons::json config(jsoncons::json_object_arg);

  config.insert_or_assign("interval", this->update_interval_);
  config.insert_or_assign("clients", jsoncons::json_object_arg);

  for (const auto& client : this->clients_)
  {
    config.at("clients").merge(client->toJSON());
  }

  // Save configuration to file
  std::ofstream file(fileName.toStdString());
  file << jsoncons::pretty_print(config);

  // When everything is done, set unsaved changes to false
  this->unsavedChanges_ = false;

  auto message = "Configuration has been saved to file '" + fileName.toStdString() + "'";
  VariableLoader::showMessagePopup("info", message);
}

void VariableLoader::loadJSON()
{
  auto filter = "OPCUA PlotJuggler configuration files (*" + this->CONFIG_FILE_ENDING + ")";
  auto fileName = QFileDialog::getOpenFileName(this, "Load OPCUA configuration", this->DEFAULT_PATH, filter);

  // If there are unsaved changes, notify the user and abort
  if (this->unsavedChanges_)
  {
    this->showMessagePopup("error", "You have unsaved changes, if you want to delete all variables, please restart the "
                                    "plugin");
    return;
  }

  // Parse the new configuration
  try
  {
    auto loaderResult = VariableLoader::parseConfig(fileName.toStdString());
    this->clients_ = std::move(loaderResult.first);
    this->update_interval_ = loaderResult.second;
    this->updateTable();
  }
  catch (std::exception& e)
  {
    this->showMessagePopup("error", e.what());
    return;
  }
}

void VariableLoader::updateTable()
{
  // Clear the table
  tableWidget_->setRowCount(0);
  tableWidget_->clear();
  tableWidget_->setColumnCount(this->tableHeader_.size());
  tableWidget_->setHorizontalHeaderLabels(this->tableHeader_);
  tableWidget_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

  // For each client in the list of clients, iterate over all variables and create an entry in the table
  for (const auto& client : this->clients_)
  {
    client->addVariablesToTable(*this->tableWidget_);
  }

  //  Enable the OK button if there is at least one variable and all variables are valid
  this->okButton_->setEnabled(this->getNumberOfVariables() > 0 && this->allVariablesValid());

  // The "Save to JSON" button is enabled as soon as there is one variable, no matter whether it's valid no not
  this->saveJSONButton_->setEnabled(this->getNumberOfVariables() > 0);

  // Set unsaved changes to true
  this->unsavedChanges_ = true;
}

void VariableLoader::clearInput()
{
  // Clear all inputs
  this->intervalFormElement->clear();
  this->serverFormElement->clear();
  this->variableNameFormElement->clear();
  this->namespaceIDFormElement->clear();
  this->nodeIDFormElement->clear();
}

void VariableLoader::addVariable()
{
  // Get variable data from the input fields
  QString serverAddress = this->serverFormElement->getValue();
  QString variableName = this->variableNameFormElement->getValue();
  uint32_t variableNamespaceID = this->namespaceIDFormElement->getValue();
  uint32_t variableNodeID = this->nodeIDFormElement->getValue();

  try
  {
    // Get existing client from address or if it doesn't exist, create a new one
    auto& client = VariableLoader::getOrInsertNewClient(this->clients_, serverAddress.toStdString());

    // Try to add new variable. If it fails, show the error.
    client.addVariable(variableName.toStdString(), variableNamespaceID, variableNodeID);
  }
  catch (OPCUAClientException& e)
  {
    this->showMessagePopup("error", e.what());
  }

  // Clear the input
  this->clearInput();

  // Update the table
  this->updateTable();
}

void VariableLoader::deleteVariable()
{
  // Get currently selected row and total number of rows
  uint32_t currentRow = this->tableWidget_->currentRow();
  uint64_t numRows = this->tableWidget_->rowCount();

  // If the selection is invalid, return
  if (currentRow > numRows - 1 || currentRow < 0)
    return;

  // Otherwise delete the variable
  for (auto& client : this->clients_)
  {
    auto var = client->variableFromTableRow(currentRow);
    if (var == nullptr)
    {
      continue;
    }
    client->removeVariable(var);

    // If the client hasn't any more variables, delete it
    if (client->numVariables() == 0)
    {
      VariableLoader::removeClient(this->clients_, client);
    }
    break;
  }

  // Update the table
  updateTable();
}

void VariableLoader::accept()
{
  QDialog::accept();
}

void VariableLoader::reject()
{
  // If there aren't any unsaved changes, the dialog can be closed
  if (!this->unsavedChanges_)
  {
    QDialog::reject();
    return;
  }

  // Otherwise, ask the user to confirm
  auto resButton = QMessageBox::question(this, "Cancel", "You have unsaved changes, sure you want to cancel?",
                                         QMessageBox::No | QMessageBox::Yes);
  if (resButton == QMessageBox::Yes)
  {
    QDialog::reject();
    return;
  }
}

void VariableLoader::onTableSelectionChanged(const QItemSelection& selectedItem, const QItemSelection& deselectedItem)
{
  // Update the enabled state of the deleteVariableButton
  this->deleteVariableButton_->setEnabled(!selectedItem.isEmpty());
}

uint32_t VariableLoader::getNumberOfVariables()
{
  uint32_t numVars = 0;
  for (const auto& client : this->clients_)
  {
    numVars += client->numVariables();
  }
  return numVars;
}

std::pair<OPCUAClientList, uint32_t> VariableLoader::parseConfig(const std::string& configFilePath)
{
  jsoncons::json configuration;

  std::pair<OPCUAClientList, uint32_t> loaderResult;

  // If the config file doesn't exist, display an error and exit
  if (!file_exists(configFilePath))
  {
    throw JSONException("Couldn't find config file " + configFilePath);
  }

  // Parse configuration
  std::ifstream fs;
  fs.open(configFilePath, std::fstream::in);

  try
  {
    auto options = jsoncons::json_options{}
        .err_handler(jsoncons::strict_json_parsing());
    configuration = jsoncons::json::parse(fs, options);
  }
  catch (const jsoncons::ser_error& e)
  {
    throw JSONException("Error while parsing " + configFilePath + ":\n" + e.what());
  }

  // Validate json
  try
  {
    JsonValidator::expectKeys(configuration, { "interval", "clients" });
  }
  catch (JSONException&)
  {
    throw JSONException("The configuration must contain an update interval and the clients");
  }

  // Validate and store interval
  try
  {
    loaderResult.second = JsonValidator::parseInteger<uint64_t>(configuration, "interval");
  }
  catch (JSONException&)
  {
    throw JSONException("Invalid interval in configuration");
  }

  // Validate clients
  if (!configuration.at("clients").is_object())
  {
    throw JSONException("Client data in the configuration is invalid");
  }

  // Iterate over clients and create instances if they are valid
  for (const auto& client : configuration.at("clients").object_range())
  {
    const std::string& clientAddress = client.key();
    const jsoncons::json& clientData = client.value();
    try
    {
      auto newClient = std::make_unique<OPCUAClient>(clientAddress, clientData);
      VariableLoader::addClient(loaderResult.first, newClient);
    }
    catch (std::exception& e)
    {
      VariableLoader::showMessagePopup("error", e.what());
    }
  }

  fs.close();

  return loaderResult;
}

bool VariableLoader::createConfigDirectory()
{
  if (!QDir(this->DEFAULT_PATH).exists())
  {
    return QDir().mkpath(this->DEFAULT_PATH);
  }
  return true;
}

void VariableLoader::updateInterval()
{
  std::cout << "Updating interval to " << this->intervalFormElement->getValue() << std::endl;
  this->update_interval_ = this->intervalFormElement->getValue();
}

void VariableLoader::addClient(OPCUAClientList& clientList, std::unique_ptr<OPCUAClient>& client)
{
  if (std::find(clientList.begin(), clientList.end(), client) != clientList.end())
  {
    return;
  }
  clientList.push_back(std::move(client));
}

void VariableLoader::removeClient(OPCUAClientList& clientList, const std::unique_ptr<OPCUAClient>& client)
{
  for (auto it = clientList.begin(); it != clientList.end(); ++it)
  {
    if (*it == client)
    {
      clientList.erase(it);
      break;
    }
  }
}

OPCUAClientList&& VariableLoader::getClients()
{
  return std::move(this->clients_);
}

uint32_t VariableLoader::getInterval() const
{
  return this->update_interval_;
}

OPCUAClient& VariableLoader::getOrInsertNewClient(OPCUAClientList& clientList, const std::string& clientAddress)
{
  for (auto& client : clientList)
  {
    if (client->getAddress() == clientAddress)
      return *client;
  }
  auto newClient = std::make_unique<OPCUAClient>(clientAddress);
  clientList.push_back(std::move(newClient));
  return *clientList.back();
}

bool VariableLoader::allVariablesValid() const
{
  for (const auto& client : this->clients_) {
    for (const auto& variable : client->getVariables()) {
      if (!variable->isValid()) {
        return false;
      }
    }
  }
  return true;
}
