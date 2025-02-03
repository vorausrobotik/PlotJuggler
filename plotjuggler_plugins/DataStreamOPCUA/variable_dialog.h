#ifndef PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_VARIABLE_DIALOG_H
#define PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_VARIABLE_DIALOG_H

#include <variant>
#include <vector>
#include <iostream>
#include <fstream>
#include <QMainWindow>
#include <QDialog>
#include <QLabel>
#include <QStandardPaths>
#include <QFileDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <jsoncons/json.hpp>
#include <jsoncons/json_error.hpp>
#include "dialog_elements.h"
#include "opcua_variable.h"
#include "opcua_client.h"

class VariableLoader : public QDialog
{
  Q_OBJECT

public:
  // Constructor
  explicit VariableLoader(QWidget* parent = nullptr);

  // Interval
  uint32_t getInterval() const;

  // Clients
  OPCUAClientList&& getClients();

private:
  /**
   * VARIABLES
   */

  // Ending for the configuration files
  const QString CONFIG_FILE_ENDING = ".ua.pj.json";

  // Path for the configuration files to be saved in
  const QString DEFAULT_PATH =
      QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::toNativeSeparators("/PlotJuggler/"
                                                                                                     "DataStreamOPCUA"
                                                                                                     "/");
  const QString windowTitle_ = "OPCUA Variable Loader";
  const uint32_t windowMinWidth_ = 800;
  const uint32_t windowMaxWidth_ = 1200;
  const uint32_t windowMinHeight_ = 500;
  const uint32_t windowMaxHeight_ = 500;

  const uint64_t DEFAULT_INTERVAL_ = 50;

  // Header for the variable table
  const QStringList tableHeader_ = { "Server", "Name", "Namespace", "ID", "Description", "Type", "Status" };

  // Vector for opcua clients
  OPCUAClientList clients_;

  // Update interval
  uint16_t update_interval_;

  // Boolean which indicates whether there are unsaved changes
  bool unsavedChanges_;

  // Input fields and buttons
  FormNumberElement* intervalFormElement{};
  FormStringElement* serverFormElement{};
  FormStringElement* variableNameFormElement{};

  FormNumberElement* namespaceIDFormElement{};
  FormNumberElement* nodeIDFormElement{};

  FormButtonElement* addButtonFormElement{};
  FormButtonElement* clearButtonFormElement{};

  // Table widget for the added variables
  QTableWidget* tableWidget_{};

  // Dialog button box
  QDialogButtonBox* dialogButtons{};

  FormButtonElement* okButton_{};
  FormButtonElement* cancelButton_{};
  FormButtonElement* loadJSONButton_{};
  FormButtonElement* saveJSONButton_{};
  FormButtonElement* deleteVariableButton_{};

  /**
   * METHODS
   */

  void initLayout_();

  /**
   * Displays a dismissible dialog to the user
   */
  static void showMessagePopup(const std::string& type, const std::string& message);

  /**
   * Returns the number of variables
   */
  uint32_t getNumberOfVariables();

  /**
   * Parses a configuration from a given json file
   * @param configFilePath is the absolute path to the json configuration file
   *
   * @return true if the config has been parsed successfully, false otherwise
   */
  static std::pair<OPCUAClientList, uint32_t> parseConfig(const std::string& configFilePath);

  /**
   * Creates a directory for the exported configuration files if it doesn't already exist
   */
  bool createConfigDirectory();

  /**
   * Returns whether all variables are valid
   */
  bool allVariablesValid() const;

private slots:
  // Variables
  void addVariable();
  void deleteVariable();

  // Clients
  static void addClient(OPCUAClientList& clientList, std::unique_ptr<OPCUAClient>& client);
  static void removeClient(OPCUAClientList& clientList, const std::unique_ptr<OPCUAClient>& client);
  static OPCUAClient& getOrInsertNewClient(OPCUAClientList& clientList, const std::string& clientAddress);

  // Clears the form input
  void clearInput();

  // Updates the table widget and manages the enabled states of the buttons
  void updateTable();

  // Overrides the accept method of the dialog (OK clicked)
  void accept() override;

  // Overrides the reject method of the dialog (Cancel clicked, window closed, ...)
  void reject() override;

  // Import and export
  void saveJSON();
  void loadJSON();

  // Each time the selection changes in the tableWidget, this function gets called
  void onTableSelectionChanged(const QItemSelection&, const QItemSelection&);

  // Update the request interval
  void updateInterval();
};

#endif  // PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_VARIABLE_DIALOG_H
