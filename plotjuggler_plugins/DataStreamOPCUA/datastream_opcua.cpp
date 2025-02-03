#include "datastream_opcua.h"
#include "variable_dialog.h"

DataStreamOPCUA::DataStreamOPCUA() = default;

bool DataStreamOPCUA::start(QStringList*)
{
  // Create instance of variable loader dialog
  VariableLoader loader(nullptr);

  // If something went wrong or the user pressed cancel, return false (Stream gets killed)
  if (loader.exec() != QDialog::Accepted)
  {
    return false;
  }

  this->opcua_clients_ = loader.getClients();
  this->interval_ = loader.getInterval();

  // Set running to true
  this->running_ = true;

  // Reference to the PlotDataMap
  PJ::PlotDataMapRef& map = dataMap();

  // Iterate over all clients
  for (const auto& client : this->opcua_clients_)
  {
    // Iterate over all variables and register them to the plot
    for (const auto& var : client->getVariables())
    {
      var->registerToDataMap(map);
    }
  }

  // Do a first cycle in order to make sure the first variables are available as soon as the start method has ended
  pushSingleCycle();

  // Start the thread
  _thread = std::thread([this]() { this->loop(); });

  return true;
}

void DataStreamOPCUA::shutdown()
{
  this->stopThread_();
}

bool DataStreamOPCUA::isRunning() const
{
  return this->running_;
}

DataStreamOPCUA::~DataStreamOPCUA()
{
  this->stopThread_();
}

bool DataStreamOPCUA::xmlSaveState(QDomDocument& doc, QDomElement& parent_element) const
{
  return true;
}

bool DataStreamOPCUA::xmlLoadState(const QDomElement& parent_element)
{
  return true;
}

void DataStreamOPCUA::pushSingleCycle()
{
  std::lock_guard<std::mutex> lock(mutex());

  // Iterate over all available clients
  for (const auto& client : this->opcua_clients_)
  {
    auto variables = client->getVariables();

    // Fetch new values from the client
    UA_ReadResponse response = client->fetchData();

    // Request wasn't successful. Return
    if (response.responseHeader.serviceResult != UA_STATUSCODE_GOOD)
    {
      qDebug() << "Error while fetching a single cycle in DataStreamOPCUA";

      // Increment number of failed cycles
      this->failed_cycles_++;

      // Return
      return;
    }

    // Reset failed cycles
    this->failed_cycles_ = 0;

    using namespace std::chrono;
    static std::chrono::high_resolution_clock::time_point initial_time = high_resolution_clock::now();
    const double offset = duration_cast<duration<double>>(initial_time.time_since_epoch()).count();

    auto now = high_resolution_clock::now();
    const double t = duration_cast<duration<double>>(now - initial_time).count();

    // Iterate over all results
    for (size_t i = 0; i < response.resultsSize; i++)
    {
      // Partial result has been successful
      if (response.results[i].status == UA_STATUSCODE_GOOD)
      {
        if (UA_Variant_isScalar(&response.results[i].value))
        {
          void* rawData = response.results[i].value.data;
          auto& plot = dataMap().numeric.at(variables[i]->getName());
          double value;
          switch (variables[i]->getType())
          {
            case UA_NS0ID_FLOAT: {
              value = *(UA_Float*)rawData;
              break;
            }
            case UA_NS0ID_DOUBLE: {
              value = *(UA_Double*)rawData;
              break;
            }
            case UA_NS0ID_INT16: {
              value = *(UA_Int16*)rawData;
              break;
            }
            case UA_NS0ID_INT32: {
              value = *(UA_Int32*)rawData;
              break;
            }
            case UA_NS0ID_INT64: {
              value = *(UA_Int64*)rawData;
              break;
            }
            case UA_NS0ID_BYTE: {
              value = *(UA_Byte*)rawData;
              break;
            }
            case UA_NS0ID_UINT16: {
              value = *(UA_UInt16*)rawData;
              break;
            }
            case UA_NS0ID_UINT32: {
              value = *(UA_UInt32*)rawData;
              break;
            }
            case UA_NS0ID_UINT64: {
              value = *(UA_UInt64*)rawData;
              break;
            }
            case UA_NS0ID_BOOLEAN: {
              value = *(UA_Boolean*)rawData;
              break;
            }
          }
          plot.pushBack(PJ::PlotData::Point(t + offset, value));
        }
        else
        {
          switch (variables[i]->getType())
          {
            case UA_NS0ID_FLOAT: {
              auto value = (UA_Float*)response.results[i].value.data;
              std::vector<UA_Float> vec(value, value + response.results[i].value.arrayLength);
              this->append_vector(variables[i]->getName(), vec, t + offset);
              break;
            }
            case UA_NS0ID_DOUBLE: {
              auto value = (UA_Double*)response.results[i].value.data;
              std::vector<UA_Double> vec(value, value + response.results[i].value.arrayLength);
              this->append_vector(variables[i]->getName(), vec, t + offset);
              break;
            }
            case UA_NS0ID_INT16: {
              auto value = (UA_Int16*)response.results[i].value.data;
              std::vector<UA_Int16> vec(value, value + response.results[i].value.arrayLength);
              this->append_vector(variables[i]->getName(), vec, t + offset);
              break;
            }
            case UA_NS0ID_INT32: {
              auto value = (UA_Int32*)response.results[i].value.data;
              std::vector<UA_Int32> vec(value, value + response.results[i].value.arrayLength);
              this->append_vector(variables[i]->getName(), vec, t + offset);
              break;
            }
            case UA_NS0ID_INT64: {
              auto value = (UA_Int64*)response.results[i].value.data;
              std::vector<UA_Int64> vec(value, value + response.results[i].value.arrayLength);
              this->append_vector(variables[i]->getName(), vec, t + offset);
              break;
            }
            case UA_NS0ID_BYTE: {
              auto value = (UA_Byte*)response.results[i].value.data;
              std::vector<UA_Byte> vec(value, value + response.results[i].value.arrayLength);
              this->append_vector(variables[i]->getName(), vec, t + offset);
              break;
            }
            case UA_NS0ID_UINT16: {
              auto value = (UA_UInt16*)response.results[i].value.data;
              std::vector<UA_UInt16> vec(value, value + response.results[i].value.arrayLength);
              this->append_vector(variables[i]->getName(), vec, t + offset);
              break;
            }
            case UA_NS0ID_UINT32: {
              auto value = (UA_UInt32*)response.results[i].value.data;
              std::vector<UA_UInt32> vec(value, value + response.results[i].value.arrayLength);
              this->append_vector(variables[i]->getName(), vec, t + offset);
              break;
            }
            case UA_NS0ID_UINT64: {
              auto value = (UA_UInt64*)response.results[i].value.data;
              std::vector<UA_UInt64> vec(value, value + response.results[i].value.arrayLength);
              this->append_vector(variables[i]->getName(), vec, t + offset);
              break;
            }
            case UA_NS0ID_BOOLEAN: {
              auto value = (UA_Boolean*)response.results[i].value.data;
              std::vector<UA_Boolean> vec(value, value + response.results[i].value.arrayLength);
              this->append_vector(variables[i]->getName(), vec, t + offset);
              break;
            }
          }
        }
      }
    }

    UA_ReadResponse_clear(&response);
  }
}

void DataStreamOPCUA::loop()
{
  this->running_ = true;
  while (this->running_)
  {
    // Do a new plot iteration
    pushSingleCycle();

    // Sleep for the interval milliseconds
    std::this_thread::sleep_for(std::chrono::milliseconds(this->interval_));

    // Check whether the number of failed cycles has exceeded the maximum number of failed cycles. If that's the case,
    // shutdown the stream
    if (this->failed_cycles_ >= this->MAX_FAILED_CYCLES)
    {
      QString failedCounterString = QString::fromStdString(std::to_string(this->failed_cycles_));
      DataStreamOPCUA::showMessagePopup("error", failedCounterString + " successive failed cycles, stopping stream");
      break;
    }
  }

  // Set atomic bool for thead running to false -> thread gets stopped
  this->running_ = false;
  // Disconnect from all clients
  this->cleanUp_();
}

template <typename T>
void DataStreamOPCUA::append_vector(const std::string& name, std::vector<T> vec, double x)
{
  for (size_t j = 0; j < vec.size(); j++)
  {
    auto& plot = dataMap().numeric.at(name + "/" + std::to_string(j));

    plot.pushBack(PJ::PlotData::Point(x, vec[j]));
  }
}

void DataStreamOPCUA::cleanUp_()
{
  // Disconnect all clients
  for (const auto& client : this->opcua_clients_)
  {
    client->disconnect();
  }

  // Clear dataMap
  this->dataMap().numeric.clear();
  this->dataMap().user_defined.clear();

  // Delete clients
  this->opcua_clients_.clear();
}

void DataStreamOPCUA::stopThread_()
{
  // Set running to false
  this->running_ = false;

  // If the thread hasn't stopped yet, do it now
  if (this->_thread.joinable())
    this->_thread.join();
}

void DataStreamOPCUA::showMessagePopup(const QString& type, const QString& message)
{
  QMessageBox msgBox;
  msgBox.setWindowTitle(type);
  msgBox.setText(message);
  msgBox.setMinimumWidth(400);
  msgBox.exec();
}
