#ifndef PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_H
#define PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_H

#include <QtPlugin>
#include <thread>
#include "PlotJuggler/datastreamer_base.h"
#include <vector>
#include "opcua_client.h"

class DataStreamOPCUA : public PJ::DataStreamer
{
  Q_OBJECT
  Q_PLUGIN_METADATA(IID "facontidavide.PlotJuggler3.DataStreamer")
  Q_INTERFACES(PJ::DataStreamer)

public:
  /**
   * Constructor
   */
  DataStreamOPCUA();

  /**
   * Destructor
   */
  ~DataStreamOPCUA() override;

  /**
   * Start method
   * @return true if the start was successful, false otherwise
   */
  bool start(QStringList*) override;

  /**
   * Shutdown method
   */
  void shutdown() override;

  /**
   * @return whether the plugin is running
   */
  bool isRunning() const override;

  /**
   * Returns the name of this plugin
   */
  const char* name() const override
  {
    return "OPCUA Stream";
  }

  /**
   * Returns the whether the plugin is a debug plugin
   * @return false
   */
  bool isDebugPlugin() override
  {
    return false;
  }

  bool xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const override;

  bool xmlLoadState(const QDomElement& parent_element) override;

private:
  // List of opcua clients
  OPCUAClientList opcua_clients_;

  // Update interval in ms
  uint32_t interval_{};

  // Maximum number of successive failed cycles
  const uint64_t MAX_FAILED_CYCLES = 5;

  /**
   * Internal loop method. Periodically fetches data from OPC UA server
   */
  void loop();

  /**
   * Disconnect all clients and clear dataMap
   */
  void cleanUp_();

  /**
   * Stops the main thread
   */
  void stopThread_();

  /**
   * Shows a dismissible dialog with a message
   * @param type title of the dialog
   * @param message content of the dialog
   */
  static void showMessagePopup(const QString& type, const QString& message);

  // Thread variables
  std::atomic<bool> running_{ false };
  std::thread _thread;

  // Number of failed successive cycles. Gets reset as soon as the client is reachable again.
  uint32_t failed_cycles_{};

  /**
   * Fetches a single cycle from the OPCUA Servers and plots it
   */
  void pushSingleCycle();

  template <typename T>
  /**
   * Appends a vector with a generic type to the plot
   */
  void append_vector(const std::string& name, std::vector<T> vec, double x);
};

#endif  // PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_H
