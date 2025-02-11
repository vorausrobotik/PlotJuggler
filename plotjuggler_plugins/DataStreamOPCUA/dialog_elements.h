

#ifndef PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_GUI_DIALOG_ELEMENTS_H
#define PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_GUI_DIALOG_ELEMENTS_H

#include <QMainWindow>
#include <QDialog>
#include <QLabel>
#include <QStandardPaths>
#include <QFileDialog>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <vector>
#include <QTableWidget>
#include <QHeaderView>
#include <QVBoxLayout>
#include <variant>
#include <QMessageBox>
#include <QDialogButtonBox>
#include "iostream"
#include "opcua_variable.h"
#include "opcua_client.h"
#include <fstream>

template <typename ValueType, typename EditElement>
class FormElement
{
protected:
  QLabel* label_{};
  EditElement* edit_{};
  const uint32_t formRowHeight_ = 30;

public:
  explicit FormElement(const QString& title)
  {
    this->label_ = new QLabel;
    this->label_->setText(title);
    this->edit_ = new EditElement;
  }

  /**
   * Returns the value of the form element.
   *
   * @return valueType value of the form element.
   */
  virtual ValueType getValue() = 0;

  /**
   * Register a callback function to the onChange event of the inherited class (can be value change or pressed of a
   * button)
   *
   * @param receiver the receiver element
   * @param slot the slot function (callback)
   */
  virtual void onChange(const QObject* receiver, const char* slot) = 0;

  /**
   * Reset the form element
   */
  virtual void clear() = 0;

  /**
   * Adds the form element to a layout element
   * @param layout is the layout element
   */
  void addToLayout(QHBoxLayout* layout)
  {
    auto vLayout = new QVBoxLayout;
    vLayout->addWidget(this->edit_);
    vLayout->addWidget(this->label_);
    layout->addLayout(vLayout);
  }
};

/**
 * Form element for number inputs
 */
class FormNumberElement : public FormElement<int, QSpinBox>
{
public:
  FormNumberElement(const QString& title, int initialValue);

  int getValue() override;

  void clear() override;

  void onChange(const QObject* receiver, const char* slot) override;
};

/**
 * Form element for text elements
 */
class FormStringElement : public FormElement<QString, QLineEdit>
{
public:
  FormStringElement(const QString& title, const QString& placeholder);

  QString getValue() override;

  void clear() override;

  void onChange(const QObject* receiver, const char* slot) override;
};

/**
 * Form element for buttons
 */
class FormButtonElement : public FormElement<void, QPushButton>
{
private:
  QDialogButtonBox::ButtonRole role_;

public:
  explicit FormButtonElement(const QString& title);
  FormButtonElement(const QString& title, QDialogButtonBox::ButtonRole role);

  void getValue() override{};

  void clear() override{};

  void onChange(const QObject* receiver, const char* slot) override;

  void addToButtonBox(QDialogButtonBox* buttonBox);

  void setEnabled(bool enabled);
};

#endif  // PLOTJUGGLER_PLUGINS_DATASTREAMOPCUA_GUI_DIALOG_ELEMENTS_H
