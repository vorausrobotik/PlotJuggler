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
    static auto initial_time = high_resolution_clock::now();
    const double offset = duration_cast<duration<double>>(initial_time.time_since_epoch()).count();
  
    auto now = high_resolution_clock::now();
    const double stamp = duration_cast<duration<double>>(now - initial_time).count() + offset;

    // Iterate over all results
    for (size_t i = 0; i < response.resultsSize; i++)
    {
      // Partial result has been successful
      const auto &ua_data_value = response.results[i];
      if (ua_data_value.status == UA_STATUSCODE_GOOD && ua_data_value.hasValue)
      {
        const auto& ua_variant = ua_data_value.value;

        if (UA_Variant_isEmpty(&ua_variant)) {
          qDebug() << "UA_Variant is empty in DataStreamOPCUA";
          continue;
        }

        if (UA_Variant_isScalar(&ua_variant))
        {
          double numeric_scalar = UA_Variant_to_numeric_scalar(ua_variant);

          auto& plot = dataMap().numeric.at(variables[i]->getName());
          plot.pushBack(PJ::PlotData::Point(stamp, numeric_scalar));
        }
        else
        {
          std::vector<double> numeric_vector = UA_Variant_to_numeric_vector(ua_variant);

          for (size_t j = 0; j < numeric_vector.size(); j++)
          {
            auto& plot = dataMap().numeric.at(variables[i]->getName() + "/" + std::to_string(j));
            plot.pushBack(PJ::PlotData::Point(stamp, numeric_vector[j]));
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
    emit dataReceived();

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

double UA_Variant_to_numeric_scalar(const UA_Variant &ua_variant) {
  double value{};

  switch (ua_variant.type->typeId.identifier.numeric)
  {
    case UA_NS0ID_FLOAT: {
      value = *(UA_Float*)ua_variant.data;
      break;
    }
    case UA_NS0ID_DOUBLE: {
      value = *(UA_Double*)ua_variant.data;
      break;
    }
    case UA_NS0ID_INT16: {
      value = *(UA_Int16*)ua_variant.data;
      break;
    }
    case UA_NS0ID_INT32: {
      value = *(UA_Int32*)ua_variant.data;
      break;
    }
    case UA_NS0ID_INT64: {
      value = *(UA_Int64*)ua_variant.data;
      break;
    }
    case UA_NS0ID_BYTE: {
      value = *(UA_Byte*)ua_variant.data;
      break;
    }
    case UA_NS0ID_UINT16: {
      value = *(UA_UInt16*)ua_variant.data;
      break;
    }
    case UA_NS0ID_UINT32: {
      value = *(UA_UInt32*)ua_variant.data;
      break;
    }
    case UA_NS0ID_UINT64: {
      value = *(UA_UInt64*)ua_variant.data;
      break;
    }
    case UA_NS0ID_BOOLEAN: {
      value = *(UA_Boolean*)ua_variant.data;
      break;
    }
  }
  return value;
}

template<typename T>
std::vector<double> cast_numeric_array(void *data, size_t length){
  auto value = static_cast<T*>(data);
  std::vector<T> casted_vector(value, value + length);
  std::vector<double> numeric_vector(casted_vector.begin(), casted_vector.end());
  return numeric_vector;
}

std::vector<double> UA_Variant_to_numeric_vector(const UA_Variant &ua_variant) {
  std::vector<double> numeric_vector{};

  switch (ua_variant.type->typeId.identifier.numeric)
  {
    case UA_NS0ID_FLOAT: 
      numeric_vector = cast_numeric_array<UA_Float>(ua_variant.data, ua_variant.arrayLength);
      break;
    case UA_NS0ID_DOUBLE: 
      numeric_vector = cast_numeric_array<UA_Double>(ua_variant.data, ua_variant.arrayLength);
      break;
    case UA_NS0ID_INT16: 
      numeric_vector = cast_numeric_array<UA_Int16>(ua_variant.data, ua_variant.arrayLength);
      break;
    case UA_NS0ID_INT32: 
      numeric_vector = cast_numeric_array<UA_Int32>(ua_variant.data, ua_variant.arrayLength);
      break;
    case UA_NS0ID_INT64: 
      numeric_vector = cast_numeric_array<UA_Int64>(ua_variant.data, ua_variant.arrayLength);
      break;
    case UA_NS0ID_BYTE: 
      numeric_vector = cast_numeric_array<UA_Byte>(ua_variant.data, ua_variant.arrayLength);
      break;
    case UA_NS0ID_UINT16: 
      numeric_vector = cast_numeric_array<UA_UInt16>(ua_variant.data, ua_variant.arrayLength);
      break;
    case UA_NS0ID_UINT32: 
      numeric_vector = cast_numeric_array<UA_UInt32>(ua_variant.data, ua_variant.arrayLength);
      break;
    case UA_NS0ID_UINT64: 
      numeric_vector = cast_numeric_array<UA_UInt64>(ua_variant.data, ua_variant.arrayLength);
      break;
    case UA_NS0ID_BOOLEAN: 
      numeric_vector = cast_numeric_array<UA_Boolean>(ua_variant.data, ua_variant.arrayLength);
      break;
  }

  return numeric_vector;
}